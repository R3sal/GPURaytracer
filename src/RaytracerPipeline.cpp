//include-files
#include "RaytracerPipeline.h"



namespace RT::GraphicsAPI
{

	static std::random_device s_stdSeedGenerator;
	


	//the camera ray generator class
	//class constructor
	CameraRayGen::CameraRayGen() :
		//initialize the class variables
		m_rtFrameScheduler(nullptr),
		m_rtCameraRayGenState(nullptr),
		m_rtUAVDescriptorHeap(nullptr),
		m_rtInfoData(),
		m_rtCameraRayGenInfoBuffer(nullptr),
		m_rtRayBuffer(nullptr),
		m_rtOldRayBuffer(nullptr),
		m_rtRayPixelsBuffer(nullptr),
		m_stdPRNG(s_stdSeedGenerator())
	{

	}

	//destructor: uninitializes all our pointers
	CameraRayGen::~CameraRayGen()
	{

	}



	//private class functions



	//public class functions
	bool CameraRayGen::Initialize(GPUScheduler* rtScheduler, DescriptorHeap* rtUAVDescriptorTable, CameraInfo rtCameraData)
	{
		//assign the device
		m_rtFrameScheduler = rtScheduler;
		ID3D12CommandQueue* d3dCommandQueue = m_rtFrameScheduler->GetDX12Device()->GetCommandQueue();
		IDXGISwapChain4* dxSwapChain = m_rtFrameScheduler->GetDX12Device()->GetSwapChain();
		m_rtUAVDescriptorHeap = rtUAVDescriptorTable;


		//create the pipeline state for the camera ray generation shader
		RootSignature rtRootSignatures;
		DescriptorTable rtDescriptorTable;
		rtRootSignatures.AddConstantBuffer(0, 0, ShaderStageCS);
		rtDescriptorTable.AddUAVRange(0, 0, 3);
		rtRootSignatures.AddDescriptorTable(rtDescriptorTable, ShaderStageCS);

		m_rtCameraRayGenState = new PipelineState();
		m_rtCameraRayGenState->Initialize(m_rtFrameScheduler, true);
		if (!(m_rtCameraRayGenState->SetRootSignature(rtRootSignatures))) return false;
		if (!(m_rtCameraRayGenState->SetCS("shader/shaderbin/CS_CameraRayGeneration.cso"))) return false;
		if (!(m_rtCameraRayGenState->CreatePSO())) return false;

		//create the resources
		m_rtCameraRayGenInfoBuffer = new ConstantBuffer();
		m_rtRayBuffer = new RWStructuredBuffer();
		m_rtOldRayBuffer = new RWStructuredBuffer();
		m_rtRayPixelsBuffer = new RWStructuredBuffer();
		if (!m_rtCameraRayGenInfoBuffer) return false;
		if (!m_rtRayBuffer) return false;
		if (!m_rtOldRayBuffer) return false;
		if (!m_rtRayPixelsBuffer) return false;

		if (!(m_rtCameraRayGenInfoBuffer->Initialize(m_rtFrameScheduler, sizeof(CameraRayGenInfo), {}))) return false;
		if (!(m_rtRayBuffer->Initialize(m_rtFrameScheduler, SIZEOF_RAY, MAX_RAYS, DescriptorHeapInfo(m_rtUAVDescriptorHeap, 0)))) return false;
		if (!(m_rtOldRayBuffer->Initialize(m_rtFrameScheduler, SIZEOF_RAY, MAX_RAYS, DescriptorHeapInfo(m_rtUAVDescriptorHeap, 1)))) return false;
		if (!(m_rtRayPixelsBuffer->Initialize(m_rtFrameScheduler, SIZEOF_RAYPIXEL, MAX_RAYS, DescriptorHeapInfo(m_rtUAVDescriptorHeap, 2)))) return false;

		//store the info data and make it visible to the gpu
		float fAspectRatio = (float)RT_WINDOW_WIDTH / (float)RT_WINDOW_HEIGHT;
		DirectX::XMVECTOR xmDeterminant{};
		DirectX::XMMATRIX xmInverseProjectionMatrix = DirectX::XMMatrixInverse(&xmDeterminant, DirectX::XMMatrixTranspose(
			DirectX::XMMatrixPerspectiveFovLH(rtCameraData.VerticalFOV, fAspectRatio, rtCameraData.NearZ, rtCameraData.FarZ)));
		DirectX::XMMATRIX xmInverseViewMatrix = DirectX::XMMatrixInverse(&xmDeterminant, DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtLH(
			DirectX::XMLoadFloat3(&(rtCameraData.Position)), DirectX::XMLoadFloat3(&(rtCameraData.FocusPoint)), DirectX::XMLoadFloat3(&(rtCameraData.UpDirection)))));

		DirectX::XMStoreFloat4x4(&(m_rtInfoData.InverseView), xmInverseViewMatrix);
		DirectX::XMStoreFloat4x4(&(m_rtInfoData.InverseProjection), xmInverseProjectionMatrix);
		m_rtInfoData.ScreenSize.x = RT_WINDOW_WIDTH;
		m_rtInfoData.ScreenSize.y = RT_WINDOW_HEIGHT;
		m_rtInfoData.AASampleSpread = AA_SAMPLE_SPREAD;
		m_rtInfoData.DOFSampleSpread = DOF_SAMPLE_SPREAD;
		m_rtInfoData.MaxRaysPerPixel = MAX_RAYS_PER_PIXEL;
		m_rtInfoData.RNGSeed.x = 0;
		m_rtInfoData.RNGSeed.y = 0;
		m_rtInfoData.RNGSeed.z = 0;
		m_rtCameraRayGenInfoBuffer->UpdateAll(&m_rtInfoData);

