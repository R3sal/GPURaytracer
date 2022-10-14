//include-files
#include "Raytracer.h"



namespace RT::GraphicsAPI
{

	//AppWindow constructor and destructor
	//constructor: initializes all the variables (at least with "0", "nullptr" or "")
	Raytracer::Raytracer() :
		//initialize the class variables
		m_rtDevice(nullptr),
		m_d3dCommandList(nullptr),
		m_d3dCommandAllocators(),
		m_d3dComputeCommandAllocator(nullptr),
		m_d3dFrameFences(),
		m_d3dComputeFence(nullptr),
		m_iFenceValues(),
		m_iComputeFenceValue(0),
		m_iCurrentFenceValue(0),
		m_d3dRTVsDescriptorHeap(nullptr),
		m_d3dUAVDescriptorHeap(nullptr),

		m_d3dPipelineState(nullptr),
		m_d3dComputePipelineState(nullptr),
		m_d3dViewport(),
		m_d3dScissorRect(),

		m_d3dRootSignature(nullptr),
		m_d3dComputeRootSignature(nullptr),
		m_d3dBackBuffers(),
		m_d3dIntersectionTarget(nullptr),
		m_d3dVertexBuffer(nullptr),
		m_d3dVertexBufferView(),
		m_iCurrentQueueSlot(0)
	{
		
	}

	//destructor: uninitializes all our pointers
	Raytracer::~Raytracer()
	{

	}



	//private class functions
	//wait for a fence to finish
	void Raytracer::WaitForFence(ID3D12Fence1* d3dFenceToWaitFor, UINT64 iCompletionValue, DWORD iMaxWaitingTime)
	{
		//create an event, that is raised upon task completion
		HANDLE hEventOnFinish = CreateEvent(nullptr, false, false, nullptr);
		if (!hEventOnFinish) return; //error

		//wait for GPU to complete its execution
		if (d3dFenceToWaitFor->GetCompletedValue() < iCompletionValue)
		{
			d3dFenceToWaitFor->SetEventOnCompletion(iCompletionValue, hEventOnFinish);
			WaitForSingleObject(hEventOnFinish, iMaxWaitingTime);
		}

		//close the event handle to avoid memory leaks
		CloseHandle(hEventOnFinish);
	}