		return true;
	}


	//render a single frame
	bool CameraRayGen::Render()
	{
		ID3D12CommandQueue* d3dCommandQueue = m_rtFrameScheduler->GetDX12Device()->GetCommandQueue();
		IDXGISwapChain4* dxSwapChain = m_rtFrameScheduler->GetDX12Device()->GetSwapChain();
		ID3D12GraphicsCommandList* d3dCommandList = m_rtFrameScheduler->GetCommandList();


		//update the info buffer
		m_rtInfoData.RNGSeed.x = m_stdPRNG();
		m_rtInfoData.RNGSeed.y = m_stdPRNG();
		m_rtInfoData.RNGSeed.z = m_stdPRNG();
		m_rtCameraRayGenInfoBuffer->Update(&m_rtInfoData);

		//camera ray generation
		m_rtCameraRayGenState->Bind();
		m_rtUAVDescriptorHeap->Bind(1, 0, true);
		m_rtCameraRayGenInfoBuffer->Bind(0, true);

		D3D12_RESOURCE_BARRIER d3dUAVBarriers[3] = {};
		d3dUAVBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarriers[0].UAV.pResource = m_rtRayBuffer->GetResources()[0];
		d3dUAVBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarriers[1].UAV.pResource = m_rtOldRayBuffer->GetResources()[0];
		d3dUAVBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarriers[2].UAV.pResource = m_rtRayPixelsBuffer->GetResources()[0];
		d3dCommandList->ResourceBarrier(3, d3dUAVBarriers);

		//dispatch the workload
		d3dCommandList->Dispatch((RT_WINDOW_WIDTH + 15) / 16, (RT_WINDOW_HEIGHT + 15) / 16, 1);

		d3dUAVBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarriers[0].UAV.pResource = m_rtRayBuffer->GetResources()[0];
		d3dUAVBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarriers[1].UAV.pResource = m_rtOldRayBuffer->GetResources()[0];
		d3dUAVBarriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarriers[2].UAV.pResource = m_rtRayPixelsBuffer->GetResources()[0];
		d3dCommandList->ResourceBarrier(3, d3dUAVBarriers);

		return true;
	}



	//the ray tracing class
	//class constructor
	SortPrimitives::SortPrimitives() :
		//initialize the class variables
		m_rtFrameScheduler(nullptr),
		m_rtGenMortonCodeState(nullptr),
		m_rtSortCountState(nullptr),
		m_rtSortPrefixSumState(nullptr),
		m_rtSortSortState(nullptr),
		m_rtMortonCodeInfoData(),
		m_rtSortInfoData(),
		m_rtMortonCodeInfoBuffer(nullptr),
		m_rtSortInfoBuffer(),
		m_rtMortonCodeBuffer(nullptr),
		m_rtTempMortonCodeBuffer(nullptr),
		m_rtCodeFrequenciesBuffer(nullptr)
	{

	}

	//destructor: uninitializes all our pointers
	SortPrimitives::~SortPrimitives()
	{

	}



	//private class functions



	//public class functions
	bool SortPrimitives::Initialize(GPUScheduler* rtScheduler, uint32_t iNumPrimitives, AABB rtSceneAABB)
	{
		//assign the device
		m_rtFrameScheduler = rtScheduler;
		ID3D12CommandQueue* d3dCommandQueue = m_rtFrameScheduler->GetDX12Device()->GetCommandQueue();
		IDXGISwapChain4* dxSwapChain = m_rtFrameScheduler->GetDX12Device()->GetSwapChain();

		RootSignature rtRootSignatures;


		//create the pipeline states for the different shaders
		rtRootSignatures.Release();
		rtRootSignatures.AddConstantBuffer(0, 0, ShaderStageCS);
		rtRootSignatures.AddShaderResource(0, 0, ShaderStageCS);
		rtRootSignatures.AddShaderResource(1, 0, ShaderStageCS);
		rtRootSignatures.AddUnorderedAccessResource(5, 0, ShaderStageCS);
		rtRootSignatures.AddUnorderedAccessResource(7, 0, ShaderStageCS);
		m_rtGenMortonCodeState = new PipelineState();
		m_rtGenMortonCodeState->Initialize(m_rtFrameScheduler, true);
		if (!(m_rtGenMortonCodeState->SetRootSignature(rtRootSignatures))) return false;
		if (!(m_rtGenMortonCodeState->SetCS("shader/shaderbin/CS_GenerateMortonCodes.cso"))) return false;
		if (!(m_rtGenMortonCodeState->CreatePSO())) return false;

		rtRootSignatures.Release();
		rtRootSignatures.AddConstantBuffer(0, 0, ShaderStageCS);
		rtRootSignatures.AddUnorderedAccessResource(5, 0, ShaderStageCS);
		rtRootSignatures.AddUnorderedAccessResource(6, 0, ShaderStageCS);
		rtRootSignatures.AddUnorderedAccessResource(7, 0, ShaderStageCS);
		m_rtSortCountState = new PipelineState();
		m_rtSortCountState->Initialize(m_rtFrameScheduler, true);
		if (!(m_rtSortCountState->SetRootSignature(rtRootSignatures))) return false;
		if (!(m_rtSortCountState->SetCS("shader/shaderbin/CS_SortCount.cso"))) return false;
		if (!(m_rtSortCountState->CreatePSO())) return false;

		rtRootSignatures.Release();
		rtRootSignatures.AddConstantBuffer(0, 0, ShaderStageCS);
		rtRootSignatures.AddUnorderedAccessResource(7, 0, ShaderStageCS);
		m_rtSortPrefixSumState = new PipelineState();
		m_rtSortPrefixSumState->Initialize(m_rtFrameScheduler, true);
		if (!(m_rtSortPrefixSumState->SetRootSignature(rtRootSignatures))) return false;
		if (!(m_rtSortPrefixSumState->SetCS("shader/shaderbin/CS_SortPrefixSum.cso"))) return false;
		if (!(m_rtSortPrefixSumState->CreatePSO())) return false;

		rtRootSignatures.Release();
		rtRootSignatures.AddConstantBuffer(0, 0, ShaderStageCS);
		rtRootSignatures.AddUnorderedAccessResource(5, 0, ShaderStageCS);
		rtRootSignatures.AddUnorderedAccessResource(6, 0, ShaderStageCS);
		rtRootSignatures.AddUnorderedAccessResource(7, 0, ShaderStageCS);
		m_rtSortSortState = new PipelineState();
		m_rtSortSortState->Initialize(m_rtFrameScheduler, true);
		if (!(m_rtSortSortState->SetRootSignature(rtRootSignatures))) return false;
		if (!(m_rtSortSortState->SetCS("shader/shaderbin/CS_SortSort.cso"))) return false;
		if (!(m_rtSortSortState->CreatePSO())) return false;

		//create the constant buffers
		m_rtMortonCodeInfoBuffer = new ConstantBuffer();
		if (!m_rtMortonCodeInfoBuffer) return false;
		if (!(m_rtMortonCodeInfoBuffer->Initialize(m_rtFrameScheduler, sizeof(MortonCodeInfo)))) return false;
		for (uint32_t i = 0; i < 4; i++)
		{
			m_rtSortInfoBuffer[i] = new ConstantBuffer();
			if (!(m_rtSortInfoBuffer[i])) return false;
			if (!(m_rtSortInfoBuffer[i]->Initialize(m_rtFrameScheduler, sizeof(SortInfo)))) return false;
		}

		//create the structured buffers
		m_rtMortonCodeBuffer = new RWStructuredBuffer();
		if (!m_rtMortonCodeBuffer) return false;
		if (!(m_rtMortonCodeBuffer->Initialize(m_rtFrameScheduler, 16, iNumPrimitives))) return false;
		m_rtTempMortonCodeBuffer = new RWStructuredBuffer();
		if (!m_rtTempMortonCodeBuffer) return false;
		if (!(m_rtTempMortonCodeBuffer->Initialize(m_rtFrameScheduler, 16, iNumPrimitives))) return false;
		m_rtCodeFrequenciesBuffer = new RWStructuredBuffer();
		if (!m_rtCodeFrequenciesBuffer) return false;
		if (!(m_rtCodeFrequenciesBuffer->Initialize(m_rtFrameScheduler, 16, 256))) return false;


		//save the scene AABB and the number of primitives
		m_rtMortonCodeInfoData.SceneMin = { rtSceneAABB.Min.x, rtSceneAABB.Min.y, rtSceneAABB.Min.z, 0.0f };
		m_rtMortonCodeInfoData.SceneMax = { rtSceneAABB.Max.x, rtSceneAABB.Max.y, rtSceneAABB.Max.z, 0.0f };
		m_rtMortonCodeInfoData.NumPrimitives = iNumPrimitives;
		m_rtSortInfoData.NumElements = iNumPrimitives;


		return true;
	}


	//render a single frame
	bool SortPrimitives::Sort(RaytracerMesh* rtMesh)
	{
		ID3D12CommandQueue* d3dCommandQueue = m_rtFrameScheduler->GetDX12Device()->GetCommandQueue();
		IDXGISwapChain4* dxSwapChain = m_rtFrameScheduler->GetDX12Device()->GetSwapChain();
		ID3D12GraphicsCommandList* d3dCommandList = m_rtFrameScheduler->GetCommandList();


		//the pass, which calculates the morton codes
		m_rtMortonCodeInfoBuffer->Update(&m_rtMortonCodeInfoData);

		m_rtGenMortonCodeState->Bind();
		m_rtMortonCodeInfoBuffer->Bind(0, true);
		rtMesh->Bind(1, 2, true);
		m_rtMortonCodeBuffer->Bind(3, true);
		m_rtCodeFrequenciesBuffer->Bind(4, true);
		
		D3D12_RESOURCE_BARRIER d3dUAVBarrier[1] = {};
		d3dUAVBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarrier[0].UAV.pResource = m_rtMortonCodeBuffer->GetResources()[0];
		d3dCommandList->ResourceBarrier(1, d3dUAVBarrier);

		d3dCommandList->Dispatch((m_rtSortInfoData.NumElements + 255) / 256, 1, 1);

		d3dUAVBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarrier[0].UAV.pResource = m_rtMortonCodeBuffer->GetResources()[0];
		d3dCommandList->ResourceBarrier(1, d3dUAVBarrier);


		//the sorting
		for (uint32_t i = 0; i < 4; i++)
		{
			//update the info buffer
			m_rtSortInfoData.SortPassIndex = i;
			m_rtSortInfoBuffer[i]->Update(&m_rtSortInfoData);

			//the counting pass
			m_rtSortCountState->Bind();
			m_rtSortInfoBuffer[i]->Bind(0, true);
			m_rtMortonCodeBuffer->Bind(1, true);
			m_rtTempMortonCodeBuffer->Bind(2, true);
			m_rtCodeFrequenciesBuffer->Bind(3, true);

			D3D12_RESOURCE_BARRIER d3dUAVBarriers[2] = {};
			d3dUAVBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dUAVBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			d3dUAVBarriers[0].UAV.pResource = m_rtMortonCodeBuffer->GetResources()[0];
			d3dUAVBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dUAVBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			d3dUAVBarriers[1].UAV.pResource = m_rtCodeFrequenciesBuffer->GetResources()[0];
			d3dCommandList->ResourceBarrier(2, d3dUAVBarriers);

			d3dCommandList->Dispatch((m_rtSortInfoData.NumElements + 255) / 256, 1, 1);

			d3dUAVBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dUAVBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			d3dUAVBarriers[0].UAV.pResource = m_rtMortonCodeBuffer->GetResources()[0];
			d3dUAVBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dUAVBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			d3dUAVBarriers[1].UAV.pResource = m_rtCodeFrequenciesBuffer->GetResources()[0];
			d3dCommandList->ResourceBarrier(2, d3dUAVBarriers);

			//the prefix sum pass
			m_rtSortPrefixSumState->Bind();
			m_rtSortInfoBuffer[i]->Bind(0, true);
			m_rtCodeFrequenciesBuffer->Bind(1, true);

			d3dUAVBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dUAVBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			d3dUAVBarrier[0].UAV.pResource = m_rtCodeFrequenciesBuffer->GetResources()[0];
			d3dCommandList->ResourceBarrier(1, d3dUAVBarriers);

			d3dCommandList->Dispatch(1, 1, 1);

			d3dUAVBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dUAVBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			d3dUAVBarrier[0].UAV.pResource = m_rtCodeFrequenciesBuffer->GetResources()[0];
			d3dCommandList->ResourceBarrier(1, d3dUAVBarriers);

			//the actual sorting pass
			m_rtSortSortState->Bind();
			m_rtSortInfoBuffer[i]->Bind(0, true);
			m_rtMortonCodeBuffer->Bind(1, true);
			m_rtTempMortonCodeBuffer->Bind(2, true);
			m_rtCodeFrequenciesBuffer->Bind(3, true);

			d3dUAVBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dUAVBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			d3dUAVBarriers[0].UAV.pResource = m_rtMortonCodeBuffer->GetResources()[0];
			d3dUAVBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dUAVBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			d3dUAVBarriers[1].UAV.pResource = m_rtCodeFrequenciesBuffer->GetResources()[0];
			d3dCommandList->ResourceBarrier(2, d3dUAVBarriers);

			d3dCommandList->Dispatch(1, 1, 1);

			d3dUAVBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dUAVBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			d3dUAVBarriers[0].UAV.pResource = m_rtMortonCodeBuffer->GetResources()[0];
			d3dUAVBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dUAVBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			d3dUAVBarriers[1].UAV.pResource = m_rtCodeFrequenciesBuffer->GetResources()[0];
			d3dCommandList->ResourceBarrier(2, d3dUAVBarriers);
		}

		return true;
	}



	//the ray tracing class
	//class constructor
	BuildBVH::BuildBVH() :
		//initialize the class variables
		m_rtFrameScheduler(nullptr),
		m_rtBuildLeavesState(nullptr),
		m_rtBuildState(nullptr),
		m_rtBVHInfoData(),
		m_rtBuildLeavesInfoBuffer(nullptr),
		m_rtBVHBuildInfoBuffer(),
		m_rtBVHBuffer(nullptr)
	{

	}

	//destructor: uninitializes all our pointers
	BuildBVH::~BuildBVH()
	{

	}



	//private class functions



	//public class functions
	bool BuildBVH::Initialize(GPUScheduler* rtScheduler, uint32_t iNumPrimitives)
	{
		//assign the device
		m_rtFrameScheduler = rtScheduler;
		ID3D12CommandQueue* d3dCommandQueue = m_rtFrameScheduler->GetDX12Device()->GetCommandQueue();
		IDXGISwapChain4* dxSwapChain = m_rtFrameScheduler->GetDX12Device()->GetSwapChain();

		RootSignature rtRootSignatures;


		//create the pipeline states for the different shaders
		rtRootSignatures.Release();
		rtRootSignatures.AddConstantBuffer(0, 0, ShaderStageCS);
		rtRootSignatures.AddShaderResource(0, 0, ShaderStageCS);
		rtRootSignatures.AddShaderResource(1, 0, ShaderStageCS);
		rtRootSignatures.AddUnorderedAccessResource(5, 0, ShaderStageCS);
		rtRootSignatures.AddUnorderedAccessResource(6, 0, ShaderStageCS);
		m_rtBuildLeavesState = new PipelineState();
		m_rtBuildLeavesState->Initialize(m_rtFrameScheduler, true);
		if (!(m_rtBuildLeavesState->SetRootSignature(rtRootSignatures))) return false;
		if (!(m_rtBuildLeavesState->SetCS("shader/shaderbin/CS_BVHBuildLeaves.cso"))) return false;
		if (!(m_rtBuildLeavesState->CreatePSO())) return false;

		rtRootSignatures.Release();
		rtRootSignatures.AddConstantBuffer(0, 0, ShaderStageCS);
		rtRootSignatures.AddUnorderedAccessResource(6, 0, ShaderStageCS);
		m_rtBuildState = new PipelineState();
		m_rtBuildState->Initialize(m_rtFrameScheduler, true);
		if (!(m_rtBuildState->SetRootSignature(rtRootSignatures))) return false;
		if (!(m_rtBuildState->SetCS("shader/shaderbin/CS_BVHBuild.cso"))) return false;
		if (!(m_rtBuildState->CreatePSO())) return false;

		//create the constant buffers
		m_rtBuildLeavesInfoBuffer = new ConstantBuffer();
		if (!m_rtBuildLeavesInfoBuffer) return false;
		if (!(m_rtBuildLeavesInfoBuffer->Initialize(m_rtFrameScheduler, sizeof(BVHInfo)))) return false;
		for (uint32_t i = 0; i < 32; i++)
		{
			m_rtBVHBuildInfoBuffer[i] = new ConstantBuffer();
			if (!(m_rtBVHBuildInfoBuffer[i])) return false;
			if (!(m_rtBVHBuildInfoBuffer[i]->Initialize(m_rtFrameScheduler, sizeof(BVHInfo)))) return false;
		}

		//create the structured buffers
		m_rtBVHBuffer = new RWStructuredBuffer();
		if (!m_rtBVHBuffer) return false;
		if (!(m_rtBVHBuffer->Initialize(m_rtFrameScheduler, sizeof(AABB), iNumPrimitives * 4))) return false;


		//save the number of BVH leaves
		m_rtBVHInfoData.NumChildren = iNumPrimitives;
		m_rtBVHInfoData.PreviousIndex = 1;
		m_rtBVHInfoData.CurrentIndex = ((iNumPrimitives + 1) / 2) + 1;


		return true;
	}


	//render a single frame
	bool BuildBVH::Build(RaytracerMesh* rtMesh, RWStructuredBuffer* rtMortonCodes)
	{
		ID3D12CommandQueue* d3dCommandQueue = m_rtFrameScheduler->GetDX12Device()->GetCommandQueue();
		IDXGISwapChain4* dxSwapChain = m_rtFrameScheduler->GetDX12Device()->GetSwapChain();
		ID3D12GraphicsCommandList* d3dCommandList = m_rtFrameScheduler->GetCommandList();


		//building the leaves
		m_rtBuildLeavesInfoBuffer->Update(&m_rtBVHInfoData);

		m_rtBuildLeavesState->Bind();
		m_rtBuildLeavesInfoBuffer->Bind(0, true);
		rtMesh->Bind(1, 2, true);
		rtMortonCodes->Bind(3, true);
		m_rtBVHBuffer->Bind(4, true);
		
		D3D12_RESOURCE_BARRIER d3dUAVBarrier[1] = {};
		d3dUAVBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarrier[0].UAV.pResource = m_rtBVHBuffer->GetResources()[0];
		d3dCommandList->ResourceBarrier(1, d3dUAVBarrier);

		m_rtBVHInfoData.NumChildren = (m_rtBVHInfoData.NumChildren + 1) / 2;
		d3dCommandList->Dispatch((m_rtBVHInfoData.NumChildren + 255) / 256, 1, 1);

		d3dUAVBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarrier[0].UAV.pResource = m_rtBVHBuffer->GetResources()[0];
		d3dCommandList->ResourceBarrier(1, d3dUAVBarrier);


		//build the rest of the tree
		uint32_t iIndex = 0;
		for (; m_rtBVHInfoData.NumChildren != 2;)
		{
			//update the info buffer
			m_rtBVHBuildInfoBuffer[iIndex]->Update(&m_rtBVHInfoData);
			m_rtBVHInfoData.NumChildren = (m_rtBVHInfoData.NumChildren + 1) / 2; //refresh the value after updating the buffer with it

			//the compute pass
			m_rtBuildState->Bind();
			m_rtBVHBuildInfoBuffer[iIndex]->Bind(0, true);
			m_rtBVHBuffer->Bind(1, true);

			d3dUAVBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dUAVBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			d3dUAVBarrier[0].UAV.pResource = m_rtBVHBuffer->GetResources()[0];
			d3dCommandList->ResourceBarrier(1, d3dUAVBarrier);

			d3dCommandList->Dispatch((m_rtBVHInfoData.NumChildren + 255) / 256, 1, 1);

			d3dUAVBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dUAVBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			d3dUAVBarrier[0].UAV.pResource = m_rtBVHBuffer->GetResources()[0];
			d3dCommandList->ResourceBarrier(1, d3dUAVBarrier);

			//update some info data
			m_rtBVHInfoData.PreviousIndex = m_rtBVHInfoData.CurrentIndex;
			m_rtBVHInfoData.CurrentIndex += m_rtBVHInfoData.NumChildren;

			iIndex++;
		}

		//construct the trunk
		//update the info buffer
		m_rtBVHInfoData.NumChildren = 2;
		m_rtBVHInfoData.CurrentIndex = 0;
		m_rtBVHBuildInfoBuffer[31]->Update(&m_rtBVHInfoData);

		//the compute pass
		m_rtBuildState->Bind();
		m_rtBVHBuildInfoBuffer[31]->Bind(0, true);
		m_rtBVHBuffer->Bind(1, true);

		d3dUAVBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarrier[0].UAV.pResource = m_rtBVHBuffer->GetResources()[0];
		d3dCommandList->ResourceBarrier(1, d3dUAVBarrier);

		d3dCommandList->Dispatch(1, 1, 1);

		d3dUAVBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarrier[0].UAV.pResource = m_rtBVHBuffer->GetResources()[0];
		d3dCommandList->ResourceBarrier(1, d3dUAVBarrier);

		return true;
	}



	//the ray tracing class
	//class constructor
	TraceRays::TraceRays() :
		//initialize the class variables
		m_rtFrameScheduler(nullptr),
		m_rtTraceRaysState(nullptr),
		m_rtMesh(nullptr),
		m_rtTextures(nullptr),
		m_rtUAVDescriptorHeap(nullptr),
		m_rtInfoData(),
		m_rtTraceRaysInfoBuffer(nullptr),
		m_rtMaterialBuffer(nullptr),
		m_rtScatteredLightBuffer(nullptr),
		m_rtEmittedLightBuffer(nullptr),
		m_stdPRNG(s_stdSeedGenerator())
	{

	}

	//destructor: uninitializes all our pointers
	TraceRays::~TraceRays()
	{

	}



	//private class functions



	//public class functions
	bool TraceRays::Initialize(GPUScheduler* rtScheduler, DescriptorHeap* rtUAVDescriptorTable, MeshInfo rtMeshData)
	{
		//assign the device
		m_rtFrameScheduler = rtScheduler;
		ID3D12CommandQueue* d3dCommandQueue = m_rtFrameScheduler->GetDX12Device()->GetCommandQueue();
		IDXGISwapChain4* dxSwapChain = m_rtFrameScheduler->GetDX12Device()->GetSwapChain();
		m_rtUAVDescriptorHeap = rtUAVDescriptorTable;


		//create the pipeline state for the trace rays shader
		RootSignature rtRootSignatures;
		DescriptorTable rtDescriptorTable1;
		DescriptorTable rtDescriptorTable2;
		rtRootSignatures.AddConstantBuffer(0, 0, ShaderStageCS);
		rtRootSignatures.AddShaderResource(0, 0, ShaderStageCS);
		rtRootSignatures.AddShaderResource(1, 0, ShaderStageCS);
		rtRootSignatures.AddShaderResource(2, 0, ShaderStageCS);
		rtRootSignatures.AddShaderResource(3, 0, ShaderStageCS);
		rtRootSignatures.AddUnorderedAccessResource(6, 0, ShaderStageCS);
		rtDescriptorTable1.AddUAVRange(0, 0, 6);
		rtDescriptorTable2.AddSRVRange(4, 0, 1);
		rtRootSignatures.AddDescriptorTable(rtDescriptorTable1, ShaderStageCS);
		rtRootSignatures.AddDescriptorTable(rtDescriptorTable2, ShaderStageCS);

		m_rtTraceRaysState = new PipelineState();
		m_rtTraceRaysState->Initialize(m_rtFrameScheduler, true);
		if (!(m_rtTraceRaysState->SetRootSignature(rtRootSignatures))) return false;
		if (!(m_rtTraceRaysState->SetCS("shader/shaderbin/CS_TraceRays.cso"))) return false;
		if (!(m_rtTraceRaysState->CreatePSO())) return false;

		//create the resources
		m_rtTraceRaysInfoBuffer = new ConstantBuffer();
		m_rtMaterialBuffer = new StructuredBuffer();
		m_rtScatteredLightBuffer = new RWStructuredBuffer();
		m_rtEmittedLightBuffer = new RWStructuredBuffer();
		if (!m_rtTraceRaysInfoBuffer) return false;
		if (!(m_rtMaterialBuffer)) return false;
		if (!m_rtScatteredLightBuffer) return false;
		if (!m_rtEmittedLightBuffer) return false;

		if (!(m_rtTraceRaysInfoBuffer->Initialize(m_rtFrameScheduler, sizeof(TraceRaysInfo), {}))) return false;
		if (!(m_rtMaterialBuffer->Initialize(m_rtFrameScheduler, sizeof(PBRMaterial), rtMeshData.MaterialCount))) return false;
		if (!(m_rtScatteredLightBuffer->Initialize(m_rtFrameScheduler, 16, MAX_RAYS, DescriptorHeapInfo(m_rtUAVDescriptorHeap, 3)))) return false;
		if (!(m_rtEmittedLightBuffer->Initialize(m_rtFrameScheduler, 16, MAX_RAYS, DescriptorHeapInfo(m_rtUAVDescriptorHeap, 4)))) return false;

		//upload the materials to the gpu
		GPUScheduler rtUploadScheduler;
		UploadBuffer rtUploadBuffer;
		if (!(rtUploadScheduler.Initialize(m_rtFrameScheduler->GetDX12Device()))) return false;
		if (!(rtUploadBuffer.Initialize(&rtUploadScheduler, sizeof(PBRMaterial) * rtMeshData.MaterialCount))) return false;
		if (!(rtUploadBuffer.Update(rtMeshData.Materials))) return false;

		if (!(rtUploadScheduler.Record())) return false;
		if (!(m_rtMaterialBuffer->UploadAll(&rtUploadBuffer))) return false;
		if (!(rtUploadScheduler.Execute())) return false;
		rtUploadScheduler.Flush();
		
		rtUploadBuffer.Release();
		rtUploadScheduler.Release();
		
		//create the mesh
		m_rtMesh = new RaytracerMesh();
		if (!(m_rtMesh->Initialize(m_rtFrameScheduler, rtMeshData))) return false;

		//create the texture atlas
		m_rtTextures = new TextureAtlas();
		for (uint64_t i = 1; i < rtMeshData.TextureNameCount; i++)
		{
			uint32_t iTextureID = 0; //we don't use this, since the textures are sorted by index
			if (!(m_rtTextures->AddTexture(&iTextureID, LoadTextureFromFile(rtMeshData.TextureNames[i])))) return false;
		}
		if (!(m_rtTextures->Initialize(m_rtFrameScheduler, DescriptorHeapInfo(m_rtUAVDescriptorHeap, 7)))) return false;
		
		//store the info data and make it visible to the gpu
		m_rtInfoData.ScreenDimensions.x = RT_WINDOW_WIDTH;
		m_rtInfoData.ScreenDimensions.y = RT_WINDOW_HEIGHT;
		m_rtInfoData.NumIndices = (uint32_t)(m_rtMesh->GetIndexCount());
		m_rtInfoData.NumRays = MAX_RAYS;
		m_rtInfoData.MaxRaysPerPixel = MAX_RAYS_PER_PIXEL;
		m_rtInfoData.RNGSeed.x = 0;
		m_rtInfoData.RNGSeed.y = 0;
		m_rtInfoData.RNGSeed.z = 0;
		m_rtTraceRaysInfoBuffer->UpdateAll(&m_rtInfoData);
		
		return true;
	}


	//render a single frame
	bool TraceRays::Render(RWStructuredBuffer* rtBVH)
	{
		ID3D12CommandQueue* d3dCommandQueue = m_rtFrameScheduler->GetDX12Device()->GetCommandQueue();
		IDXGISwapChain4* dxSwapChain = m_rtFrameScheduler->GetDX12Device()->GetSwapChain();
		ID3D12GraphicsCommandList* d3dCommandList = m_rtFrameScheduler->GetCommandList();


		//update the info buffer
		m_rtInfoData.RNGSeed.x = m_stdPRNG();
		m_rtInfoData.RNGSeed.y = m_stdPRNG();
		m_rtInfoData.RNGSeed.z = m_stdPRNG();
		m_rtTraceRaysInfoBuffer->Update(&m_rtInfoData);

		m_rtTraceRaysState->Bind();
		rtBVH->Bind(5, true);
		m_rtUAVDescriptorHeap->Bind(6, 0, true);
		m_rtUAVDescriptorHeap->Bind(7, 7, true, false);
		m_rtTraceRaysInfoBuffer->Bind(0, true);
		m_rtMaterialBuffer->Bind(3, true);
		m_rtMesh->Bind(1, 2, true);
		m_rtTextures->Bind(4, true);
		
		D3D12_RESOURCE_BARRIER d3dUAVBarriers[2] = {};
		d3dUAVBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarriers[0].UAV.pResource = m_rtScatteredLightBuffer->GetResources()[0];
		d3dUAVBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarriers[1].UAV.pResource = m_rtEmittedLightBuffer->GetResources()[0];
		d3dCommandList->ResourceBarrier(2, d3dUAVBarriers);

		d3dCommandList->Dispatch((MAX_RAYS + 255) / 256, 1, 1);

		d3dUAVBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarriers[0].UAV.pResource = m_rtScatteredLightBuffer->GetResources()[0];
		d3dUAVBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarriers[1].UAV.pResource = m_rtEmittedLightBuffer->GetResources()[0];
		d3dCommandList->ResourceBarrier(2, d3dUAVBarriers);

		return true;
	}



	//the final image generation class
	//class constructor
	GenerateFinalImage::GenerateFinalImage() :
		//initialize the class variables
		m_rtFrameScheduler(nullptr),
		m_rtGenerateImageState(nullptr),
		m_rtUAVDescriptorHeap(nullptr),
		m_rtInfoData(),
		m_rtGenerateImageInfoBuffer(nullptr),
		m_rtResultBuffer(nullptr),
		m_rtOutputTexture(nullptr)
	{

	}

	//destructor: uninitializes all our pointers
	GenerateFinalImage::~GenerateFinalImage()
	{

	}



	//private class functions



	//public class functions
	bool GenerateFinalImage::Initialize(GPUScheduler* rtScheduler, DescriptorHeap* rtUAVDescriptorTable, DXGI_FORMAT dxTargetFormat)
	{
		//assign the device
		m_rtFrameScheduler = rtScheduler;
		ID3D12CommandQueue* d3dCommandQueue = m_rtFrameScheduler->GetDX12Device()->GetCommandQueue();
		IDXGISwapChain4* dxSwapChain = m_rtFrameScheduler->GetDX12Device()->GetSwapChain();
		m_rtUAVDescriptorHeap = rtUAVDescriptorTable;


		//create the pipeline state for the image generation shader
		RootSignature rtRootSignatures;
		DescriptorTable rtDescriptorTable;
		rtRootSignatures.AddConstantBuffer(0, 0, ShaderStageCS);
		rtDescriptorTable.AddUAVRange(3, 0, 4);
		rtRootSignatures.AddDescriptorTable(rtDescriptorTable, ShaderStageCS);

		m_rtGenerateImageState = new PipelineState();
		m_rtGenerateImageState->Initialize(m_rtFrameScheduler, true);
		if (!(m_rtGenerateImageState->SetRootSignature(rtRootSignatures))) return false;
		if (!(m_rtGenerateImageState->SetCS("shader/shaderbin/CS_GenerateFinalImage.cso"))) return false;
		if (!(m_rtGenerateImageState->CreatePSO())) return false;

		//create the resources
		m_rtGenerateImageInfoBuffer = new ConstantBuffer();
		m_rtResultBuffer = new RWStructuredBuffer();
		m_rtOutputTexture = new RWTexture2D();
		if (!m_rtGenerateImageInfoBuffer) return false;
		if (!m_rtResultBuffer) return false;
		if (!m_rtOutputTexture) return false;

		if (!(m_rtGenerateImageInfoBuffer->Initialize(m_rtFrameScheduler, sizeof(GenerateFinalImageInfo)))) return false;
		if (!(m_rtResultBuffer->Initialize(m_rtFrameScheduler, 16, RT_WINDOW_WIDTH * RT_WINDOW_HEIGHT,
			DescriptorHeapInfo(m_rtUAVDescriptorHeap, 5)))) return false;
		if (!(m_rtOutputTexture->Initialize(m_rtFrameScheduler, dxTargetFormat,
			RT_WINDOW_WIDTH, RT_WINDOW_HEIGHT, 1, DescriptorHeapInfo(m_rtUAVDescriptorHeap, 6)))) return false;

		//store the info data and make it visible to the gpu
		m_rtInfoData.ScreenDimensions.x = RT_WINDOW_WIDTH;
		m_rtInfoData.ScreenDimensions.y = RT_WINDOW_HEIGHT;
		m_rtInfoData.MaxRaysPerPixel = MAX_RAYS_PER_PIXEL;
		m_rtInfoData.NumSamples = 1;
		m_rtGenerateImageInfoBuffer->UpdateAll(&m_rtInfoData);

		return true;
	}


	//render a single frame
	bool GenerateFinalImage::Render(bool bApplyResults)
	{
		ID3D12CommandQueue* d3dCommandQueue = m_rtFrameScheduler->GetDX12Device()->GetCommandQueue();
		IDXGISwapChain4* dxSwapChain = m_rtFrameScheduler->GetDX12Device()->GetSwapChain();
		ID3D12GraphicsCommandList* d3dCommandList = m_rtFrameScheduler->GetCommandList();


		//update the info buffer
		if (bApplyResults)
		{
			m_rtInfoData.NumSamples |= 0x80000000;
			m_rtInfoData.NumSamples++;
		}
		else
		{
			m_rtInfoData.NumSamples &= 0x7fffffff;
		}
		m_rtGenerateImageInfoBuffer->Update(&m_rtInfoData);

		//the pass to generate the final image
		m_rtGenerateImageState->Bind();
		m_rtUAVDescriptorHeap->Bind(1, 3, true);
		m_rtGenerateImageInfoBuffer->Bind(0, true);

		D3D12_RESOURCE_BARRIER d3dUAVBarriers[2] = {};
		d3dUAVBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarriers[0].UAV.pResource = m_rtResultBuffer->GetResources()[0];
		d3dUAVBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarriers[1].UAV.pResource = m_rtOutputTexture->GetResource();
		d3dCommandList->ResourceBarrier(2, d3dUAVBarriers);

		d3dCommandList->Dispatch((RT_WINDOW_WIDTH + 15) / 16, (RT_WINDOW_HEIGHT + 15) / 16, 1);

		d3dUAVBarriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarriers[0].UAV.pResource = m_rtResultBuffer->GetResources()[0];
		d3dUAVBarriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarriers[1].UAV.pResource = m_rtOutputTexture->GetResource();
		d3dCommandList->ResourceBarrier(2, d3dUAVBarriers);


		return true;
	}



	//the raytracer class
	//constructor: initializes all the variables (at least with "0", "nullptr" or "")
	RaytracerPipeline::RaytracerPipeline() :
		//initialize the class variables
		m_rtDevice(nullptr),
		m_rtFrameScheduler(nullptr),
		m_rtCameraRayGen(nullptr),
		m_rtSortPrimitives(nullptr),
		m_rtBuildBVH(nullptr),
		m_rtTraceRays(nullptr),
		m_rtImageGeneration(nullptr),
		m_rtFinalPass(nullptr),
		m_rtUAVDescriptorHeap(nullptr)
	{

	}

	//destructor: uninitializes all our pointers
	RaytracerPipeline::~RaytracerPipeline()
	{

	}



	//private class functions



	//public class functions
	bool RaytracerPipeline::Initialize(DX12Device* rtDevice, MeshInfo rtMeshData)
	{
		//assign the device
		m_rtDevice = rtDevice;
		ID3D12Device8* d3dDevice = m_rtDevice->GetDevice();
		ID3D12CommandQueue* d3dCommandQueue = m_rtDevice->GetCommandQueue();
		IDXGISwapChain4* dxSwapChain = m_rtDevice->GetSwapChain();


		//create our GPU task schedulers
		m_rtFrameScheduler = new GPUScheduler();
		if (!m_rtFrameScheduler) return false;
		if (!(m_rtFrameScheduler->Initialize(m_rtDevice, RT_NUM_BUFFERS))) return false;

		//create the descriptor and resource heaps
		m_rtUAVDescriptorHeap = new DescriptorHeap();
		if (!(m_rtUAVDescriptorHeap->Initialize(m_rtFrameScheduler, 8, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))) return false;
		
		CameraInfo rtCamera{};
		rtCamera.VerticalFOV = RT_CAMERA_FOV;
		rtCamera.NearZ = RT_CAMERA_NEARZ;
		rtCamera.FarZ = RT_CAMERA_FARZ;
		rtCamera.Position = RT_CAMERA_POSITION;
		rtCamera.FocusPoint = RT_CAMERA_FOCUS_POINT;
		rtCamera.UpDirection = RT_CAMERA_UP_DIRECTION;
		m_rtCameraRayGen = new CameraRayGen();
		if (!(m_rtCameraRayGen->Initialize(m_rtFrameScheduler, m_rtUAVDescriptorHeap, rtCamera))) return false;

		m_rtSortPrimitives = new SortPrimitives();
		if (!(m_rtSortPrimitives->Initialize(m_rtFrameScheduler, rtMeshData.IndexCount / 3, rtMeshData.SceneAABB))) return false;
		
		m_rtBuildBVH = new BuildBVH();
		if (!(m_rtBuildBVH->Initialize(m_rtFrameScheduler, rtMeshData.IndexCount / 3))) return false;

		m_rtTraceRays = new TraceRays();
		if (!(m_rtTraceRays->Initialize(m_rtFrameScheduler, m_rtUAVDescriptorHeap, rtMeshData))) return false;

		m_rtImageGeneration = new GenerateFinalImage();
		if (!(m_rtImageGeneration->Initialize(m_rtFrameScheduler, m_rtUAVDescriptorHeap))) return false;

		//create the class that handles the pass which renders a texture to the back buffer
		m_rtFinalPass = new TextureToScreenPass();
		if (!m_rtFinalPass) return false;
		if (!(m_rtFinalPass->Initialize(m_rtFrameScheduler, RT_WINDOW_WIDTH, RT_WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB))) return false;


		return true;
	}


	//render a single frame
	bool RaytracerPipeline::Render()
	{
		ID3D12Device8* d3dDevice = m_rtDevice->GetDevice();
		ID3D12CommandQueue* d3dCommandQueue = m_rtDevice->GetCommandQueue();
		IDXGISwapChain4* dxSwapChain = m_rtDevice->GetSwapChain();

		if (!(m_rtFrameScheduler->Record())) return false;
		ID3D12GraphicsCommandList* d3dCommandList = m_rtFrameScheduler->GetCommandList();


#if RT_USE_BVH

		//building the bvh (only once)
		static bool bBuildBVH = true;
		if (bBuildBVH)
		{
			if (!(m_rtSortPrimitives->Sort(m_rtTraceRays->GetMesh()))) return false;
			if (!(m_rtBuildBVH->Build(m_rtTraceRays->GetMesh(), m_rtSortPrimitives->GetMortonCodes()))) return false;
			bBuildBVH = false;
		}

#endif


		//camera ray generation
		//only generate rays from the camera on the first iteration
		static unsigned int iIteration = 0;
		if (iIteration == 0)
		{
			if (!(m_rtCameraRayGen->Render())) return false;
		}

		//if we reached the maximum number of iterations, we start again from the camera
		iIteration++;
		if (iIteration == MAX_RAY_DEPTH) iIteration = 0;

		//the ray tracing
		if (!(m_rtTraceRays->Render(m_rtBuildBVH->GetBVH()))) return false;

		//the pass to generate the final image
		if (!(m_rtImageGeneration->Render(iIteration == 0))) return false;

		//the final pass
		m_rtFinalPass->Render(m_rtUAVDescriptorHeap, 6);
		if (!(m_rtFrameScheduler->Execute())) return false;


		//present the frame
		if (dxSwapChain->Present(1, 0) < 0) return false;


		return true;
	}

}