	//public class functions
	bool Raytracer::Initialize(DX12Device& rtDevice)
	{
		//assign the device
		m_rtDevice = &rtDevice;
		ID3D12Device8* d3dDevice = m_rtDevice->GetDevice();
		ID3D12CommandQueue* d3dCommandQueue = m_rtDevice->GetCommandQueue();
		IDXGISwapChain4* dxSwapChain = m_rtDevice->GetSwapChain();


		//create the render targets
		//create the descriptor heap for the render targets
		D3D12_DESCRIPTOR_HEAP_DESC d3dRTVDescriptorHeapDesc{};
		d3dRTVDescriptorHeapDesc.NodeMask = 0;
		d3dRTVDescriptorHeapDesc.NumDescriptors = s_iNumQueueSlots;
		d3dRTVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		d3dRTVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (d3dDevice->CreateDescriptorHeap(&d3dRTVDescriptorHeapDesc, IID_PPV_ARGS(&m_d3dRTVsDescriptorHeap)) < 0) return false;

		//create the render targets themselves
		unsigned int iRTVDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE d3dCPUDescriptorHandle = m_d3dRTVsDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		for (unsigned int i = 0; i < s_iNumQueueSlots; i++)
		{
			//get the buffers of the swap chain
			if (dxSwapChain->GetBuffer(i, IID_PPV_ARGS(&(m_d3dBackBuffers[i]))) < 0) return false;

			D3D12_RENDER_TARGET_VIEW_DESC d3dRTVDesc{};
			d3dRTVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			d3dRTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			d3dRTVDesc.Texture2D.MipSlice = 0;
			d3dRTVDesc.Texture2D.PlaneSlice = 0;
			//create the render target on the given back buffer (bind it to a specific spot in the descriptor handle)
			d3dDevice->CreateRenderTargetView(m_d3dBackBuffers[i], &d3dRTVDesc, d3dCPUDescriptorHandle);
			//offset the pointer in the descriptor handle, so the next iteration doesn't overwrite the current render target descriptor
			d3dCPUDescriptorHandle.ptr += iRTVDescriptorSize;
		}


		//create the viewport and scissor rect
		//viewport
		m_d3dViewport.TopLeftX = 0.0f;
		m_d3dViewport.TopLeftY = 0.0f;
		m_d3dViewport.Width = 1920.0f;
		m_d3dViewport.Height = 1080.0f;
		m_d3dViewport.MinDepth = 0.0f;
		m_d3dViewport.MaxDepth = 1.0f;
		//scissor rect
		m_d3dScissorRect.left = 0;
		m_d3dScissorRect.top = 0;
		m_d3dScissorRect.right = 1920;
		m_d3dScissorRect.bottom = 1080;


		//create some resources, such as command allocators and lists
		if (d3dDevice->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_d3dCommandList)) < 0) return false;
		for (unsigned int i = 0; i < s_iNumQueueSlots; i++)
		{
			//create the command allocators and command lists
			if (d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&(m_d3dCommandAllocators[i]))) < 0) return false;
			//create the frame fences
			if (d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&(m_d3dFrameFences[i]))) < 0) return false;
			m_iFenceValues[i] = 0;
		}
		//create the compute command allocator
		if (d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&(m_d3dComputeCommandAllocator))) < 0) return false;
		//create the compute fence
		if (d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&(m_d3dComputeFence))) < 0) return false;


		//create a root signature and a pipeline state object
		//first, the root signature
		D3D12_DESCRIPTOR_RANGE1 d3dDescriptorRanges[1] = {};
		d3dDescriptorRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
		d3dDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		d3dDescriptorRanges[0].OffsetInDescriptorsFromTableStart = 0;
		d3dDescriptorRanges[0].NumDescriptors = 1;
		d3dDescriptorRanges[0].BaseShaderRegister = 0;
		d3dDescriptorRanges[0].RegisterSpace = 0;
		D3D12_ROOT_PARAMETER1 d3dRootParameters[2] = {};
		d3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		d3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		d3dRootParameters[0].Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
		d3dRootParameters[0].Descriptor.ShaderRegister = 0;
		d3dRootParameters[0].Descriptor.RegisterSpace = 0;
		d3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		d3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		d3dRootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
		d3dRootParameters[1].DescriptorTable.pDescriptorRanges = d3dDescriptorRanges;
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC d3dVersionedSignatureDesc{};
		d3dVersionedSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		d3dVersionedSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		d3dVersionedSignatureDesc.Desc_1_1.NumParameters = 2;
		d3dVersionedSignatureDesc.Desc_1_1.pParameters = d3dRootParameters;
		d3dVersionedSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
		d3dVersionedSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;
		ID3DBlob* d3dSignatureBlob = nullptr;
		ID3DBlob* d3dErrorBlob = nullptr;
		if (D3D12SerializeVersionedRootSignature(&d3dVersionedSignatureDesc, &d3dSignatureBlob, &d3dErrorBlob) < 0) return false;
		if ((!d3dSignatureBlob) || (d3dErrorBlob)) return false;
		if (d3dDevice->CreateRootSignature(0, d3dSignatureBlob->GetBufferPointer(), d3dSignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&m_d3dRootSignature)) < 0) return false;
		//initialize the compute root signature (same as above without the vertex buffer)
		d3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		d3dVersionedSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
		d3dVersionedSignatureDesc.Desc_1_1.NumParameters = 1;
		d3dVersionedSignatureDesc.Desc_1_1.pParameters = d3dRootParameters + 1;
		d3dSignatureBlob = d3dErrorBlob = nullptr;
		if (D3D12SerializeVersionedRootSignature(&d3dVersionedSignatureDesc, &d3dSignatureBlob, &d3dErrorBlob) < 0) return false;
		if ((!d3dSignatureBlob) || (d3dErrorBlob)) return false;
		if (d3dDevice->CreateRootSignature(0, d3dSignatureBlob->GetBufferPointer(), d3dSignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&m_d3dComputeRootSignature)) < 0) return false;

		//second, load the shaders
		std::ifstream stdShaderCodeStream = std::ifstream();
		stdShaderCodeStream.open("VS_FullscreenPass.cso", std::ios::ate | std::ios::binary);
		if (!stdShaderCodeStream.is_open()) return false;
		unsigned int iVertexShaderSize = (unsigned int)(stdShaderCodeStream.tellg());
		stdShaderCodeStream.seekg(0, std::ios::beg);
		char* pVertexShaderData = new char[iVertexShaderSize];
		stdShaderCodeStream.read(pVertexShaderData, iVertexShaderSize);
		stdShaderCodeStream.close();
		stdShaderCodeStream.open("PS_FullscreenPass.cso", std::ios::ate | std::ios::binary);
		if (!stdShaderCodeStream.is_open()) return false;
		unsigned int iPixelShaderSize = (unsigned int)(stdShaderCodeStream.tellg());
		stdShaderCodeStream.seekg(0, std::ios::beg);
		char* pPixelShaderData = new char[iPixelShaderSize];
		stdShaderCodeStream.read(pPixelShaderData, iPixelShaderSize);
		stdShaderCodeStream.close();
		stdShaderCodeStream.open("CS_Intersection.cso", std::ios::ate | std::ios::binary);
		if (!stdShaderCodeStream.is_open()) return false;
		unsigned int iCSIntersectionShaderSize = (unsigned int)(stdShaderCodeStream.tellg());
		stdShaderCodeStream.seekg(0, std::ios::beg);
		char* pCSIntersectionShaderData = new char[iCSIntersectionShaderSize];
		stdShaderCodeStream.read(pCSIntersectionShaderData, iCSIntersectionShaderSize);
		stdShaderCodeStream.close();

		//third, the pipeline state object
		//before filling out the pipeline state desc, just fill out some of the structures it contains
		//an input layout
		const unsigned int iInputElementCount = 2;
		D3D12_INPUT_ELEMENT_DESC d3dInputLayout[iInputElementCount] = {};
		d3dInputLayout[0].SemanticName = "POSITION";
		d3dInputLayout[0].SemanticIndex = 0;
		d3dInputLayout[0].Format = DXGI_FORMAT_R32G32_FLOAT;
		d3dInputLayout[0].InputSlot = 0;
		d3dInputLayout[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		d3dInputLayout[0].InstanceDataStepRate = 0;
		d3dInputLayout[0].AlignedByteOffset = 0;
		d3dInputLayout[1].SemanticName = "TEXCOORD";
		d3dInputLayout[1].SemanticIndex = 0;
		d3dInputLayout[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		d3dInputLayout[1].InputSlot = 0;
		d3dInputLayout[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		d3dInputLayout[1].InstanceDataStepRate = 0;
		d3dInputLayout[1].AlignedByteOffset = 8;
		//a stream output desc
		D3D12_STREAM_OUTPUT_DESC d3dStreamOutputDesc{};
		d3dStreamOutputDesc.NumEntries = 0;
		d3dStreamOutputDesc.NumStrides = 0;
		d3dStreamOutputDesc.pBufferStrides = nullptr;
		d3dStreamOutputDesc.pSODeclaration = nullptr;
		d3dStreamOutputDesc.RasterizedStream = 0;
		//a rasterizer desc
		D3D12_RASTERIZER_DESC d3dRasterizerDesc{};
		d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		d3dRasterizerDesc.FrontCounterClockwise = false;
		d3dRasterizerDesc.DepthClipEnable = true;
		d3dRasterizerDesc.DepthBias = 0;
		d3dRasterizerDesc.DepthBiasClamp = 0.0f;
		d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
		d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		d3dRasterizerDesc.AntialiasedLineEnable = false;
		d3dRasterizerDesc.MultisampleEnable = false;
		d3dRasterizerDesc.ForcedSampleCount = 0;
		//a depth stencil view desc
		D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc{};
		d3dDepthStencilDesc.DepthEnable = false;
		d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		d3dDepthStencilDesc.StencilEnable = false;
		d3dDepthStencilDesc.StencilReadMask = 0xff;
		d3dDepthStencilDesc.StencilWriteMask = 0xff;
		d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
		d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
		d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		//a blend desc
		D3D12_BLEND_DESC d3dBlendDesc{};
		d3dBlendDesc.AlphaToCoverageEnable = false;
		d3dBlendDesc.IndependentBlendEnable = false;
		d3dBlendDesc.RenderTarget[0].BlendEnable = false;
		d3dBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		d3dBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		d3dBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		d3dBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		d3dBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		d3dBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		d3dBlendDesc.RenderTarget[0].LogicOpEnable = false;
		d3dBlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
		d3dBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		//finally, fill out the pipelnie state object
		D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dPipelineStateDesc{};
		d3dPipelineStateDesc.NodeMask = 0;
		d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		d3dPipelineStateDesc.CachedPSO.CachedBlobSizeInBytes = 0;
		d3dPipelineStateDesc.CachedPSO.pCachedBlob = nullptr;
		d3dPipelineStateDesc.pRootSignature = m_d3dRootSignature;
		d3dPipelineStateDesc.InputLayout.NumElements = iInputElementCount;
		d3dPipelineStateDesc.InputLayout.pInputElementDescs = d3dInputLayout;
		d3dPipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		d3dPipelineStateDesc.VS.BytecodeLength = iVertexShaderSize;
		d3dPipelineStateDesc.VS.pShaderBytecode = pVertexShaderData;
		d3dPipelineStateDesc.HS.BytecodeLength = 0;
		d3dPipelineStateDesc.HS.pShaderBytecode = nullptr;
		d3dPipelineStateDesc.DS.BytecodeLength = 0;
		d3dPipelineStateDesc.DS.pShaderBytecode = nullptr;
		d3dPipelineStateDesc.GS.BytecodeLength = 0;
		d3dPipelineStateDesc.GS.pShaderBytecode = nullptr;
		d3dPipelineStateDesc.PS.BytecodeLength = iPixelShaderSize;
		d3dPipelineStateDesc.PS.pShaderBytecode = pPixelShaderData;
		d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		d3dPipelineStateDesc.NumRenderTargets = 1;
		d3dPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		d3dPipelineStateDesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
		d3dPipelineStateDesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
		d3dPipelineStateDesc.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
		d3dPipelineStateDesc.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
		d3dPipelineStateDesc.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
		d3dPipelineStateDesc.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
		d3dPipelineStateDesc.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
		d3dPipelineStateDesc.StreamOutput = d3dStreamOutputDesc;
		d3dPipelineStateDesc.RasterizerState = d3dRasterizerDesc;
		d3dPipelineStateDesc.DepthStencilState = d3dDepthStencilDesc;
		d3dPipelineStateDesc.BlendState = d3dBlendDesc;
		d3dPipelineStateDesc.SampleDesc.Count = 1;
		d3dPipelineStateDesc.SampleDesc.Quality = 0;
		d3dPipelineStateDesc.SampleMask = 0xffffffff;
		
		//create the pso with the desc
		if (d3dDevice->CreateGraphicsPipelineState(&d3dPipelineStateDesc, IID_PPV_ARGS(&m_d3dPipelineState)) < 0) return false;


		//create a pso for the compute pipeline
		D3D12_COMPUTE_PIPELINE_STATE_DESC d3dComputePSODesc{};
		d3dComputePSODesc.CachedPSO.CachedBlobSizeInBytes = 0;
		d3dComputePSODesc.CachedPSO.pCachedBlob = nullptr;
		d3dComputePSODesc.CS.BytecodeLength = iCSIntersectionShaderSize;
		d3dComputePSODesc.CS.pShaderBytecode = pCSIntersectionShaderData;
		d3dComputePSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		d3dComputePSODesc.pRootSignature = m_d3dComputeRootSignature;
		d3dComputePSODesc.NodeMask = 0;
		if (d3dDevice->CreateComputePipelineState(&d3dComputePSODesc, IID_PPV_ARGS(&m_d3dComputePipelineState)) < 0) return false;


		//create and upload the mesh data
		ID3D12CommandAllocator* d3dUploadCommandAllocator = nullptr;
		ID3D12GraphicsCommandList6* d3dUploadCommandList = nullptr;
		ID3D12Fence1* d3dUploadFence = nullptr;
		ID3D12Resource2* d3dUploadBuffer = nullptr;

		//create a command allocator and command list for uploading data
		if (d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&d3dUploadCommandAllocator)) < 0) return false;
		if (d3dDevice->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&d3dUploadCommandList)) < 0) return false;
		if (d3dUploadCommandAllocator->Reset() < 0) return false;
		if (d3dUploadCommandList->Reset(d3dUploadCommandAllocator, nullptr) < 0) return false;
		//create a fence for indicating when the data upload finished
		if (d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3dUploadFence)) < 0) return false;

		//create the upload buffer
		D3D12_HEAP_PROPERTIES d3dUploadHeapProperties{};
		d3dUploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		d3dUploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		d3dUploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		d3dUploadHeapProperties.CreationNodeMask = 1;
		d3dUploadHeapProperties.VisibleNodeMask = 1;
		D3D12_RESOURCE_DESC1 d3dUploadBufferDesc{};
		d3dUploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		d3dUploadBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		d3dUploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		d3dUploadBufferDesc.Alignment = 0;
		d3dUploadBufferDesc.Width = 48; // = sizeof(Vertex) * VertexCount = (sizeof(float) * 4) * 3 = (4 * 4) * 3 = 16 * 3
		d3dUploadBufferDesc.Height = 1;
		d3dUploadBufferDesc.DepthOrArraySize = 1;
		d3dUploadBufferDesc.MipLevels = 1;
		d3dUploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // a requirement for buffers
		d3dUploadBufferDesc.SampleDesc.Count = 1;
		d3dUploadBufferDesc.SampleDesc.Quality = 0;
		d3dUploadBufferDesc.SamplerFeedbackMipRegion.Width = 0;
		d3dUploadBufferDesc.SamplerFeedbackMipRegion.Height = 0;
		d3dUploadBufferDesc.SamplerFeedbackMipRegion.Depth = 0;
		if (d3dDevice->CreateCommittedResource2(&d3dUploadHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dUploadBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, nullptr, IID_PPV_ARGS(&d3dUploadBuffer)) < 0) return false;

		//create the vertex buffer and the vertex buffer view
		//the vertex buffer
		D3D12_HEAP_PROPERTIES d3dVertexHeapProperties{};
		d3dVertexHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		d3dVertexHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		d3dVertexHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		d3dVertexHeapProperties.CreationNodeMask = 1;
		d3dVertexHeapProperties.VisibleNodeMask = 1;
		D3D12_RESOURCE_DESC1 d3dVertexBufferDesc{};
		d3dVertexBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		d3dVertexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		d3dVertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		d3dVertexBufferDesc.Alignment = 0;
		d3dVertexBufferDesc.Width = 48; // = sizeof(Vertex) * VertexCount = (sizeof(float) * 4) * 3 = (4 * 4) * 3 = 16 * 3
		d3dVertexBufferDesc.Height = 1;
		d3dVertexBufferDesc.DepthOrArraySize = 1;
		d3dVertexBufferDesc.MipLevels = 1;
		d3dVertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // a requirement for buffers
		d3dVertexBufferDesc.SampleDesc.Count = 1;
		d3dVertexBufferDesc.SampleDesc.Quality = 0;
		d3dVertexBufferDesc.SamplerFeedbackMipRegion.Width = 0;
		d3dVertexBufferDesc.SamplerFeedbackMipRegion.Height = 0;
		d3dVertexBufferDesc.SamplerFeedbackMipRegion.Depth = 0;
		if (d3dDevice->CreateCommittedResource2(&d3dVertexHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dVertexBufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, nullptr, IID_PPV_ARGS(&m_d3dVertexBuffer)) < 0) return false;
		//the vertex buffer view
		m_d3dVertexBufferView.BufferLocation = m_d3dVertexBuffer->GetGPUVirtualAddress();
		m_d3dVertexBufferView.SizeInBytes = 48; // = sizeof(Vertex) * VertexCount = (sizeof(float) * 4) * 3 = (4 * 4) * 3 = 16 * 3
		m_d3dVertexBufferView.StrideInBytes = 16; // = sizeof(Vertex) = sizeof(float) * 4 = 4 * 4
		
		//create a triangle for the fullscreenpass
		/*one vertex consists of two coordinates for the position and two UV-coordinates
		(we do a fullscreenpass, therefor we don't need the third dimension (the whird coordinate will be set in the vertex shader))*/
		float Vertices[12] = {
			0.0f /*X*/, 3.0f /*Y*/, 0.5f /*U*/, -1.0f /*V*/, //the topmost vertex
			2.0f /*X*/, -1.0f /*Y*/, 1.5f /*U*/, 1.0f /*V*/, //the bottom right vertex
			-2.0f /*X*/, -1.0f /*Y*/, -0.5f /*U*/, 1.0f /*V*/, //the bottom left vertex
		};
		
		//copy the vertex data into the upload buffer
		void* pBufferData = nullptr;
		if (d3dUploadBuffer->Map(0, nullptr, &pBufferData) < 0) return false;
		memcpy(pBufferData, Vertices, 48);
		d3dUploadBuffer->Unmap(0, nullptr);

		//copy the data from the upload buffer to the vertex buffer
		d3dUploadCommandList->CopyBufferRegion(m_d3dVertexBuffer, 0, d3dUploadBuffer, 0, 48);

		//make a resource barrier
		D3D12_RESOURCE_BARRIER d3dResourceBarrier[1] = {};
		d3dResourceBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dResourceBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		d3dResourceBarrier[0].Transition.pResource = m_d3dVertexBuffer;
		d3dResourceBarrier[0].Transition.Subresource = 0; // we only have one subresource
		d3dResourceBarrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		d3dResourceBarrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		d3dUploadCommandList->ResourceBarrier(1, d3dResourceBarrier);


		//create the UAV
		//create the descriptor heap for the UAVs
		D3D12_DESCRIPTOR_HEAP_DESC d3dUAVDescriptorHeapDesc{};
		d3dUAVDescriptorHeapDesc.NodeMask = 0;
		d3dUAVDescriptorHeapDesc.NumDescriptors = 1;
		d3dUAVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		d3dUAVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (d3dDevice->CreateDescriptorHeap(&d3dUAVDescriptorHeapDesc, IID_PPV_ARGS(&m_d3dUAVDescriptorHeap)) < 0) return false;

		//the resource
		D3D12_HEAP_PROPERTIES d3dUAVHeapProperties{};
		d3dUAVHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		d3dUAVHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		d3dUAVHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		d3dUAVHeapProperties.CreationNodeMask = 1;
		d3dUAVHeapProperties.VisibleNodeMask = 1;
		D3D12_RESOURCE_DESC1 d3dUAVBufferDesc{};
		d3dUAVBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		d3dUAVBufferDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		d3dUAVBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		d3dUAVBufferDesc.Alignment = 0;
		d3dUAVBufferDesc.Width = 1920;
		d3dUAVBufferDesc.Height = 1080;
		d3dUAVBufferDesc.DepthOrArraySize = 1;
		d3dUAVBufferDesc.MipLevels = 1;
		d3dUAVBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		d3dUAVBufferDesc.SampleDesc.Count = 1;
		d3dUAVBufferDesc.SampleDesc.Quality = 0;
		d3dUAVBufferDesc.SamplerFeedbackMipRegion.Width = 0;
		d3dUAVBufferDesc.SamplerFeedbackMipRegion.Height = 0;
		d3dUAVBufferDesc.SamplerFeedbackMipRegion.Depth = 0;
		D3D12_UNORDERED_ACCESS_VIEW_DESC d3dUAVDesc{};
		d3dUAVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		d3dUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		d3dUAVDesc.Texture2D.MipSlice = 0;
		d3dUAVDesc.Texture2D.PlaneSlice = 0;

		if (d3dDevice->CreateCommittedResource2(&d3dUAVHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dUAVBufferDesc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, nullptr, IID_PPV_ARGS(&m_d3dIntersectionTarget)) < 0) return false;
		//create the resource view
		d3dDevice->CreateUnorderedAccessView(m_d3dIntersectionTarget, nullptr, &d3dUAVDesc, m_d3dUAVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

		//execute the uploading
		if (d3dUploadCommandList->Close() < 0) return false;
		ID3D12CommandList* d3dCommandLists[] = { d3dUploadCommandList };
		d3dCommandQueue->ExecuteCommandLists(1, d3dCommandLists);
		//wait for the GPU to finish
		if (d3dCommandQueue->Signal(d3dUploadFence, 1) < 0) return false;
		WaitForFence(d3dUploadFence, 1);

		//quick cleanup
		if (d3dUploadCommandAllocator->Reset() < 0) return false;

		return true;
	}


	//render a single frame
	bool Raytracer::Render()
	{
		ID3D12Device8* d3dDevice = m_rtDevice->GetDevice();
		ID3D12CommandQueue* d3dCommandQueue = m_rtDevice->GetCommandQueue();
		IDXGISwapChain4* dxSwapChain = m_rtDevice->GetSwapChain();

		if (m_d3dComputeCommandAllocator->Reset() < 0) return false;
		if (m_d3dCommandList->Reset(m_d3dComputeCommandAllocator, nullptr) < 0) return false;


		//execute the compute shader
		//set the root signature, the root descriptor table and the pipeline state
		m_d3dCommandList->SetComputeRootSignature(m_d3dComputeRootSignature);
		m_d3dCommandList->SetDescriptorHeaps(1, &m_d3dUAVDescriptorHeap);
		m_d3dCommandList->SetComputeRootDescriptorTable(0, m_d3dUAVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		m_d3dCommandList->SetPipelineState(m_d3dComputePipelineState);

		//make a resource barrier
		D3D12_RESOURCE_BARRIER d3dUAVBarrier[1] = {};
		d3dUAVBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarrier[0].UAV.pResource = m_d3dIntersectionTarget;
		m_d3dCommandList->ResourceBarrier(1, d3dUAVBarrier);

		//dispatch the workload
		m_d3dCommandList->Dispatch((1920 - 1) / 16 + 1, (1080 - 1) / 16 + 1, 1);

		//make a resource barrier
		d3dUAVBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dUAVBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		d3dUAVBarrier[0].UAV.pResource = m_d3dIntersectionTarget;
		m_d3dCommandList->ResourceBarrier(1, d3dUAVBarrier);

		//we reached the end of the workload --> close the command list and execute it
		if (m_d3dCommandList->Close() < 0) return false;
		ID3D12CommandList* d3dComputeCommandLists[] = { m_d3dCommandList };
		d3dCommandQueue->ExecuteCommandLists(1, d3dComputeCommandLists);

		//make a signal after this workload is finished
		m_iComputeFenceValue++;
		if (d3dCommandQueue->Signal(m_d3dComputeFence, m_iComputeFenceValue) < 0) return false;

		//wait for the compute work to finish
		WaitForFence(m_d3dComputeFence, m_iComputeFenceValue);


		//wait for the previous frame this command list is executing, to finish
		WaitForFence(m_d3dFrameFences[m_iCurrentQueueSlot], m_iFenceValues[m_iCurrentQueueSlot]);

		if (m_d3dCommandAllocators[m_iCurrentQueueSlot]->Reset() < 0) return false;
		if (m_d3dCommandList->Reset(m_d3dCommandAllocators[m_iCurrentQueueSlot], nullptr) < 0) return false;

		//render a frame
		//set the different components of the pipeline
		D3D12_CPU_DESCRIPTOR_HANDLE d3dRTVDescriptor = m_d3dRTVsDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		d3dRTVDescriptor.ptr += (SIZE_T)(m_iCurrentQueueSlot * d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

		m_d3dCommandList->RSSetScissorRects(1, &m_d3dScissorRect);
		m_d3dCommandList->RSSetViewports(1, &m_d3dViewport);
		//m_d3dCommandList->OMSetDepthBounds(0.0f, 1.0f);
		//m_d3dCommandList->OMSetStencilRef(0);
		//m_d3dCommandList->OMSetBlendFactor();
		m_d3dCommandList->OMSetRenderTargets(1, &d3dRTVDescriptor, true, nullptr);

		//make the resource transition of the RT from the present state to the render target state
		D3D12_RESOURCE_BARRIER d3dResourceBarrier[1] = {};
		d3dResourceBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dResourceBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		d3dResourceBarrier[0].Transition.pResource = m_d3dBackBuffers[m_iCurrentQueueSlot];
		d3dResourceBarrier[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		d3dResourceBarrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		d3dResourceBarrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		m_d3dCommandList->ResourceBarrier(1, d3dResourceBarrier);

		//clear the RTV
		float fClearColor[4] = { 0.1f, 0.3f, 0.9f, 1.0f };
		m_d3dCommandList->ClearRenderTargetView(d3dRTVDescriptor, fClearColor, 0, nullptr);

		//set the root signature, the root descriptor table and the pipeline state
		m_d3dCommandList->SetGraphicsRootSignature(m_d3dRootSignature);
		m_d3dCommandList->SetDescriptorHeaps(1, &m_d3dUAVDescriptorHeap);
		m_d3dCommandList->SetGraphicsRootDescriptorTable(1, m_d3dUAVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		m_d3dCommandList->SetPipelineState(m_d3dPipelineState);

		m_d3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_d3dCommandList->IASetVertexBuffers(0, 1, &m_d3dVertexBufferView);
		//m_d3dCommandList->IASetIndexBuffer();
		m_d3dCommandList->DrawInstanced(3, 1, 0, 0);

		//a second resource barrier for the RT to get back to its present state
		d3dResourceBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dResourceBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		d3dResourceBarrier[0].Transition.pResource = m_d3dBackBuffers[m_iCurrentQueueSlot];
		d3dResourceBarrier[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		d3dResourceBarrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		d3dResourceBarrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		m_d3dCommandList->ResourceBarrier(1, d3dResourceBarrier);

		//we reached the end of the frame --> close the command list and execute it
		if (m_d3dCommandList->Close() < 0) return false;
		ID3D12CommandList* d3dCommandLists[] = { m_d3dCommandList };
		d3dCommandQueue->ExecuteCommandLists(1, d3dCommandLists);
		

		//present the frame
		if (dxSwapChain->Present(1, 0) < 0) return false; //amdxc.dll throws memory access violation error, when compiled with the wrong architecture

		//make a signal after this frame is finished
		m_iCurrentFenceValue++;
		m_iFenceValues[m_iCurrentQueueSlot] = m_iCurrentFenceValue;
		if (d3dCommandQueue->Signal(m_d3dFrameFences[m_iCurrentQueueSlot], m_iFenceValues[m_iCurrentQueueSlot]) < 0) return false;

		//go on to the next buffer
		m_iCurrentQueueSlot = (m_iCurrentQueueSlot + 1) % s_iNumQueueSlots;


		return true;
	}

}