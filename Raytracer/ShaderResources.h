#pragma once


#include <bit> //for std::popcount
#include "DXGIFormatHelper.h" //for DirectX::BitsPerPixel
#include "GPUScheduler.h"


namespace RT::GraphicsAPI
{
	enum ShaderStage : uint8_t
	{
		ShaderStageNone = 0x00,
		ShaderStageVS = 0x01, // Vertex Shader
		ShaderStageHS = 0x02, // Hull Shader
		ShaderStageDS = 0x04, // Domain Shader
		ShaderStageGS = 0x08, // Geometry Shader
		ShaderStagePS = 0x10, // Pixel Shader
		ShaderStageAS = 0x20, // Amplification Shader
		ShaderStageMS = 0x40, // Mesh Shader
		ShaderStageCS = 0xff, // Compute Shader
		ShaderStageAll = 0xff // all shader stages
	};

	enum class SamplerFilter : uint8_t
	{
		Point = 0, // nearest neighbor for texels and mipmaps
		LinearMipMap = 1, // nearest neighbor for texels, linear interpolation for mipmaps
		Bilinear = 2, // linear interpolation for texels, closest mipmap
		Trilinear = 3, // linear interpolation for texels and mipmaps
		Anisotropic = 4
	};

#define D3D12_RESOURCE_STATE_SHADER_RESOURCE (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)
	
	

	class DescriptorTable
	{
	private:

		//the data of the descriptor table
		D3D12_DESCRIPTOR_RANGE1* m_d3dDescriptorRanges;
		UINT m_iNumDescriptorRanges;


	public:

		DescriptorTable() : m_d3dDescriptorRanges(nullptr), m_iNumDescriptorRanges(0) {};
		~DescriptorTable()
		{
			Release();
		};

		bool AddRange(const D3D12_DESCRIPTOR_RANGE1& d3dRange)
		{

			D3D12_DESCRIPTOR_RANGE1* d3dNewDescriptorRanges = new D3D12_DESCRIPTOR_RANGE1[m_iNumDescriptorRanges + 1];
			if (!d3dNewDescriptorRanges) return false;
			memcpy(d3dNewDescriptorRanges, m_d3dDescriptorRanges, sizeof(D3D12_DESCRIPTOR_RANGE1) * m_iNumDescriptorRanges);
			
			delete m_d3dDescriptorRanges;
			m_d3dDescriptorRanges = d3dNewDescriptorRanges;

			m_d3dDescriptorRanges[m_iNumDescriptorRanges] = d3dRange;
			m_iNumDescriptorRanges++;

			return true;
		}

		bool AddCBVRange(UINT iBaseRegister, UINT iRegisterSpace, UINT iNumDescriptors = -1)
		{
			D3D12_DESCRIPTOR_RANGE1 d3dRange{};
			d3dRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
			d3dRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			d3dRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			d3dRange.NumDescriptors = iNumDescriptors;
			d3dRange.BaseShaderRegister = iBaseRegister;
			d3dRange.RegisterSpace = iRegisterSpace;

			return AddRange(d3dRange);
		}

		bool AddSRVRange(UINT iBaseRegister, UINT iRegisterSpace, UINT iNumDescriptors = -1)
		{
			D3D12_DESCRIPTOR_RANGE1 d3dRange{};
			d3dRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
			d3dRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			d3dRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			d3dRange.NumDescriptors = iNumDescriptors;
			d3dRange.BaseShaderRegister = iBaseRegister;
			d3dRange.RegisterSpace = iRegisterSpace;

			return AddRange(d3dRange);
		}

		bool AddUAVRange(UINT iBaseRegister, UINT iRegisterSpace, UINT iNumDescriptors = -1)
		{
			D3D12_DESCRIPTOR_RANGE1 d3dRange{};
			d3dRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
			d3dRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			d3dRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			d3dRange.NumDescriptors = iNumDescriptors;
			d3dRange.BaseShaderRegister = iBaseRegister;
			d3dRange.RegisterSpace = iRegisterSpace;

			return AddRange(d3dRange);
		}

		bool AddSamplerRange(UINT iBaseRegister, UINT iRegisterSpace, UINT iNumDescriptors = -1)
		{
			D3D12_DESCRIPTOR_RANGE1 d3dRange{};
			d3dRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
			d3dRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			d3dRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			d3dRange.NumDescriptors = iNumDescriptors;
			d3dRange.BaseShaderRegister = iBaseRegister;
			d3dRange.RegisterSpace = iRegisterSpace;

			return AddRange(d3dRange);
		}

		void Release()
		{
			if (m_d3dDescriptorRanges) delete m_d3dDescriptorRanges;
			m_d3dDescriptorRanges = nullptr;
			m_iNumDescriptorRanges = 0;
		}


		//helper functions
		D3D12_DESCRIPTOR_RANGE1* GetRanges() { return m_d3dDescriptorRanges; };
		UINT GetNumRanges() { return m_iNumDescriptorRanges; };
		
	};



	class RootSignature
	{
	private:

		//the data of the signature
		D3D12_ROOT_PARAMETER1*		m_d3dRootParameters;
		D3D12_STATIC_SAMPLER_DESC*	m_d3dStaticSamplers;
		UINT m_iNumRootParameters;
		UINT m_iNumStaticSamplers;
		uint8_t m_rtUsedStages;

		D3D12_FILTER GetFilterType(SamplerFilter rtFilterType)
		{
			switch (rtFilterType)
			{
			case SamplerFilter::Point:
				return D3D12_FILTER_MIN_MAG_MIP_POINT;
			case SamplerFilter::LinearMipMap:
				return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
			case SamplerFilter::Bilinear:
				return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			case SamplerFilter::Trilinear:
				return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			case SamplerFilter::Anisotropic:
				return D3D12_FILTER_ANISOTROPIC;
			default:
				return D3D12_FILTER_MIN_MAG_MIP_POINT;
			}
		}
		
		ShaderStage GetVisibleStage(D3D12_SHADER_VISIBILITY d3dShaderVisibility)
		{
			switch (d3dShaderVisibility)
			{
			case D3D12_SHADER_VISIBILITY_ALL:
				return ShaderStageAll;
			case D3D12_SHADER_VISIBILITY_VERTEX:
				return ShaderStageVS;
			case D3D12_SHADER_VISIBILITY_HULL:
				return ShaderStageHS;
			case D3D12_SHADER_VISIBILITY_DOMAIN:
				return ShaderStageDS;
			case D3D12_SHADER_VISIBILITY_GEOMETRY:
				return ShaderStageGS;
			case D3D12_SHADER_VISIBILITY_PIXEL:
				return ShaderStagePS;
			case D3D12_SHADER_VISIBILITY_AMPLIFICATION:
				return ShaderStageAS;
			case D3D12_SHADER_VISIBILITY_MESH:
				return ShaderStageMS;
			default:
				return ShaderStageNone;
			}
		}

		D3D12_SHADER_VISIBILITY GetShaderVisibility(ShaderStage rtVisibleStages)
		{
			//std::popcount counts the number of set bits
			if (std::popcount((uint8_t)rtVisibleStages) > 1)
			{
				return D3D12_SHADER_VISIBILITY_ALL;
			}
			else
			{
				switch (rtVisibleStages)
				{
				case ShaderStageVS:
					return D3D12_SHADER_VISIBILITY_VERTEX;
				case ShaderStageHS:
					return D3D12_SHADER_VISIBILITY_HULL;
				case ShaderStageDS:
					return D3D12_SHADER_VISIBILITY_DOMAIN;
				case ShaderStageGS:
					return D3D12_SHADER_VISIBILITY_GEOMETRY;
				case ShaderStagePS:
					return D3D12_SHADER_VISIBILITY_PIXEL;
				case ShaderStageAS:
					return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
				case ShaderStageMS:
					return D3D12_SHADER_VISIBILITY_MESH;
				default:
					return D3D12_SHADER_VISIBILITY_ALL;
				}
			}
		}

		D3D12_ROOT_SIGNATURE_FLAGS GetRootSignatureFlags(bool bAllowInputLayout = false, bool bAllowStreamOutput = false)
		{
			D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = (D3D12_ROOT_SIGNATURE_FLAGS)0;
			if (~m_rtUsedStages)
			{
				m_rtUsedStages = ~m_rtUsedStages;
				if (m_rtUsedStages & ShaderStageVS) d3dRootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
				if (m_rtUsedStages & ShaderStageHS) d3dRootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
				if (m_rtUsedStages & ShaderStageDS) d3dRootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
				if (m_rtUsedStages & ShaderStageGS) d3dRootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
				if (m_rtUsedStages & ShaderStagePS) d3dRootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
				if (m_rtUsedStages & ShaderStageAS) d3dRootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
				if (m_rtUsedStages & ShaderStageMS) d3dRootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
				m_rtUsedStages = ~m_rtUsedStages;
			}
			if (bAllowInputLayout) d3dRootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			if (bAllowStreamOutput) d3dRootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;

			return d3dRootSignatureFlags;
		}


	public:

		RootSignature() :
			m_d3dRootParameters(nullptr),
			m_d3dStaticSamplers(nullptr),
			m_iNumRootParameters(0),
			m_iNumStaticSamplers(0),
			m_rtUsedStages(ShaderStageNone) {};
		~RootSignature()
		{
			Release();
		};

		bool AddRootParameter(const D3D12_ROOT_PARAMETER1& d3dRootParameter)
		{
			D3D12_ROOT_PARAMETER1* d3dNewRootParameters = new D3D12_ROOT_PARAMETER1[m_iNumRootParameters + 1];
			if (!d3dNewRootParameters) return false;
			memcpy(d3dNewRootParameters, m_d3dRootParameters, sizeof(D3D12_ROOT_PARAMETER1) * m_iNumRootParameters);

			delete m_d3dRootParameters;
			m_d3dRootParameters = d3dNewRootParameters;

			m_rtUsedStages |= GetVisibleStage(d3dRootParameter.ShaderVisibility);
			m_d3dRootParameters[m_iNumRootParameters] = d3dRootParameter;
			m_iNumRootParameters++;

			return true;
		}

		bool AddRootConstant(UINT iRegister, UINT iSpace, UINT iNum32bitValues, ShaderStage rtVisibleStages = ShaderStageAll)
		{
			D3D12_ROOT_PARAMETER1 d3dRootParameter{};
			d3dRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			d3dRootParameter.ShaderVisibility = GetShaderVisibility(rtVisibleStages);
			d3dRootParameter.Constants.ShaderRegister = iRegister;
			d3dRootParameter.Constants.RegisterSpace = iSpace;
			d3dRootParameter.Constants.Num32BitValues = iNum32bitValues;
			
			return AddRootParameter(d3dRootParameter);
		}

		bool AddConstantBuffer(UINT iRegister, UINT iSpace, ShaderStage rtVisibleStages = ShaderStageAll)
		{
			D3D12_ROOT_PARAMETER1 d3dRootParameter{};
			d3dRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			d3dRootParameter.ShaderVisibility = GetShaderVisibility(rtVisibleStages);
			d3dRootParameter.Descriptor.ShaderRegister = iRegister;
			d3dRootParameter.Descriptor.RegisterSpace = iSpace;
			d3dRootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

			return AddRootParameter(d3dRootParameter);
		}

		bool AddShaderResource(UINT iRegister, UINT iSpace, ShaderStage rtVisibleStages = ShaderStageAll)
		{
			D3D12_ROOT_PARAMETER1 d3dRootParameter{};
			d3dRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
			d3dRootParameter.ShaderVisibility = GetShaderVisibility(rtVisibleStages);
			d3dRootParameter.Descriptor.ShaderRegister = iRegister;
			d3dRootParameter.Descriptor.RegisterSpace = iSpace;
			d3dRootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

			return AddRootParameter(d3dRootParameter);
		}

		bool AddUnorderedAccessResource(UINT iRegister, UINT iSpace, ShaderStage rtVisibleStages = ShaderStageAll)
		{
			D3D12_ROOT_PARAMETER1 d3dRootParameter{};
			d3dRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
			d3dRootParameter.ShaderVisibility = GetShaderVisibility(rtVisibleStages);
			d3dRootParameter.Descriptor.ShaderRegister = iRegister;
			d3dRootParameter.Descriptor.RegisterSpace = iSpace;
			d3dRootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;

			return AddRootParameter(d3dRootParameter);
		}

		bool AddDescriptorTable(UINT iNumDescriptorRanges, D3D12_DESCRIPTOR_RANGE1* d3dDescriptorRanges, ShaderStage rtVisibleStages = ShaderStageAll)
		{
			D3D12_ROOT_PARAMETER1 d3dRootParameter{};
			d3dRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			d3dRootParameter.ShaderVisibility = GetShaderVisibility(rtVisibleStages);
			d3dRootParameter.DescriptorTable.NumDescriptorRanges = iNumDescriptorRanges;
			d3dRootParameter.DescriptorTable.pDescriptorRanges = d3dDescriptorRanges;

			return AddRootParameter(d3dRootParameter);
		}

		bool AddDescriptorTable(DescriptorTable& rtDescriptorTable, ShaderStage rtVisibleStages = ShaderStageAll)
		{
			return AddDescriptorTable(rtDescriptorTable.GetNumRanges(), rtDescriptorTable.GetRanges(), rtVisibleStages);
		}

		bool AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& d3dSamplerDesc)
		{
			D3D12_STATIC_SAMPLER_DESC* d3dNewStaticSamplers = new D3D12_STATIC_SAMPLER_DESC[m_iNumStaticSamplers + 1];
			if (!d3dNewStaticSamplers) return false;
			memcpy(d3dNewStaticSamplers, m_d3dRootParameters, sizeof(D3D12_STATIC_SAMPLER_DESC) * m_iNumStaticSamplers);

			delete m_d3dStaticSamplers;
			m_d3dStaticSamplers = d3dNewStaticSamplers;

			m_rtUsedStages |= GetVisibleStage(d3dSamplerDesc.ShaderVisibility);
			m_d3dStaticSamplers[m_iNumStaticSamplers] = d3dSamplerDesc;
			m_iNumStaticSamplers++;

			return true;
		}

		bool AddStaticSampler(UINT iRegister, UINT iSpace, SamplerFilter rtFilterType, UINT iMaxAnisotropy, D3D12_TEXTURE_ADDRESS_MODE d3dAddressMode,
			FLOAT fLODBias = 0, FLOAT fMinLOD = 0, FLOAT fMaxLOD = D3D12_FLOAT32_MAX, ShaderStage rtVisibleStages = ShaderStageAll)
		{
			D3D12_STATIC_SAMPLER_DESC d3dStaticSampler{};
			d3dStaticSampler.Filter = GetFilterType(rtFilterType);
			d3dStaticSampler.MaxAnisotropy = iMaxAnisotropy;
			d3dStaticSampler.MinLOD = fMinLOD;
			d3dStaticSampler.MaxLOD = fMaxLOD;
			d3dStaticSampler.MipLODBias = fLODBias;
			d3dStaticSampler.AddressU = d3dAddressMode;
			d3dStaticSampler.AddressV = d3dAddressMode;
			d3dStaticSampler.AddressW = d3dAddressMode;
			d3dStaticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			d3dStaticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			d3dStaticSampler.ShaderRegister = iRegister;
			d3dStaticSampler.RegisterSpace = iSpace;
			d3dStaticSampler.ShaderVisibility = GetShaderVisibility(rtVisibleStages);
			
			return AddStaticSampler(d3dStaticSampler);
		}

		void Release()
		{
			if (m_d3dRootParameters) delete m_d3dRootParameters;
			if (m_d3dStaticSamplers) delete m_d3dStaticSamplers;
			m_d3dRootParameters = nullptr;
			m_d3dStaticSamplers = nullptr;
			m_iNumRootParameters = 0;
			m_iNumStaticSamplers = 0;
		}

		bool GenerateRootSignature(ID3D12Device8* d3dDevice, ID3D12RootSignature** d3dRootSignature = nullptr,
			bool bAllowInputLayout = false, bool bAllowStreamOutput = false)
		{
			if (d3dRootSignature)
			{
				//generate a dx12 root signature
				*d3dRootSignature = nullptr;
				D3D12_VERSIONED_ROOT_SIGNATURE_DESC d3dVersionedSignatureDesc{};
				d3dVersionedSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
				d3dVersionedSignatureDesc.Desc_1_1.Flags = GetRootSignatureFlags(bAllowInputLayout, bAllowStreamOutput);
				d3dVersionedSignatureDesc.Desc_1_1.NumParameters = m_iNumRootParameters;
				d3dVersionedSignatureDesc.Desc_1_1.pParameters = m_d3dRootParameters;
				d3dVersionedSignatureDesc.Desc_1_1.NumStaticSamplers = m_iNumStaticSamplers;
				d3dVersionedSignatureDesc.Desc_1_1.pStaticSamplers = m_d3dStaticSamplers;
				ID3DBlob* d3dSignatureBlob = nullptr;
				ID3DBlob* d3dErrorBlob = nullptr;
				if (D3D12SerializeVersionedRootSignature(&d3dVersionedSignatureDesc, &d3dSignatureBlob, &d3dErrorBlob) < 0) return false;
				if ((!d3dSignatureBlob) || (d3dErrorBlob)) return false;
				if (d3dDevice->CreateRootSignature(0, d3dSignatureBlob->GetBufferPointer(), d3dSignatureBlob->GetBufferSize(),
					IID_PPV_ARGS(d3dRootSignature)) < 0) return false;

				//clean up
				Release();
			}

			return true;
		}

	};



	class ResourceHeap
	{
	private:

		//private member variables
		DX12Device* m_rtDevice;
		ID3D12Heap1* m_d3dHeap;


	public:

		ResourceHeap() : m_rtDevice(nullptr), m_d3dHeap(nullptr) {};
		~ResourceHeap()
		{
			Release();
		};

		//public functions
		bool Initialize(DX12Device* rtDevice, UINT64 iHeapSize, UINT64 iResourceAlignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
			D3D12_HEAP_TYPE d3dHeapType = D3D12_HEAP_TYPE_DEFAULT, bool bBufferHeap = true)
		{
			m_rtDevice = rtDevice;

			D3D12_HEAP_PROPERTIES d3dHeapProperties{};
			d3dHeapProperties.Type = d3dHeapType;
			d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			d3dHeapProperties.CreationNodeMask = 0;
			d3dHeapProperties.VisibleNodeMask = 0;
			D3D12_HEAP_DESC d3dHeapDesc{};
			d3dHeapDesc.SizeInBytes = iHeapSize;
			d3dHeapDesc.Alignment = iResourceAlignment;
			d3dHeapDesc.Properties = d3dHeapProperties;
			d3dHeapDesc.Flags = (bBufferHeap ? D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES : D3D12_HEAP_FLAG_DENY_BUFFERS) | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES;
			if (m_rtDevice->GetDevice()->CreateHeap1(&d3dHeapDesc, nullptr, IID_PPV_ARGS(&m_d3dHeap)) < 0) return false;

			return true;
		}

		bool CreateResource(const D3D12_RESOURCE_DESC1* d3dResourceDesc, ID3D12Resource2** d3dResource,
			D3D12_RESOURCE_STATES d3dInitialState = D3D12_RESOURCE_STATE_COMMON)
		{
			if (m_rtDevice->GetDevice()->CreatePlacedResource1(m_d3dHeap, 0, d3dResourceDesc,
				d3dInitialState, nullptr, IID_PPV_ARGS(d3dResource)) < 0) return false;
			
			return true;
		}

		void Release()
		{
			if (m_d3dHeap) m_d3dHeap->Release();
			m_d3dHeap = nullptr;
		}


		//helper functions
		ID3D12Heap1* GetHeap() { return m_d3dHeap; };

	};



	class DescriptorHeap
	{
	private:

		//private member variables
		GPUScheduler* m_rtScheduler;
		ID3D12DescriptorHeap** m_d3dDescriptorHeap;
		unsigned int m_iDescriptorHeapIncrementSize;
		unsigned int m_iNumDescriptorHeaps;


	public:

		DescriptorHeap() : m_rtScheduler(nullptr), m_d3dDescriptorHeap(nullptr), m_iDescriptorHeapIncrementSize(0), m_iNumDescriptorHeaps(0) {};
		~DescriptorHeap()
		{
			Release();
		};

		//public functions
		bool Initialize(GPUScheduler* rtScheduler, UINT iNumDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE d3dDescriptorHeapType, bool bIsShaderVisible = true)
		{
			m_rtScheduler = rtScheduler;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iDescriptorHeapIncrementSize = d3dDevice->GetDescriptorHandleIncrementSize(d3dDescriptorHeapType);
			m_iNumDescriptorHeaps = m_rtScheduler->GetNumMaxTasks();
			m_d3dDescriptorHeap = new ID3D12DescriptorHeap*[m_iNumDescriptorHeaps];

			D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc{};
			d3dDescriptorHeapDesc.NodeMask = 0;
			d3dDescriptorHeapDesc.NumDescriptors = iNumDescriptors;
			d3dDescriptorHeapDesc.Type = d3dDescriptorHeapType;
			d3dDescriptorHeapDesc.Flags = bIsShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			for (unsigned int i = 0; i < m_iNumDescriptorHeaps; i++)
			{
				if (d3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, IID_PPV_ARGS(m_d3dDescriptorHeap + i)) < 0) return false;
			}

			return true;
		}

		void Bind(UINT iRootParameterIndex, UINT64 iOffsetInDescriptor = 0, bool bBindToCS = false, bool bSetWholeHeap = true)
		{
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			D3D12_GPU_DESCRIPTOR_HANDLE d3dDescriptorHandle = m_d3dDescriptorHeap[iIndex]->GetGPUDescriptorHandleForHeapStart();
			d3dDescriptorHandle.ptr += iOffsetInDescriptor * (UINT64)m_iDescriptorHeapIncrementSize;
			if (bSetWholeHeap) d3dCommandList->SetDescriptorHeaps(1, m_d3dDescriptorHeap + iIndex);
			if (bBindToCS)
			{
				d3dCommandList->SetComputeRootDescriptorTable(iRootParameterIndex, d3dDescriptorHandle);
			}
			else
			{
				d3dCommandList->SetGraphicsRootDescriptorTable(iRootParameterIndex, d3dDescriptorHandle);
			}
		}

		void Release()
		{
			if (m_d3dDescriptorHeap)
			{
				for (unsigned int i = 0; i < m_iNumDescriptorHeaps; i++)
				{
					if (m_d3dDescriptorHeap[i]) m_d3dDescriptorHeap[i]->Release();
				}

				delete m_d3dDescriptorHeap;
				m_d3dDescriptorHeap = nullptr;
			}

			m_iNumDescriptorHeaps = 0;
		}


		//helper functions
		ID3D12DescriptorHeap** GetDescriptorHeap() { return m_d3dDescriptorHeap; };
		unsigned int GetIncrementSize() { return m_iDescriptorHeapIncrementSize; };

	};



	struct ResourceHeapInfo
	{
		ID3D12Heap1* d3dResourceHeap;
		UINT64 iOffsetInHeap;

		ResourceHeapInfo(ID3D12Heap1* d3dHeap = nullptr, UINT64 iOffsetInResourceHeap = 0) :
			d3dResourceHeap(d3dHeap), iOffsetInHeap(iOffsetInResourceHeap) {};
		ResourceHeapInfo(ResourceHeap* rtResourceHeap, UINT64 iOffsetInResourceHeap = 0) :
			d3dResourceHeap(rtResourceHeap->GetHeap()), iOffsetInHeap(iOffsetInResourceHeap) {};
		ResourceHeapInfo(const ResourceHeapInfo& rtOtherHeapInfo) :
			d3dResourceHeap(rtOtherHeapInfo.d3dResourceHeap), iOffsetInHeap(rtOtherHeapInfo.iOffsetInHeap) {};
		~ResourceHeapInfo() {};
	};



	struct DescriptorHeapInfo
	{
		ID3D12DescriptorHeap** d3dDescriptorHeap;
		unsigned int iOffsetInDescriptor;

		DescriptorHeapInfo(ID3D12DescriptorHeap** d3dDescriptorHeaps = nullptr, unsigned int iOffsetInDescriptorHeaps = 0) :
			d3dDescriptorHeap(d3dDescriptorHeaps), iOffsetInDescriptor(iOffsetInDescriptorHeaps) {};
		DescriptorHeapInfo(DescriptorHeap* rtDescriptorHeap, unsigned int iOffsetInDescriptorHeaps = 0) :
			d3dDescriptorHeap(rtDescriptorHeap->GetDescriptorHeap()), iOffsetInDescriptor(iOffsetInDescriptorHeaps) {};
		DescriptorHeapInfo(const DescriptorHeapInfo& rtOtherDescriptorInfo) :
			d3dDescriptorHeap(rtOtherDescriptorInfo.d3dDescriptorHeap), iOffsetInDescriptor(rtOtherDescriptorInfo.iOffsetInDescriptor) {};
		~DescriptorHeapInfo() {};
	};



	class BaseShaderResource
	{
	protected:

		//private members
		GPUScheduler* m_rtScheduler;
		ID3D12Resource2** m_d3dResource;
		D3D12_RESOURCE_DESC1 m_d3dResourceDesc;
		bool m_bResourceInDescriptorTable;


	public:

		BaseShaderResource() :
			m_rtScheduler(nullptr),
			m_d3dResource(nullptr),
			m_d3dResourceDesc(),
			m_bResourceInDescriptorTable(false) {};
		virtual ~BaseShaderResource() {};


		//helper functions
		D3D12_RESOURCE_ALLOCATION_INFO1 GetAllocationInfo() {
			if (!m_rtScheduler) return (D3D12_RESOURCE_ALLOCATION_INFO1)0;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			D3D12_RESOURCE_ALLOCATION_INFO1 d3dAllocationInfo{};
			d3dDevice->GetResourceAllocationInfo2(0, 1, &m_d3dResourceDesc, &d3dAllocationInfo);
			return d3dAllocationInfo; };
		ID3D12Resource2** GetResources() { return m_d3dResource; };
		ID3D12Resource2* GetResource() { return m_d3dResource[m_rtScheduler->GetCurrentTaskIndex()]; };
		D3D12_RESOURCE_DESC1* GetResourceDesc() { return &m_d3dResourceDesc; };
		UINT64 GetWidth() { return m_d3dResourceDesc.Width; };
		UINT64 GetHeight() { return (UINT64)(m_d3dResourceDesc.Height); };
		UINT16 GetDepthOrArraySize() { return m_d3dResourceDesc.DepthOrArraySize; };
		UINT16 GetMipLevels() { return m_d3dResourceDesc.MipLevels; };
		UINT64 GetResourceSize() { return GetWidth() * (UINT64)GetHeight() * (UINT64)GetDepthOrArraySize() * (UINT64)GetMipLevels(); };
		DXGI_FORMAT GetFormat() { return m_d3dResourceDesc.Format; };
		UINT64 GetAlignement() { return m_d3dResourceDesc.Alignment; };
		bool IsResourceInDescriptorTable() { return m_bResourceInDescriptorTable; };

	};



	class ShaderResources
	{
	private:

		//private members
		RootSignature*	m_rtRootSignature;
		ResourceHeap*	m_rtHeap;


	public:

		ShaderResources() : m_rtRootSignature(nullptr), m_rtHeap(nullptr) {};
		~ShaderResources() {};


		//public functions
		bool Initialize()
		{
			return false;
		}
	};


	
	class UploadBuffer : public BaseShaderResource
	{
	private:

		//the data of the buffer
		void** m_pBufferDataPointer;
		unsigned int m_iNumBuffers;
		

	public:

		UploadBuffer() :
			BaseShaderResource(),
			m_pBufferDataPointer(nullptr),
			m_iNumBuffers(0) {};
		~UploadBuffer()
		{
			Release();
		};

		bool Initialize(GPUScheduler* rtScheduler, unsigned int iBufferSize)
		{
			m_rtScheduler = rtScheduler;
			if (!m_rtScheduler) return false;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iNumBuffers = m_rtScheduler->GetNumMaxTasks();
			if (m_iNumBuffers < 1) return false;
			m_d3dResource = new ID3D12Resource2*[m_iNumBuffers];
			if (!m_d3dResource) return false;
			m_pBufferDataPointer = new void*[m_iNumBuffers];
			if (!m_pBufferDataPointer) return false;

			m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			m_d3dResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			m_d3dResourceDesc.Width = (UINT64)iBufferSize;
			m_d3dResourceDesc.Height = 1;
			m_d3dResourceDesc.DepthOrArraySize = 1;
			m_d3dResourceDesc.MipLevels = 1;
			m_d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // a requirement for buffers
			m_d3dResourceDesc.SampleDesc.Count = 1;
			m_d3dResourceDesc.SampleDesc.Quality = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

			//create the upload buffer
			D3D12_HEAP_PROPERTIES d3dHeapProperties{};
			d3dHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			d3dHeapProperties.CreationNodeMask = 0;
			d3dHeapProperties.VisibleNodeMask = 0;

			for (unsigned int i = 0; i < m_iNumBuffers; i++)
			{
				if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				D3D12_RANGE d3dReadRange = { 0, 0 }; //we don't CPU-Read-Access to this resource
				if (m_d3dResource[i]->Map(0, &d3dReadRange, m_pBufferDataPointer + i) < 0) return false;
			}

			return true;
		};

		bool Update(const void* pBufferData, unsigned int iNumBytes = 0, unsigned int iOffset = 0)
		{
			if (iNumBytes == 0) iNumBytes = (unsigned int)GetResourceSize();
			if (iOffset >= GetResourceSize()) return false;
			if ((UINT64)iNumBytes + (UINT64)iOffset > GetResourceSize()) return false;
			memcpy((byte*)(m_pBufferDataPointer[m_rtScheduler->GetCurrentTaskIndex()]) + iOffset, pBufferData, iNumBytes);

			return true;
		}

		bool Upload(ID3D12Resource2* d3dDestinationResource, UINT64 iNumBytes, UINT64 iSourceOffset, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			if ((iSourceOffset >= GetResourceSize()) || (iNumBytes > GetResourceSize())) return false;
			d3dCommandList->CopyBufferRegion(d3dDestinationResource, iDestinationOffset,
				m_d3dResource[m_rtScheduler->GetCurrentTaskIndex()], iSourceOffset, iNumBytes);

			return true;
		}

		bool Upload(ID3D12Resource2* d3dDestinationResource, D3D12_RESOURCE_STATES d3dResourceState,
			UINT64 iNumBytes, UINT64 iSourceOffset, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			D3D12_RESOURCE_BARRIER d3dResourceTransition{};
			d3dResourceTransition.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dResourceTransition.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			d3dResourceTransition.Transition.pResource = d3dDestinationResource;
			d3dResourceTransition.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			d3dResourceTransition.Transition.StateBefore = d3dResourceState;
			d3dResourceTransition.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
			d3dCommandList->ResourceBarrier(1, &d3dResourceTransition);
			if ((iSourceOffset >= GetResourceSize()) || (iNumBytes > GetResourceSize())) return false;
			d3dCommandList->CopyBufferRegion(d3dDestinationResource, iDestinationOffset,
				m_d3dResource[m_rtScheduler->GetCurrentTaskIndex()], iSourceOffset, iNumBytes);
			d3dResourceTransition.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			d3dResourceTransition.Transition.StateAfter = d3dResourceState;
			d3dCommandList->ResourceBarrier(1, &d3dResourceTransition);
			
			return true;
		}
		
		void Release()
		{
			if (m_d3dResource)
			{
				for (unsigned int i = 0; i < m_iNumBuffers; i++)
				{
					if (m_d3dResource[i]) m_d3dResource[i]->Release();
				}
				delete m_d3dResource;
				m_d3dResource = nullptr;
			}
			m_iNumBuffers = 0;
		}
	};


	
	class ReadbackBuffer : public BaseShaderResource
	{
	private:

		//the data of the buffer
		void** m_pBufferDataPointer;
		unsigned int m_iNumBuffers;
		

	public:

		ReadbackBuffer() :
			BaseShaderResource(),
			m_pBufferDataPointer(nullptr),
			m_iNumBuffers(0) {};
		~ReadbackBuffer()
		{
			Release();
		};

		bool Initialize(GPUScheduler* rtScheduler, unsigned int iBufferSize)
		{
			m_rtScheduler = rtScheduler;
			if (!m_rtScheduler) return false;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iNumBuffers = m_rtScheduler->GetNumMaxTasks();
			if (m_iNumBuffers < 1) return false;
			m_d3dResource = new ID3D12Resource2*[m_iNumBuffers];
			if (!m_d3dResource) return false;
			m_pBufferDataPointer = new void*[m_iNumBuffers];
			if (!m_pBufferDataPointer) return false;

			m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			m_d3dResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			m_d3dResourceDesc.Width = (UINT64)iBufferSize;
			m_d3dResourceDesc.Height = 1;
			m_d3dResourceDesc.DepthOrArraySize = 1;
			m_d3dResourceDesc.MipLevels = 1;
			m_d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // a requirement for buffers
			m_d3dResourceDesc.SampleDesc.Count = 1;
			m_d3dResourceDesc.SampleDesc.Quality = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

			//create the upload buffer
			D3D12_HEAP_PROPERTIES d3dHeapProperties{};
			d3dHeapProperties.Type = D3D12_HEAP_TYPE_READBACK;
			d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			d3dHeapProperties.CreationNodeMask = 0;
			d3dHeapProperties.VisibleNodeMask = 0;

			for (unsigned int i = 0; i < m_iNumBuffers; i++)
			{
				if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
					D3D12_RESOURCE_STATE_COPY_DEST, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				D3D12_RANGE d3dReadRange = { 0, 0 }; //we don't CPU-Read-Access to this resource
				if (m_d3dResource[i]->Map(0, &d3dReadRange, m_pBufferDataPointer + i) < 0) return false;
			}

			return true;
		};

		bool Update(const void* pBufferData, unsigned int iNumBytes = 0, unsigned int iOffset = 0)
		{
			if (iNumBytes == 0) iNumBytes = (unsigned int)GetResourceSize();
			if (iOffset >= GetResourceSize()) return false;
			if ((UINT64)iNumBytes + (UINT64)iOffset > GetResourceSize()) return false;
			memcpy((byte*)(m_pBufferDataPointer[m_rtScheduler->GetCurrentTaskIndex()]) + iOffset, pBufferData, iNumBytes);

			return true;
		}

		bool Readback(ID3D12Resource2* d3dSource, UINT64 iNumBytes, UINT64 iSourceOffset, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			if ((iSourceOffset >= GetResourceSize()) || (iNumBytes > GetResourceSize())) return false;
			d3dCommandList->CopyBufferRegion(m_d3dResource[m_rtScheduler->GetCurrentTaskIndex()],
				iSourceOffset, d3dSource, iDestinationOffset, iNumBytes);

			return true;
		}

		bool Readback(ID3D12Resource2* d3dSource, D3D12_RESOURCE_STATES d3dResourceState,
			UINT64 iNumBytes, UINT64 iSourceOffset, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			D3D12_RESOURCE_BARRIER d3dResourceTransition{};
			d3dResourceTransition.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dResourceTransition.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			d3dResourceTransition.Transition.pResource = d3dSource;
			d3dResourceTransition.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			d3dResourceTransition.Transition.StateBefore = d3dResourceState;
			d3dResourceTransition.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
			d3dCommandList->ResourceBarrier(1, &d3dResourceTransition);
			if ((iSourceOffset >= GetResourceSize()) || (iNumBytes > GetResourceSize())) return false;
			d3dCommandList->CopyBufferRegion(d3dSource, iDestinationOffset,
				m_d3dResource[m_rtScheduler->GetCurrentTaskIndex()], iSourceOffset, iNumBytes);
			d3dResourceTransition.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
			d3dResourceTransition.Transition.StateAfter = d3dResourceState;
			d3dCommandList->ResourceBarrier(1, &d3dResourceTransition);

			return true;
		}
		
		void Release()
		{
			if (m_d3dResource)
			{
				for (unsigned int i = 0; i < m_iNumBuffers; i++)
				{
					if (m_d3dResource[i]) m_d3dResource[i]->Release();
				}
				delete m_d3dResource;
				m_d3dResource = nullptr;
			}
			m_iNumBuffers = 0;
		}
	};


	
	class ConstantBuffer : public BaseShaderResource
	{
	private:

		//the data of the buffer
		D3D12_CPU_DESCRIPTOR_HANDLE* m_d3dCPUDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE* m_d3dGPUDescriptor;
		void**				m_pBufferDataPointer;
		unsigned int m_iNumBuffers;
		

	public:

		ConstantBuffer() :
			BaseShaderResource(),
			m_d3dCPUDescriptor(nullptr),
			m_d3dGPUDescriptor(nullptr),
			m_pBufferDataPointer(nullptr),
			m_iNumBuffers(0) {};
		~ConstantBuffer()
		{
			Release();
		};

		bool Initialize(GPUScheduler* rtScheduler, unsigned int iBufferSize,
			DescriptorHeapInfo rtDescriptorInfo = {}, ResourceHeapInfo rtHeapInfo = {})
		{
			//initialize the member variables
			m_rtScheduler = rtScheduler;
			if (!m_rtScheduler) return false;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iNumBuffers = m_rtScheduler->GetNumMaxTasks();
			if (m_iNumBuffers < 1) return false;
			m_d3dResource = new ID3D12Resource2*[m_iNumBuffers];
			if (!m_d3dResource) return false;
			m_pBufferDataPointer = new void*[m_iNumBuffers];
			if (!m_pBufferDataPointer) return false;
			m_bResourceInDescriptorTable = (rtDescriptorInfo.d3dDescriptorHeap != nullptr);

			//initialize the descriptor handles
			if (m_bResourceInDescriptorTable)
			{
				m_d3dCPUDescriptor = new D3D12_CPU_DESCRIPTOR_HANDLE[m_iNumBuffers];
				if (!m_d3dCPUDescriptor) return false;
				m_d3dGPUDescriptor = new D3D12_GPU_DESCRIPTOR_HANDLE[m_iNumBuffers];
				if (!m_d3dGPUDescriptor) return false;

				unsigned int iDescriptorIncrementSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				for (unsigned int i = 0; i < m_iNumBuffers; i++)
				{
					m_d3dCPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart();
					m_d3dGPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetGPUDescriptorHandleForHeapStart();
					m_d3dCPUDescriptor[i].ptr += (SIZE_T)iDescriptorIncrementSize * (SIZE_T)(rtDescriptorInfo.iOffsetInDescriptor);
					m_d3dGPUDescriptor[i].ptr += (UINT64)iDescriptorIncrementSize * (UINT64)(rtDescriptorInfo.iOffsetInDescriptor);
				}
			}

			//fill the resource description
			m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			m_d3dResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			m_d3dResourceDesc.Width = (UINT64)iBufferSize;
			m_d3dResourceDesc.Height = 1;
			m_d3dResourceDesc.DepthOrArraySize = 1;
			m_d3dResourceDesc.MipLevels = 1;
			m_d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // a requirement for buffers
			m_d3dResourceDesc.SampleDesc.Count = 1;
			m_d3dResourceDesc.SampleDesc.Quality = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

			//create the resource (possibly in a resource heap)
			if (rtHeapInfo.d3dResourceHeap)
			{
				const UINT64 iIncrementSize = (GetResourceSize() + (UINT64)65535) & (UINT64)(~65535);
				for (unsigned int i = 0; i < m_iNumBuffers; i++)
				{
					if (d3dDevice->CreatePlacedResource1(rtHeapInfo.d3dResourceHeap, rtHeapInfo.iOffsetInHeap + (UINT64)i * iIncrementSize,
						&m_d3dResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
					D3D12_RANGE d3dReadRange = { 0, 0 }; //we don't CPU-Read-Access to this resource
					if (m_d3dResource[i]->Map(0, &d3dReadRange, m_pBufferDataPointer + i) < 0) return false;
				}
			}
			else
			{
				//create the upload buffer
				D3D12_HEAP_PROPERTIES d3dHeapProperties{};
				d3dHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
				d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				d3dHeapProperties.CreationNodeMask = 0;
				d3dHeapProperties.VisibleNodeMask = 0;

				for (unsigned int i = 0; i < m_iNumBuffers; i++)
				{
					if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
						D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
					D3D12_RANGE d3dReadRange = { 0, 0 }; //we don't CPU-Read-Access to this resource
					if (m_d3dResource[i]->Map(0, &d3dReadRange, m_pBufferDataPointer + i) < 0) return false;
				}
			}

			if (m_bResourceInDescriptorTable)
			{
				for (unsigned int i = 0; i < m_iNumBuffers; i++)
				{
					D3D12_CONSTANT_BUFFER_VIEW_DESC d3dBufferViewDesc{};
					d3dBufferViewDesc.BufferLocation = m_d3dResource[i]->GetGPUVirtualAddress();
					d3dBufferViewDesc.SizeInBytes = ((UINT)GetResourceSize() + 255) & ~255;

					d3dDevice->CreateConstantBufferView(&d3dBufferViewDesc, m_d3dCPUDescriptor[i]);
				}
			}

			return true;
		}

		void Update(const void* pBufferData)
		{
			memcpy(m_pBufferDataPointer[m_rtScheduler->GetCurrentTaskIndex()], pBufferData, (size_t)GetResourceSize());
		}

		void UpdateAll(const void* pBufferData)
		{
			for (unsigned int i = 0; i < m_iNumBuffers; i++)
			{
				memcpy(m_pBufferDataPointer[i], pBufferData, (size_t)GetResourceSize());
			}
		}

		void Bind(UINT iRootParameterIndex, bool bBindToCS = false)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();
			
			if (bBindToCS)
			{
				d3dCommandList->SetComputeRootConstantBufferView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
			else
			{
				d3dCommandList->SetGraphicsRootConstantBufferView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
		}
		
		void Release()
		{
			if (m_d3dResource)
			{
				for (unsigned int i = 0; i < m_iNumBuffers; i++)
				{
					if (m_d3dResource[i]) m_d3dResource[i]->Release();
				}
				delete m_d3dResource;
				m_d3dResource = nullptr;
			}

			if (m_d3dCPUDescriptor) delete m_d3dCPUDescriptor;
			m_d3dCPUDescriptor = nullptr;
			if (m_d3dGPUDescriptor) delete m_d3dGPUDescriptor;
			m_d3dGPUDescriptor = nullptr;
			m_iNumBuffers = 0;
		}
	};


	
	class StructuredBuffer : public BaseShaderResource
	{
	private:

		//the data of the buffer
		D3D12_CPU_DESCRIPTOR_HANDLE* m_d3dCPUDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE* m_d3dGPUDescriptor;
		unsigned int m_iNumBuffers;
		unsigned int m_iElementSize;
		unsigned int m_iNumElements;
		

	public:

		StructuredBuffer() :
			BaseShaderResource(),
			m_d3dCPUDescriptor(nullptr),
			m_d3dGPUDescriptor(nullptr),
			m_iNumBuffers(0),
			m_iElementSize(0),
			m_iNumElements(0) {};
		~StructuredBuffer()
		{
			Release();
		};

		bool Initialize(GPUScheduler* rtScheduler, unsigned int iElementSize, unsigned int iNumElements,
			DescriptorHeapInfo rtDescriptorInfo = {}, ResourceHeapInfo rtHeapInfo = {})
		{
			//initialize the variables
			m_rtScheduler = rtScheduler;
			if (!m_rtScheduler) return false;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iNumBuffers = m_rtScheduler->GetNumMaxTasks();
			if (m_iNumBuffers < 1) return false;
			m_d3dResource = new ID3D12Resource2*[m_iNumBuffers];
			if (!m_d3dResource) return false;
			m_iElementSize = iElementSize;
			m_iNumElements = iNumElements;
			m_bResourceInDescriptorTable = (rtDescriptorInfo.d3dDescriptorHeap != nullptr);

			//initialize the descriptor handles
			if (m_bResourceInDescriptorTable)
			{
				m_d3dCPUDescriptor = new D3D12_CPU_DESCRIPTOR_HANDLE[m_iNumBuffers];
				if (!m_d3dCPUDescriptor) return false;
				m_d3dGPUDescriptor = new D3D12_GPU_DESCRIPTOR_HANDLE[m_iNumBuffers];
				if (!m_d3dGPUDescriptor) return false;

				unsigned int iDescriptorIncrementSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				for (unsigned int i = 0; i < m_iNumBuffers; i++)
				{
					m_d3dCPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart();
					m_d3dGPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetGPUDescriptorHandleForHeapStart();
					m_d3dCPUDescriptor[i].ptr += (SIZE_T)iDescriptorIncrementSize * (SIZE_T)(rtDescriptorInfo.iOffsetInDescriptor);
					m_d3dGPUDescriptor[i].ptr += (UINT64)iDescriptorIncrementSize * (UINT64)(rtDescriptorInfo.iOffsetInDescriptor);
				}
			}

			//fill the resource description
			m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			m_d3dResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			m_d3dResourceDesc.Width = (UINT64)m_iElementSize * (UINT64)m_iNumElements;
			m_d3dResourceDesc.Height = 1;
			m_d3dResourceDesc.DepthOrArraySize = 1;
			m_d3dResourceDesc.MipLevels = 1;
			m_d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // a requirement for buffers
			m_d3dResourceDesc.SampleDesc.Count = 1;
			m_d3dResourceDesc.SampleDesc.Quality = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

			//create the resource
			if (rtHeapInfo.d3dResourceHeap)
			{
				const UINT64 iIncrementSize = (UINT64)(((m_iElementSize * m_iNumElements) + 65535) & ~65535);
				for (unsigned int i = 0; i < m_iNumBuffers; i++)
				{
					if (d3dDevice->CreatePlacedResource1(rtHeapInfo.d3dResourceHeap, rtHeapInfo.iOffsetInHeap + (UINT64)i * iIncrementSize,
						&m_d3dResourceDesc, D3D12_RESOURCE_STATE_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}
			else
			{
				D3D12_HEAP_PROPERTIES d3dHeapProperties{};
				d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
				d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				d3dHeapProperties.CreationNodeMask = 0;
				d3dHeapProperties.VisibleNodeMask = 0;

				for (unsigned int i = 0; i < m_iNumBuffers; i++)
				{
					if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
						D3D12_RESOURCE_STATE_SHADER_RESOURCE, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}

			//create the resource view
			if (m_bResourceInDescriptorTable)
			{
				for (unsigned int i = 0; i < m_iNumBuffers; i++)
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC d3dBufferViewDesc{};
					d3dBufferViewDesc.Format = DXGI_FORMAT_UNKNOWN;
					d3dBufferViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
					d3dBufferViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					d3dBufferViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
					d3dBufferViewDesc.Buffer.FirstElement = 0;
					d3dBufferViewDesc.Buffer.NumElements = m_iNumElements;
					d3dBufferViewDesc.Buffer.StructureByteStride = m_iElementSize;

					d3dDevice->CreateShaderResourceView(m_d3dResource[i], & d3dBufferViewDesc, m_d3dCPUDescriptor[i]);
				}
			}

			return true;
		}

		bool Upload(UploadBuffer* rtSourceBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			if (iNumBytes == 0) iNumBytes = (UINT64)m_iNumElements * (UINT64)m_iElementSize;
			if (iNumBytes + iDestinationOffset > (UINT64)m_iNumElements * (UINT64)m_iElementSize) return false;

			return rtSourceBuffer->Upload(m_d3dResource[iIndex], D3D12_RESOURCE_STATE_SHADER_RESOURCE, iNumBytes, iSourceOffset, iDestinationOffset);
		}

		bool UploadAll(UploadBuffer* rtSourceBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			if (iNumBytes == 0) iNumBytes = (UINT64)m_iNumElements * (UINT64)m_iElementSize;
			if (iNumBytes + iDestinationOffset > (UINT64)m_iNumElements * (UINT64)m_iElementSize) return false;

			for (unsigned int i = 0; i < m_iNumBuffers; i++)
			{
				if (!(rtSourceBuffer->Upload(m_d3dResource[i], D3D12_RESOURCE_STATE_SHADER_RESOURCE,
					iNumBytes, iSourceOffset, iDestinationOffset))) return false;
			}

			return true;
		}

		void Bind(UINT iRootParameterIndex, bool bBindToCS = false)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();
			
			if (bBindToCS)
			{
				d3dCommandList->SetComputeRootShaderResourceView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
			else
			{
				d3dCommandList->SetGraphicsRootShaderResourceView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
		}
		
		void Release()
		{
			if (m_d3dResource)
			{
				for (unsigned int i = 0; i < m_iNumBuffers; i++)
				{
					if (m_d3dResource[i]) m_d3dResource[i]->Release();
				}
				delete m_d3dResource;
				m_d3dResource = nullptr;
			}
			m_iNumBuffers = 0;

			if (m_d3dCPUDescriptor) delete m_d3dCPUDescriptor;
			m_d3dCPUDescriptor = nullptr;
			if (m_d3dGPUDescriptor) delete m_d3dGPUDescriptor;
			m_d3dGPUDescriptor = nullptr;
		}
	};


	
	class Texture1D : public BaseShaderResource
	{
	private:

		//the data of the texture
		D3D12_CPU_DESCRIPTOR_HANDLE* m_d3dCPUDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE* m_d3dGPUDescriptor;
		unsigned int m_iNumTextures;
		unsigned int m_iBytesPerPixel;
		

	public:

		Texture1D() :
			BaseShaderResource(),
			m_d3dCPUDescriptor(nullptr),
			m_d3dGPUDescriptor(nullptr),
			m_iNumTextures(0),
			m_iBytesPerPixel(0) {};
		~Texture1D()
		{
			Release();
		};
		
		bool Initialize(GPUScheduler* rtScheduler, DXGI_FORMAT dxTextureFormat, unsigned int iWidth, unsigned int iMipLevels = 1,
			DescriptorHeapInfo rtDescriptorInfo = {}, ResourceHeapInfo rtHeapInfo = {})
		{
			//initialize the member variables
			m_rtScheduler = rtScheduler;
			if (!m_rtScheduler) return false;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iNumTextures = m_rtScheduler->GetNumMaxTasks();
			if (m_iNumTextures < 1) return false;
			m_d3dResource = new ID3D12Resource2*[m_iNumTextures];
			if (!m_d3dResource) return false;
			m_iBytesPerPixel = (unsigned int)DirectX::BitsPerPixel(dxTextureFormat) / 8;
			m_bResourceInDescriptorTable = (rtDescriptorInfo.d3dDescriptorHeap != nullptr);

			//initialize the descriptor handles
			if (m_bResourceInDescriptorTable)
			{
				m_d3dCPUDescriptor = new D3D12_CPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dCPUDescriptor) return false;
				m_d3dGPUDescriptor = new D3D12_GPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dGPUDescriptor) return false;

				unsigned int iDescriptorIncrementSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					m_d3dCPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart();
					m_d3dGPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetGPUDescriptorHandleForHeapStart();
					m_d3dCPUDescriptor[i].ptr += (SIZE_T)iDescriptorIncrementSize * (SIZE_T)(rtDescriptorInfo.iOffsetInDescriptor);
					m_d3dGPUDescriptor[i].ptr += (UINT64)iDescriptorIncrementSize * (UINT64)(rtDescriptorInfo.iOffsetInDescriptor);
				}
			}

			//fill the resource description
			m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			m_d3dResourceDesc.Format = dxTextureFormat;
			m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			m_d3dResourceDesc.Width = (UINT64)iWidth;
			m_d3dResourceDesc.Height = 1;
			m_d3dResourceDesc.DepthOrArraySize = 1;
			m_d3dResourceDesc.MipLevels = (UINT16)iMipLevels;
			m_d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			m_d3dResourceDesc.SampleDesc.Count = 1;
			m_d3dResourceDesc.SampleDesc.Quality = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

			//create the resource
			if (rtHeapInfo.d3dResourceHeap)
			{
				const UINT64 iIncrementSize = (UINT64)(((m_iBytesPerPixel * GetResourceSize()) + 65535) & ~65535);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreatePlacedResource1(rtHeapInfo.d3dResourceHeap, rtHeapInfo.iOffsetInHeap + (UINT64)i * iIncrementSize,
						&m_d3dResourceDesc, D3D12_RESOURCE_STATE_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}
			else
			{
				D3D12_HEAP_PROPERTIES d3dHeapProperties{};
				d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
				d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				d3dHeapProperties.CreationNodeMask = 0;
				d3dHeapProperties.VisibleNodeMask = 0;

				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
						D3D12_RESOURCE_STATE_SHADER_RESOURCE, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}

			//create the resource view
			if (m_bResourceInDescriptorTable)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC d3dTextureViewDesc{};
					d3dTextureViewDesc.Format = GetFormat();
					d3dTextureViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
					d3dTextureViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					d3dTextureViewDesc.Texture1D.MipLevels = GetMipLevels();
					d3dTextureViewDesc.Texture1D.MostDetailedMip = 0;
					d3dTextureViewDesc.Texture1D.ResourceMinLODClamp = 0.0f;

					d3dDevice->CreateShaderResourceView(m_d3dResource[i], &d3dTextureViewDesc, m_d3dCPUDescriptor[i]);
				}
			}

			return true;
		}

		bool Upload(UploadBuffer* rtSourceBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			return rtSourceBuffer->Upload(m_d3dResource[iIndex], D3D12_RESOURCE_STATE_SHADER_RESOURCE, iNumBytes, iSourceOffset, iDestinationOffset);
		}

		bool UploadAll(UploadBuffer* rtSourceBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			for (unsigned int i = 0; i < m_iNumTextures; i++)
			{
				if (!(rtSourceBuffer->Upload(m_d3dResource[i], D3D12_RESOURCE_STATE_SHADER_RESOURCE,
					iNumBytes, iSourceOffset, iDestinationOffset))) return false;
			}

			return true;
		}

		void Bind(UINT iRootParameterIndex, bool bBindToCS = false)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();
			
			if (bBindToCS)
			{
				d3dCommandList->SetComputeRootShaderResourceView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
			else
			{
				d3dCommandList->SetGraphicsRootShaderResourceView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
		}
		
		void Release()
		{
			if (m_d3dResource)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (m_d3dResource[i]) m_d3dResource[i]->Release();
				}
				delete m_d3dResource;
				m_d3dResource = nullptr;
			}
			m_iNumTextures = 0;

			if (m_d3dCPUDescriptor) delete m_d3dCPUDescriptor;
			m_d3dCPUDescriptor = nullptr;
			if (m_d3dGPUDescriptor) delete m_d3dGPUDescriptor;
			m_d3dGPUDescriptor = nullptr;
		}
	};


	
	class Texture1DArray : public BaseShaderResource
	{
	private:

		//the data of the texture
		D3D12_CPU_DESCRIPTOR_HANDLE* m_d3dCPUDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE* m_d3dGPUDescriptor;
		unsigned int m_iNumTextures;
		unsigned int m_iBytesPerPixel;
		

	public:

		Texture1DArray() :
			BaseShaderResource(),
			m_d3dCPUDescriptor(nullptr),
			m_d3dGPUDescriptor(nullptr),
			m_iNumTextures(0),
			m_iBytesPerPixel(0) {};
		~Texture1DArray()
		{
			Release();
		};
		
		bool Initialize(GPUScheduler* rtScheduler, DXGI_FORMAT dxTextureFormat, unsigned int iWidth, unsigned int iArraySize,
			unsigned int iMipLevels = 1, DescriptorHeapInfo rtDescriptorInfo = {}, ResourceHeapInfo rtHeapInfo = {})
		{
			//initialize the member variables
			m_rtScheduler = rtScheduler;
			if (!m_rtScheduler) return false;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iNumTextures = m_rtScheduler->GetNumMaxTasks();
			if (m_iNumTextures < 1) return false;
			m_d3dResource = new ID3D12Resource2*[m_iNumTextures];
			if (!m_d3dResource) return false;
			m_iBytesPerPixel = (unsigned int)DirectX::BitsPerPixel(dxTextureFormat) / 8;
			m_bResourceInDescriptorTable = (rtDescriptorInfo.d3dDescriptorHeap != nullptr);

			//initialize the descriptor handles
			if (m_bResourceInDescriptorTable)
			{
				m_d3dCPUDescriptor = new D3D12_CPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dCPUDescriptor) return false;
				m_d3dGPUDescriptor = new D3D12_GPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dGPUDescriptor) return false;

				unsigned int iDescriptorIncrementSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					m_d3dCPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart();
					m_d3dGPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetGPUDescriptorHandleForHeapStart();
					m_d3dCPUDescriptor[i].ptr += (SIZE_T)iDescriptorIncrementSize * (SIZE_T)(rtDescriptorInfo.iOffsetInDescriptor);
					m_d3dGPUDescriptor[i].ptr += (UINT64)iDescriptorIncrementSize * (UINT64)(rtDescriptorInfo.iOffsetInDescriptor);
				}
			}

			//fill the resource desc
			m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			m_d3dResourceDesc.Format = dxTextureFormat;
			m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			m_d3dResourceDesc.Width = (UINT64)iWidth;
			m_d3dResourceDesc.Height = 1;
			m_d3dResourceDesc.DepthOrArraySize = (UINT16)iArraySize;
			m_d3dResourceDesc.MipLevels = (UINT16)iMipLevels;
			m_d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			m_d3dResourceDesc.SampleDesc.Count = 1;
			m_d3dResourceDesc.SampleDesc.Quality = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

			//create the resource
			if (rtHeapInfo.d3dResourceHeap)
			{
				const UINT64 iIncrementSize = (UINT64)(((m_iBytesPerPixel * GetResourceSize()) + 65535) & ~65535);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreatePlacedResource1(rtHeapInfo.d3dResourceHeap, rtHeapInfo.iOffsetInHeap + (UINT64)i * iIncrementSize,
						&m_d3dResourceDesc, D3D12_RESOURCE_STATE_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}
			else
			{
				D3D12_HEAP_PROPERTIES d3dHeapProperties{};
				d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
				d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				d3dHeapProperties.CreationNodeMask = 0;
				d3dHeapProperties.VisibleNodeMask = 0;

				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
						D3D12_RESOURCE_STATE_SHADER_RESOURCE, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}

			//create the resource view
			if (m_bResourceInDescriptorTable)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					D3D12_SHADER_RESOURCE_VIEW_DESC d3dTextureViewDesc{};
					d3dTextureViewDesc.Format = GetFormat();
					d3dTextureViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
					d3dTextureViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					d3dTextureViewDesc.Texture1DArray.ArraySize = GetDepthOrArraySize();
					d3dTextureViewDesc.Texture1DArray.FirstArraySlice = 0;
					d3dTextureViewDesc.Texture1DArray.MipLevels = GetMipLevels();
					d3dTextureViewDesc.Texture1DArray.MostDetailedMip = 0;
					d3dTextureViewDesc.Texture1DArray.ResourceMinLODClamp = 0.0f;

					d3dDevice->CreateShaderResourceView(m_d3dResource[i], &d3dTextureViewDesc, m_d3dCPUDescriptor[i]);
				}
			}

			return true;
		}

		bool Upload(UploadBuffer* rtSourceBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			return rtSourceBuffer->Upload(m_d3dResource[iIndex], D3D12_RESOURCE_STATE_SHADER_RESOURCE, iNumBytes, iSourceOffset, iDestinationOffset);
		}

		bool UploadAll(UploadBuffer* rtSourceBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			for (unsigned int i = 0; i < m_iNumTextures; i++)
			{
				if (!(rtSourceBuffer->Upload(m_d3dResource[i], D3D12_RESOURCE_STATE_SHADER_RESOURCE,
					iNumBytes, iSourceOffset, iDestinationOffset))) return false;
			}

			return true;
		}

		void Bind(UINT iRootParameterIndex, bool bBindToCS = false)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();
			
			if (bBindToCS)
			{
				d3dCommandList->SetComputeRootShaderResourceView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
			else
			{
				d3dCommandList->SetGraphicsRootShaderResourceView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
		}
		
		void Release()
		{
			if (m_d3dResource)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (m_d3dResource[i]) m_d3dResource[i]->Release();
				}
				delete m_d3dResource;
				m_d3dResource = nullptr;
			}
			m_iNumTextures = 0;

			if (m_d3dCPUDescriptor) delete m_d3dCPUDescriptor;
			m_d3dCPUDescriptor = nullptr;
			if (m_d3dGPUDescriptor) delete m_d3dGPUDescriptor;
			m_d3dGPUDescriptor = nullptr;
		}
	};



	class Texture2D : public BaseShaderResource
	{
	private:

		//the data of the texture
		D3D12_CPU_DESCRIPTOR_HANDLE* m_d3dCPUDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE* m_d3dGPUDescriptor;
		unsigned int m_iNumTextures;
		unsigned int m_iBytesPerPixel;


	public:

		Texture2D() :
			BaseShaderResource(),
			m_d3dCPUDescriptor(nullptr),
			m_d3dGPUDescriptor(nullptr),
			m_iNumTextures(0),
			m_iBytesPerPixel(0) {};
		~Texture2D()
		{
			Release();
		};

		bool Initialize(GPUScheduler* rtScheduler, DXGI_FORMAT dxTextureFormat, unsigned int iWidth, unsigned int iHeight, unsigned int iMipLevels = 1,
			bool bSwizzledLayout = true, DescriptorHeapInfo rtDescriptorInfo = {}, ResourceHeapInfo rtHeapInfo = {})
		{
			//initialize the member variables
			m_rtScheduler = rtScheduler;
			if (!m_rtScheduler) return false;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iNumTextures = m_rtScheduler->GetNumMaxTasks();
			if (m_iNumTextures < 1) return false;
			m_d3dResource = new ID3D12Resource2*[m_iNumTextures];
			if (!m_d3dResource) return false;
			m_iBytesPerPixel = (unsigned int)DirectX::BitsPerPixel(dxTextureFormat) / 8;
			m_bResourceInDescriptorTable = (rtDescriptorInfo.d3dDescriptorHeap != nullptr);

			//initialize the descriptor handles
			if (m_bResourceInDescriptorTable)
			{
				m_d3dCPUDescriptor = new D3D12_CPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dCPUDescriptor) return false;
				m_d3dGPUDescriptor = new D3D12_GPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dGPUDescriptor) return false;

				unsigned int iDescriptorIncrementSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					m_d3dCPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart();
					m_d3dGPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetGPUDescriptorHandleForHeapStart();
					m_d3dCPUDescriptor[i].ptr += (SIZE_T)iDescriptorIncrementSize * (SIZE_T)(rtDescriptorInfo.iOffsetInDescriptor);
					m_d3dGPUDescriptor[i].ptr += (UINT64)iDescriptorIncrementSize * (UINT64)(rtDescriptorInfo.iOffsetInDescriptor);
				}
			}

			//fill in the resource desc
			m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			m_d3dResourceDesc.Format = dxTextureFormat;
			m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			m_d3dResourceDesc.Width = (UINT64)iWidth;
			m_d3dResourceDesc.Height = (UINT)iHeight;
			m_d3dResourceDesc.DepthOrArraySize = 1;
			m_d3dResourceDesc.MipLevels = (UINT16)iMipLevels;
			m_d3dResourceDesc.Layout = bSwizzledLayout ? D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE : D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			m_d3dResourceDesc.SampleDesc.Count = 1;
			m_d3dResourceDesc.SampleDesc.Quality = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

			//create the resource
			if (rtHeapInfo.d3dResourceHeap)
			{
				const UINT64 iIncrementSize = (UINT64)(((m_iBytesPerPixel * GetResourceSize()) + 65535) & ~65535);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreatePlacedResource1(rtHeapInfo.d3dResourceHeap, rtHeapInfo.iOffsetInHeap + (UINT64)i * iIncrementSize,
						&m_d3dResourceDesc, D3D12_RESOURCE_STATE_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}
			else
			{
				D3D12_HEAP_PROPERTIES d3dHeapProperties{};
				d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
				d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				d3dHeapProperties.CreationNodeMask = 0;
				d3dHeapProperties.VisibleNodeMask = 0;

				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
						D3D12_RESOURCE_STATE_SHADER_RESOURCE, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}

			//create the resource view
			if (m_bResourceInDescriptorTable)
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC d3dTextureViewDesc{};
				d3dTextureViewDesc.Format = GetFormat();
				d3dTextureViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				d3dTextureViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				d3dTextureViewDesc.Texture2D.PlaneSlice = 0;
				d3dTextureViewDesc.Texture2D.MipLevels = GetMipLevels();
				d3dTextureViewDesc.Texture2D.MostDetailedMip = 0;
				d3dTextureViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;

				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					d3dDevice->CreateShaderResourceView(m_d3dResource[i], &d3dTextureViewDesc, m_d3dCPUDescriptor[i]);
				}
			}

			return true;
		}

		bool Upload(UploadBuffer* rtSourceBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			return rtSourceBuffer->Upload(m_d3dResource[iIndex], D3D12_RESOURCE_STATE_SHADER_RESOURCE, iNumBytes, iSourceOffset, iDestinationOffset);
		}

		bool UploadAll(UploadBuffer* rtSourceBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			for (unsigned int i = 0; i < m_iNumTextures; i++)
			{
				if (!(rtSourceBuffer->Upload(m_d3dResource[i], D3D12_RESOURCE_STATE_SHADER_RESOURCE,
					iNumBytes, iSourceOffset, iDestinationOffset))) return false;
			}

			return true;
		}

		void Bind(UINT iRootParameterIndex, bool bBindToCS = false)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			if (bBindToCS)
			{
				d3dCommandList->SetComputeRootShaderResourceView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
			else
			{
				d3dCommandList->SetGraphicsRootShaderResourceView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
		}

		void Release()
		{
			if (m_d3dResource)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (m_d3dResource[i]) m_d3dResource[i]->Release();
				}
				delete m_d3dResource;
				m_d3dResource = nullptr;
			}
			m_iNumTextures = 0;

			if (m_d3dCPUDescriptor) delete m_d3dCPUDescriptor;
			m_d3dCPUDescriptor = nullptr;
			if (m_d3dGPUDescriptor) delete m_d3dGPUDescriptor;
			m_d3dGPUDescriptor = nullptr;
		}
	};



	class Texture2DArray : public BaseShaderResource
	{
	private:

		//the data of the texture
		D3D12_CPU_DESCRIPTOR_HANDLE* m_d3dCPUDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE* m_d3dGPUDescriptor;
		unsigned int m_iNumTextures;
		unsigned int m_iBytesPerPixel;


	public:

		Texture2DArray() :
			BaseShaderResource(),
			m_d3dCPUDescriptor(nullptr),
			m_d3dGPUDescriptor(nullptr),
			m_iNumTextures(0),
			m_iBytesPerPixel(0) {};
		~Texture2DArray()
		{
			Release();
		};

		bool Initialize(GPUScheduler* rtScheduler, DXGI_FORMAT dxTextureFormat, unsigned int iWidth, unsigned int iHeight, unsigned int iArraySize,
			unsigned int iMipLevels = 1, DescriptorHeapInfo rtDescriptorInfo = {}, ResourceHeapInfo rtHeapInfo = {})
		{
			//initialize the member variables
			m_rtScheduler = rtScheduler;
			if (!m_rtScheduler) return false;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iNumTextures = m_rtScheduler->GetNumMaxTasks();
			if (m_iNumTextures < 1) return false;
			m_d3dResource = new ID3D12Resource2*[m_iNumTextures];
			if (!m_d3dResource) return false;
			m_iBytesPerPixel = (unsigned int)DirectX::BitsPerPixel(dxTextureFormat) / 8;
			m_bResourceInDescriptorTable = (rtDescriptorInfo.d3dDescriptorHeap != nullptr);

			//initialize the descriptor handles
			if (m_bResourceInDescriptorTable)
			{
				m_d3dCPUDescriptor = new D3D12_CPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dCPUDescriptor) return false;
				m_d3dGPUDescriptor = new D3D12_GPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dGPUDescriptor) return false;

				unsigned int iDescriptorIncrementSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					m_d3dCPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart();
					m_d3dGPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetGPUDescriptorHandleForHeapStart();
					m_d3dCPUDescriptor[i].ptr += (SIZE_T)iDescriptorIncrementSize * (SIZE_T)(rtDescriptorInfo.iOffsetInDescriptor);
					m_d3dGPUDescriptor[i].ptr += (UINT64)iDescriptorIncrementSize * (UINT64)(rtDescriptorInfo.iOffsetInDescriptor);
				}
			}

			//fill the resource desc
			m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			m_d3dResourceDesc.Format = dxTextureFormat;
			m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			m_d3dResourceDesc.Width = (UINT64)iWidth;
			m_d3dResourceDesc.Height = (UINT)iHeight;
			m_d3dResourceDesc.DepthOrArraySize = (UINT16)iArraySize;
			m_d3dResourceDesc.MipLevels = (UINT16)iMipLevels;
			m_d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			m_d3dResourceDesc.SampleDesc.Count = 1;
			m_d3dResourceDesc.SampleDesc.Quality = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

			//create the resource
			if (rtHeapInfo.d3dResourceHeap)
			{
				const UINT64 iIncrementSize = (UINT64)(((m_iBytesPerPixel * GetResourceSize()) + 65535) & ~65535);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreatePlacedResource1(rtHeapInfo.d3dResourceHeap, rtHeapInfo.iOffsetInHeap + (UINT64)i * iIncrementSize,
						&m_d3dResourceDesc, D3D12_RESOURCE_STATE_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}
			else
			{
				D3D12_HEAP_PROPERTIES d3dHeapProperties{};
				d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
				d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				d3dHeapProperties.CreationNodeMask = 0;
				d3dHeapProperties.VisibleNodeMask = 0;

				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
						D3D12_RESOURCE_STATE_SHADER_RESOURCE, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}

			//create the resource view
			if (m_bResourceInDescriptorTable)
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC d3dTextureViewDesc{};
				d3dTextureViewDesc.Format = GetFormat();
				d3dTextureViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				d3dTextureViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				d3dTextureViewDesc.Texture2DArray.ArraySize = GetDepthOrArraySize();
				d3dTextureViewDesc.Texture2DArray.FirstArraySlice = 0;
				d3dTextureViewDesc.Texture2DArray.PlaneSlice = 0;
				d3dTextureViewDesc.Texture2DArray.MipLevels = GetMipLevels();
				d3dTextureViewDesc.Texture2DArray.MostDetailedMip = 0;
				d3dTextureViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;

				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					d3dDevice->CreateShaderResourceView(m_d3dResource[i], &d3dTextureViewDesc, m_d3dCPUDescriptor[i]);
				}
			}

			return true;
		}

		bool Upload(UploadBuffer* rtSourceBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			return rtSourceBuffer->Upload(m_d3dResource[iIndex], D3D12_RESOURCE_STATE_SHADER_RESOURCE, iNumBytes, iSourceOffset, iDestinationOffset);
		}

		bool UploadAll(UploadBuffer* rtSourceBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			for (unsigned int i = 0; i < m_iNumTextures; i++)
			{
				if (!(rtSourceBuffer->Upload(m_d3dResource[i], D3D12_RESOURCE_STATE_SHADER_RESOURCE,
					iNumBytes, iSourceOffset, iDestinationOffset))) return false;
			}

			return true;
		}

		void Bind(UINT iRootParameterIndex, bool bBindToCS = false)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			if (bBindToCS)
			{
				d3dCommandList->SetComputeRootShaderResourceView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
			else
			{
				d3dCommandList->SetGraphicsRootShaderResourceView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
		}

		void Release()
		{
			if (m_d3dResource)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (m_d3dResource[i]) m_d3dResource[i]->Release();
				}
				delete m_d3dResource;
				m_d3dResource = nullptr;
			}
			m_iNumTextures = 0;

			if (m_d3dCPUDescriptor) delete m_d3dCPUDescriptor;
			m_d3dCPUDescriptor = nullptr;
			if (m_d3dGPUDescriptor) delete m_d3dGPUDescriptor;
			m_d3dGPUDescriptor = nullptr;
		}
	};



	class Texture3D : public BaseShaderResource
	{
	private:

		//the data of the texture
		D3D12_CPU_DESCRIPTOR_HANDLE* m_d3dCPUDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE* m_d3dGPUDescriptor;
		unsigned int m_iNumTextures;
		unsigned int m_iBytesPerPixel;


	public:

		Texture3D() :
			BaseShaderResource(),
			m_d3dCPUDescriptor(nullptr),
			m_d3dGPUDescriptor(nullptr),
			m_iNumTextures(0),
			m_iBytesPerPixel(0) {};
		~Texture3D()
		{
			Release();
		};

		bool Initialize(GPUScheduler* rtScheduler, DXGI_FORMAT dxTextureFormat, unsigned int iWidth, unsigned int iHeight, unsigned int iDepth,
			unsigned int iMipLevels = 1, DescriptorHeapInfo rtDescriptorInfo = {}, ResourceHeapInfo rtHeapInfo = {})
		{
			//initialize the member variables
			m_rtScheduler = rtScheduler;
			if (!m_rtScheduler) return false;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iNumTextures = m_rtScheduler->GetNumMaxTasks();
			if (m_iNumTextures < 1) return false;
			m_d3dResource = new ID3D12Resource2*[m_iNumTextures];
			if (!m_d3dResource) return false;
			m_iBytesPerPixel = (unsigned int)DirectX::BitsPerPixel(dxTextureFormat) / 8;
			m_bResourceInDescriptorTable = (rtDescriptorInfo.d3dDescriptorHeap != nullptr);

			//initialize the descriptor handles
			if (m_bResourceInDescriptorTable)
			{
				m_d3dCPUDescriptor = new D3D12_CPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dCPUDescriptor) return false;
				m_d3dGPUDescriptor = new D3D12_GPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dGPUDescriptor) return false;

				unsigned int iDescriptorIncrementSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					m_d3dCPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart();
					m_d3dGPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetGPUDescriptorHandleForHeapStart();
					m_d3dCPUDescriptor[i].ptr += (SIZE_T)iDescriptorIncrementSize * (SIZE_T)(rtDescriptorInfo.iOffsetInDescriptor);
					m_d3dGPUDescriptor[i].ptr += (UINT64)iDescriptorIncrementSize * (UINT64)(rtDescriptorInfo.iOffsetInDescriptor);
				}
			}

			//fill the resource desc
			m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			m_d3dResourceDesc.Format = dxTextureFormat;
			m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			m_d3dResourceDesc.Width = (UINT64)iWidth;
			m_d3dResourceDesc.Height = (UINT)iHeight;
			m_d3dResourceDesc.DepthOrArraySize = (UINT16)iDepth;
			m_d3dResourceDesc.MipLevels = (UINT16)iMipLevels;
			m_d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
			m_d3dResourceDesc.SampleDesc.Count = 1;
			m_d3dResourceDesc.SampleDesc.Quality = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

			//create the resource
			if (rtHeapInfo.d3dResourceHeap)
			{
				const UINT64 iIncrementSize = (UINT64)(((m_iBytesPerPixel * GetResourceSize()) + 65535) & ~65535);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreatePlacedResource1(rtHeapInfo.d3dResourceHeap, rtHeapInfo.iOffsetInHeap + (UINT64)i * iIncrementSize,
						&m_d3dResourceDesc, D3D12_RESOURCE_STATE_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}
			else
			{
				D3D12_HEAP_PROPERTIES d3dHeapProperties{};
				d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
				d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				d3dHeapProperties.CreationNodeMask = 0;
				d3dHeapProperties.VisibleNodeMask = 0;

				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
						D3D12_RESOURCE_STATE_SHADER_RESOURCE, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}

			//create the resource view
			if (m_bResourceInDescriptorTable)
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC d3dTextureViewDesc{};
				d3dTextureViewDesc.Format = GetFormat();
				d3dTextureViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				d3dTextureViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				d3dTextureViewDesc.Texture3D.MipLevels = GetMipLevels();
				d3dTextureViewDesc.Texture3D.MostDetailedMip = 0;
				d3dTextureViewDesc.Texture3D.ResourceMinLODClamp = 0.0f;

				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					d3dDevice->CreateShaderResourceView(m_d3dResource[i], &d3dTextureViewDesc, m_d3dCPUDescriptor[i]);
				}
			}

			return true;
		}

		bool Upload(UploadBuffer* rtSourceBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			return rtSourceBuffer->Upload(m_d3dResource[iIndex], D3D12_RESOURCE_STATE_SHADER_RESOURCE, iNumBytes, iSourceOffset, iDestinationOffset);
		}

		bool UploadAll(UploadBuffer* rtSourceBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			for (unsigned int i = 0; i < m_iNumTextures; i++)
			{
				if (!(rtSourceBuffer->Upload(m_d3dResource[i], D3D12_RESOURCE_STATE_SHADER_RESOURCE,
					iNumBytes, iSourceOffset, iDestinationOffset))) return false;
			}

			return true;
		}

		void Bind(UINT iRootParameterIndex, bool bBindToCS = false)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			if (bBindToCS)
			{
				d3dCommandList->SetComputeRootShaderResourceView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
			else
			{
				d3dCommandList->SetGraphicsRootShaderResourceView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
		}

		void Release()
		{
			if (m_d3dResource)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (m_d3dResource[i]) m_d3dResource[i]->Release();
				}
				delete m_d3dResource;
				m_d3dResource = nullptr;
			}
			m_iNumTextures = 0;

			if (m_d3dCPUDescriptor) delete m_d3dCPUDescriptor;
			m_d3dCPUDescriptor = nullptr;
			if (m_d3dGPUDescriptor) delete m_d3dGPUDescriptor;
			m_d3dGPUDescriptor = nullptr;
		}
	};



	class RWStructuredBuffer : public BaseShaderResource
	{
	private:

		//the data of the buffer
		D3D12_CPU_DESCRIPTOR_HANDLE* m_d3dCPUDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE* m_d3dGPUDescriptor;
		unsigned int m_iNumBuffers;
		unsigned int m_iElementSize;
		unsigned int m_iNumElements;


	public:

		RWStructuredBuffer() :
			BaseShaderResource(),
			m_d3dCPUDescriptor(nullptr),
			m_d3dGPUDescriptor(nullptr),
			m_iNumBuffers(0),
			m_iElementSize(0),
			m_iNumElements(0) {};
		~RWStructuredBuffer()
		{
			Release();
		};

		bool Initialize(GPUScheduler* rtScheduler, unsigned int iElementSize, unsigned int iNumElements,
			DescriptorHeapInfo rtDescriptorInfo = {}, ResourceHeapInfo rtHeapInfo = {})
		{
			//initialize the member variables
			m_rtScheduler = rtScheduler;
			if (!m_rtScheduler) return false;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iNumBuffers = m_rtScheduler->GetNumMaxTasks();
			if (m_iNumBuffers < 1) return false;
			m_d3dResource = new ID3D12Resource2*[1];
			if (!m_d3dResource) return false;
			m_iElementSize = iElementSize;
			m_iNumElements = iNumElements;
			m_bResourceInDescriptorTable = (rtDescriptorInfo.d3dDescriptorHeap != nullptr);

			//initialize the descriptor handles
			if (m_bResourceInDescriptorTable)
			{
				m_d3dCPUDescriptor = new D3D12_CPU_DESCRIPTOR_HANDLE[m_iNumBuffers];
				if (!m_d3dCPUDescriptor) return false;
				m_d3dGPUDescriptor = new D3D12_GPU_DESCRIPTOR_HANDLE[m_iNumBuffers];
				if (!m_d3dGPUDescriptor) return false;

				unsigned int iDescriptorIncrementSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				for (unsigned int i = 0; i < m_iNumBuffers; i++)
				{
					m_d3dCPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart();
					m_d3dGPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetGPUDescriptorHandleForHeapStart();
					m_d3dCPUDescriptor[i].ptr += (SIZE_T)iDescriptorIncrementSize * (SIZE_T)(rtDescriptorInfo.iOffsetInDescriptor);
					m_d3dGPUDescriptor[i].ptr += (UINT64)iDescriptorIncrementSize * (UINT64)(rtDescriptorInfo.iOffsetInDescriptor);
				}
			}

			//fill in the resource desc
			m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			m_d3dResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			m_d3dResourceDesc.Width = (UINT64)m_iElementSize * (UINT64)m_iNumElements;
			m_d3dResourceDesc.Height = 1;
			m_d3dResourceDesc.DepthOrArraySize = 1;
			m_d3dResourceDesc.MipLevels = 1;
			m_d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // a requirement for buffers
			m_d3dResourceDesc.SampleDesc.Count = 1;
			m_d3dResourceDesc.SampleDesc.Quality = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

			//create the resource
			if (rtHeapInfo.d3dResourceHeap)
			{
				const UINT64 iIncrementSize = (UINT64)(((m_iElementSize * m_iNumElements) + 65535) & ~65535);
				for (unsigned int i = 0; i < 1; i++)
				{
					if (d3dDevice->CreatePlacedResource1(rtHeapInfo.d3dResourceHeap, rtHeapInfo.iOffsetInHeap + (UINT64)i * iIncrementSize,
						&m_d3dResourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}
			else
			{
				//create the upload buffer
				D3D12_HEAP_PROPERTIES d3dHeapProperties{};
				d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
				d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				d3dHeapProperties.CreationNodeMask = 0;
				d3dHeapProperties.VisibleNodeMask = 0;

				for (unsigned int i = 0; i < 1; i++)
				{
					if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}

			//create the resource view
			if (m_bResourceInDescriptorTable)
			{
				for (unsigned int i = 0; i < m_iNumBuffers; i++)
				{
					D3D12_UNORDERED_ACCESS_VIEW_DESC d3dBufferViewDesc{};
					d3dBufferViewDesc.Format = DXGI_FORMAT_UNKNOWN;
					d3dBufferViewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
					d3dBufferViewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
					d3dBufferViewDesc.Buffer.FirstElement = 0;
					d3dBufferViewDesc.Buffer.NumElements = m_iNumElements;
					d3dBufferViewDesc.Buffer.StructureByteStride = m_iElementSize;
					d3dBufferViewDesc.Buffer.CounterOffsetInBytes = 0;

					d3dDevice->CreateUnorderedAccessView(m_d3dResource[0], nullptr, &d3dBufferViewDesc, m_d3dCPUDescriptor[i]);
				}
			}

			return true;
		}

		bool Readback(ReadbackBuffer* rtDestinationBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = 0;

			if (iNumBytes == 0) iNumBytes = (UINT64)m_iNumElements * (UINT64)m_iElementSize;
			if (iNumBytes + iDestinationOffset > (UINT64)m_iNumElements * (UINT64)m_iElementSize) return false;

			return rtDestinationBuffer->Readback(m_d3dResource[iIndex], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, iNumBytes, iSourceOffset, iDestinationOffset);
		}

		bool ReadbackAll(ReadbackBuffer* rtDestinationBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			if (iNumBytes == 0) iNumBytes = (UINT64)m_iNumElements * (UINT64)m_iElementSize;
			if (iNumBytes + iDestinationOffset > (UINT64)m_iNumElements * (UINT64)m_iElementSize) return false;

			for (unsigned int i = 0; i < 1; i++)
			{
				if (!(rtDestinationBuffer->Readback(m_d3dResource[i], D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					iNumBytes, iSourceOffset, iDestinationOffset))) return false;
			}

			return true;
		}

		void Bind(UINT iRootParameterIndex, bool bBindToCS = false)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = 0;

			if (bBindToCS)
			{
				d3dCommandList->SetComputeRootUnorderedAccessView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
			else
			{
				d3dCommandList->SetGraphicsRootUnorderedAccessView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
		}

		void Release()
		{
			if (m_d3dResource)
			{
				for (unsigned int i = 0; i < 1; i++)
				{
					if (m_d3dResource[i]) m_d3dResource[i]->Release();
				}
				delete m_d3dResource;
				m_d3dResource = nullptr;
			}
			m_iNumBuffers = 0;

			if (m_d3dCPUDescriptor) delete m_d3dCPUDescriptor;
			m_d3dCPUDescriptor = nullptr;
			if (m_d3dGPUDescriptor) delete m_d3dGPUDescriptor;
			m_d3dGPUDescriptor = nullptr;
		}
	};



	class RWTexture1D : public BaseShaderResource
	{
	private:

		//the data of the texture
		D3D12_CPU_DESCRIPTOR_HANDLE* m_d3dCPUDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE* m_d3dGPUDescriptor;
		unsigned int m_iNumTextures;
		unsigned int m_iBytesPerPixel;


	public:

		RWTexture1D() :
			BaseShaderResource(),
			m_d3dCPUDescriptor(nullptr),
			m_d3dGPUDescriptor(nullptr),
			m_iNumTextures(0),
			m_iBytesPerPixel(0) {};
		~RWTexture1D()
		{
			Release();
		};

		bool Initialize(GPUScheduler* rtScheduler, DXGI_FORMAT dxTextureFormat, unsigned int iWidth,
			unsigned int iMipLevels = 1, DescriptorHeapInfo rtDescriptorInfo = {}, ResourceHeapInfo rtHeapInfo = {})
		{
			//initialize member variables
			m_rtScheduler = rtScheduler;
			if (!m_rtScheduler) return false;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iNumTextures = m_rtScheduler->GetNumMaxTasks();
			if (m_iNumTextures < 1) return false;
			m_d3dResource = new ID3D12Resource2 * [m_iNumTextures];
			if (!m_d3dResource) return false;
			m_iBytesPerPixel = (unsigned int)DirectX::BitsPerPixel(dxTextureFormat) / 8;
			m_bResourceInDescriptorTable = (rtDescriptorInfo.d3dDescriptorHeap != nullptr);

			//initialize the descriptor handles
			if (m_bResourceInDescriptorTable)
			{
				m_d3dCPUDescriptor = new D3D12_CPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dCPUDescriptor) return false;
				m_d3dGPUDescriptor = new D3D12_GPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dGPUDescriptor) return false;

				unsigned int iDescriptorIncrementSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					m_d3dCPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart();
					m_d3dGPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetGPUDescriptorHandleForHeapStart();
					m_d3dCPUDescriptor[i].ptr += (SIZE_T)iDescriptorIncrementSize * (SIZE_T)(rtDescriptorInfo.iOffsetInDescriptor);
					m_d3dGPUDescriptor[i].ptr += (UINT64)iDescriptorIncrementSize * (UINT64)(rtDescriptorInfo.iOffsetInDescriptor);
				}
			}

			//fill in the resource desc
			m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			m_d3dResourceDesc.Format = dxTextureFormat;
			m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			m_d3dResourceDesc.Width = (UINT64)iWidth;
			m_d3dResourceDesc.Height = 1;
			m_d3dResourceDesc.DepthOrArraySize = 1;
			m_d3dResourceDesc.MipLevels = (UINT16)iMipLevels;
			m_d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			m_d3dResourceDesc.SampleDesc.Count = 1;
			m_d3dResourceDesc.SampleDesc.Quality = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

			//create the resource
			if (rtHeapInfo.d3dResourceHeap)
			{
				const UINT64 iIncrementSize = (UINT64)(((m_iBytesPerPixel * GetResourceSize()) + 65535) & ~65535);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreatePlacedResource1(rtHeapInfo.d3dResourceHeap, rtHeapInfo.iOffsetInHeap + (UINT64)i * iIncrementSize,
						&m_d3dResourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}
			else
			{
				D3D12_HEAP_PROPERTIES d3dHeapProperties{};
				d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
				d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				d3dHeapProperties.CreationNodeMask = 0;
				d3dHeapProperties.VisibleNodeMask = 0;

				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}

			//create the resource view
			if (m_bResourceInDescriptorTable)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					D3D12_UNORDERED_ACCESS_VIEW_DESC d3dTextureViewDesc{};
					d3dTextureViewDesc.Format = GetFormat();
					d3dTextureViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
					d3dTextureViewDesc.Texture1D.MipSlice = 0;

					d3dDevice->CreateUnorderedAccessView(m_d3dResource[i], nullptr, &d3dTextureViewDesc, m_d3dCPUDescriptor[i]);
				}
			}

			return true;
		}

		bool Readback(ReadbackBuffer* rtDestinationBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			return rtDestinationBuffer->Readback(m_d3dResource[iIndex], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, iNumBytes, iSourceOffset, iDestinationOffset);
		}

		bool ReadbackAll(ReadbackBuffer* rtDestinationBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			for (unsigned int i = 0; i < m_iNumTextures; i++)
			{
				if (!(rtDestinationBuffer->Readback(m_d3dResource[i], D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					iNumBytes, iSourceOffset, iDestinationOffset))) return false;
			}

			return true;
		}

		void Bind(UINT iRootParameterIndex, bool bBindToCS = false)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			if (bBindToCS)
			{
				d3dCommandList->SetComputeRootUnorderedAccessView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
			else
			{
				d3dCommandList->SetGraphicsRootUnorderedAccessView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
		}

		void Release()
		{
			if (m_d3dResource)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (m_d3dResource[i]) m_d3dResource[i]->Release();
				}
				delete m_d3dResource;
				m_d3dResource = nullptr;
			}
			m_iNumTextures = 0;

			if (m_d3dCPUDescriptor) delete m_d3dCPUDescriptor;
			m_d3dCPUDescriptor = nullptr;
			if (m_d3dGPUDescriptor) delete m_d3dGPUDescriptor;
			m_d3dGPUDescriptor = nullptr;
		}
	};



	class RWTexture1DArray : public BaseShaderResource
	{
	private:

		//the data of the texture
		D3D12_CPU_DESCRIPTOR_HANDLE* m_d3dCPUDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE* m_d3dGPUDescriptor;
		unsigned int m_iNumTextures;
		unsigned int m_iBytesPerPixel;


	public:

		RWTexture1DArray() :
			BaseShaderResource(),
			m_d3dCPUDescriptor(nullptr),
			m_d3dGPUDescriptor(nullptr),
			m_iNumTextures(0),
			m_iBytesPerPixel(0) {};
		~RWTexture1DArray()
		{
			Release();
		};

		bool Initialize(GPUScheduler* rtScheduler, DXGI_FORMAT dxTextureFormat, unsigned int iWidth, unsigned int iArraySize,
			unsigned int iMipLevels = 1, DescriptorHeapInfo rtDescriptorInfo = {}, ResourceHeapInfo rtHeapInfo = {})
		{
			//initialize member variables
			m_rtScheduler = rtScheduler;
			if (!m_rtScheduler) return false;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iNumTextures = m_rtScheduler->GetNumMaxTasks();
			if (m_iNumTextures < 1) return false;
			m_d3dResource = new ID3D12Resource2 * [m_iNumTextures];
			if (!m_d3dResource) return false;
			m_iBytesPerPixel = (unsigned int)DirectX::BitsPerPixel(dxTextureFormat) / 8;
			m_bResourceInDescriptorTable = (rtDescriptorInfo.d3dDescriptorHeap != nullptr);

			//initialize the descriptor handles
			if (m_bResourceInDescriptorTable)
			{
				m_d3dCPUDescriptor = new D3D12_CPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dCPUDescriptor) return false;
				m_d3dGPUDescriptor = new D3D12_GPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dGPUDescriptor) return false;

				unsigned int iDescriptorIncrementSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					m_d3dCPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart();
					m_d3dGPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetGPUDescriptorHandleForHeapStart();
					m_d3dCPUDescriptor[i].ptr += (SIZE_T)iDescriptorIncrementSize * (SIZE_T)(rtDescriptorInfo.iOffsetInDescriptor);
					m_d3dGPUDescriptor[i].ptr += (UINT64)iDescriptorIncrementSize * (UINT64)(rtDescriptorInfo.iOffsetInDescriptor);
				}
			}

			//fill in the resource desc
			m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			m_d3dResourceDesc.Format = dxTextureFormat;
			m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			m_d3dResourceDesc.Width = (UINT64)iWidth;
			m_d3dResourceDesc.Height = 1;
			m_d3dResourceDesc.DepthOrArraySize = (UINT16)iArraySize;
			m_d3dResourceDesc.MipLevels = (UINT16)iMipLevels;
			m_d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			m_d3dResourceDesc.SampleDesc.Count = 1;
			m_d3dResourceDesc.SampleDesc.Quality = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

			//create the resource
			if (rtHeapInfo.d3dResourceHeap)
			{
				const UINT64 iIncrementSize = (UINT64)(((m_iBytesPerPixel * GetResourceSize()) + 65535) & ~65535);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreatePlacedResource1(rtHeapInfo.d3dResourceHeap, rtHeapInfo.iOffsetInHeap + (UINT64)i * iIncrementSize,
						&m_d3dResourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}
			else
			{
				D3D12_HEAP_PROPERTIES d3dHeapProperties{};
				d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
				d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				d3dHeapProperties.CreationNodeMask = 0;
				d3dHeapProperties.VisibleNodeMask = 0;

				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}

			//create the resource view
			if (m_bResourceInDescriptorTable)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					D3D12_UNORDERED_ACCESS_VIEW_DESC d3dTextureViewDesc{};
					d3dTextureViewDesc.Format = GetFormat();
					d3dTextureViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
					d3dTextureViewDesc.Texture1DArray.ArraySize = GetDepthOrArraySize();
					d3dTextureViewDesc.Texture1DArray.FirstArraySlice = 0;
					d3dTextureViewDesc.Texture1DArray.MipSlice = 0;

					d3dDevice->CreateUnorderedAccessView(m_d3dResource[i], nullptr, &d3dTextureViewDesc, m_d3dCPUDescriptor[i]);
				}
			}

			return true;
		}

		bool Readback(ReadbackBuffer* rtDestinationBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			return rtDestinationBuffer->Readback(m_d3dResource[iIndex], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, iNumBytes, iSourceOffset, iDestinationOffset);
		}

		bool ReadbackAll(ReadbackBuffer* rtDestinationBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			for (unsigned int i = 0; i < m_iNumTextures; i++)
			{
				if (!(rtDestinationBuffer->Readback(m_d3dResource[i], D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					iNumBytes, iSourceOffset, iDestinationOffset))) return false;
			}

			return true;
		}

		void Bind(UINT iRootParameterIndex, bool bBindToCS = false)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			if (bBindToCS)
			{
				d3dCommandList->SetComputeRootUnorderedAccessView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
			else
			{
				d3dCommandList->SetGraphicsRootUnorderedAccessView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
		}

		void Release()
		{
			if (m_d3dResource)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (m_d3dResource[i]) m_d3dResource[i]->Release();
				}
				delete m_d3dResource;
				m_d3dResource = nullptr;
			}
			m_iNumTextures = 0;

			if (m_d3dCPUDescriptor) delete m_d3dCPUDescriptor;
			m_d3dCPUDescriptor = nullptr;
			if (m_d3dGPUDescriptor) delete m_d3dGPUDescriptor;
			m_d3dGPUDescriptor = nullptr;
		}
	};



	class RWTexture2D : public BaseShaderResource
	{
	private:

		//the data of the texture
		D3D12_CPU_DESCRIPTOR_HANDLE* m_d3dCPUDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE* m_d3dGPUDescriptor;
		unsigned int m_iNumTextures;
		unsigned int m_iBytesPerPixel;


	public:

		RWTexture2D() :
			BaseShaderResource(),
			m_d3dCPUDescriptor(nullptr),
			m_d3dGPUDescriptor(nullptr),
			m_iNumTextures(0),
			m_iBytesPerPixel(0) {};
		~RWTexture2D()
		{
			Release();
		};

		bool Initialize(GPUScheduler* rtScheduler, DXGI_FORMAT dxTextureFormat, unsigned int iWidth, unsigned int iHeight,
			unsigned int iMipLevels = 1, DescriptorHeapInfo rtDescriptorInfo = {}, ResourceHeapInfo rtHeapInfo = {})
		{
			//create the member variables
			m_rtScheduler = rtScheduler;
			if (!m_rtScheduler) return false;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iNumTextures = m_rtScheduler->GetNumMaxTasks();
			if (m_iNumTextures < 1) return false;
			m_d3dResource = new ID3D12Resource2 * [m_iNumTextures];
			if (!m_d3dResource) return false;
			m_iBytesPerPixel = (unsigned int)DirectX::BitsPerPixel(dxTextureFormat) / 8;
			m_bResourceInDescriptorTable = (rtDescriptorInfo.d3dDescriptorHeap != nullptr);

			//initialize the descriptor handles
			if (m_bResourceInDescriptorTable)
			{
				m_d3dCPUDescriptor = new D3D12_CPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dCPUDescriptor) return false;
				m_d3dGPUDescriptor = new D3D12_GPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dGPUDescriptor) return false;

				unsigned int iDescriptorIncrementSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					m_d3dCPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart();
					m_d3dGPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetGPUDescriptorHandleForHeapStart();
					m_d3dCPUDescriptor[i].ptr += (SIZE_T)iDescriptorIncrementSize * (SIZE_T)(rtDescriptorInfo.iOffsetInDescriptor);
					m_d3dGPUDescriptor[i].ptr += (UINT64)iDescriptorIncrementSize * (UINT64)(rtDescriptorInfo.iOffsetInDescriptor);
				}
			}

			//fill in the resource desc
			m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			m_d3dResourceDesc.Format = dxTextureFormat;
			m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			m_d3dResourceDesc.Width = (UINT64)iWidth;
			m_d3dResourceDesc.Height = (UINT)iHeight;
			m_d3dResourceDesc.DepthOrArraySize = 1;
			m_d3dResourceDesc.MipLevels = (UINT16)iMipLevels;
			m_d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			m_d3dResourceDesc.SampleDesc.Count = 1;
			m_d3dResourceDesc.SampleDesc.Quality = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

			//create the resource
			if (rtHeapInfo.d3dResourceHeap)
			{
				const UINT64 iIncrementSize = (UINT64)(((m_iBytesPerPixel * GetResourceSize()) + 65535) & ~65535);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreatePlacedResource1(rtHeapInfo.d3dResourceHeap, rtHeapInfo.iOffsetInHeap + (UINT64)i * iIncrementSize,
						&m_d3dResourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}
			else
			{
				D3D12_HEAP_PROPERTIES d3dHeapProperties{};
				d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
				d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				d3dHeapProperties.CreationNodeMask = 0;
				d3dHeapProperties.VisibleNodeMask = 0;

				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}

			//create the resource view
			if (m_bResourceInDescriptorTable)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					D3D12_UNORDERED_ACCESS_VIEW_DESC d3dTextureViewDesc{};
					d3dTextureViewDesc.Format = GetFormat();
					d3dTextureViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
					d3dTextureViewDesc.Texture2D.PlaneSlice = 0;
					d3dTextureViewDesc.Texture2D.MipSlice = 0;

					d3dDevice->CreateUnorderedAccessView(m_d3dResource[i], nullptr, &d3dTextureViewDesc, m_d3dCPUDescriptor[i]);
				}
			}

			return true;
		}

		bool Readback(ReadbackBuffer* rtDestinationBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			return rtDestinationBuffer->Readback(m_d3dResource[iIndex], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, iNumBytes, iSourceOffset, iDestinationOffset);
		}

		bool ReadbackAll(ReadbackBuffer* rtDestinationBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			for (unsigned int i = 0; i < m_iNumTextures; i++)
			{
				if (!(rtDestinationBuffer->Readback(m_d3dResource[i], D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					iNumBytes, iSourceOffset, iDestinationOffset))) return false;
			}

			return true;
		}

		void Bind(UINT iRootParameterIndex, bool bBindToCS = false)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			if (bBindToCS)
			{
				d3dCommandList->SetComputeRootUnorderedAccessView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
			else
			{
				d3dCommandList->SetGraphicsRootUnorderedAccessView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
		}

		void Release()
		{
			if (m_d3dResource)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (m_d3dResource[i]) m_d3dResource[i]->Release();
				}
				delete m_d3dResource;
				m_d3dResource = nullptr;
			}
			m_iNumTextures = 0;

			if (m_d3dCPUDescriptor) delete m_d3dCPUDescriptor;
			m_d3dCPUDescriptor = nullptr;
			if (m_d3dGPUDescriptor) delete m_d3dGPUDescriptor;
			m_d3dGPUDescriptor = nullptr;
		}
	};



	class RWTexture2DArray : public BaseShaderResource
	{
	private:

		//the data of the texture
		D3D12_CPU_DESCRIPTOR_HANDLE* m_d3dCPUDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE* m_d3dGPUDescriptor;
		unsigned int m_iNumTextures;
		unsigned int m_iBytesPerPixel;


	public:

		RWTexture2DArray() :
			BaseShaderResource(),
			m_d3dCPUDescriptor(nullptr),
			m_d3dGPUDescriptor(nullptr),
			m_iNumTextures(0),
			m_iBytesPerPixel(0) {};
		~RWTexture2DArray()
		{
			Release();
		};

		bool Initialize(GPUScheduler* rtScheduler, DXGI_FORMAT dxTextureFormat, unsigned int iWidth, unsigned int iHeight,
			unsigned int iArraySize, unsigned int iMipLevels = 1, DescriptorHeapInfo rtDescriptorInfo = {}, ResourceHeapInfo rtHeapInfo = {})
		{
			//initialize the variables
			m_rtScheduler = rtScheduler;
			if (!m_rtScheduler) return false;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iNumTextures = m_rtScheduler->GetNumMaxTasks();
			if (m_iNumTextures < 1) return false;
			m_d3dResource = new ID3D12Resource2 * [m_iNumTextures];
			if (!m_d3dResource) return false;
			m_iBytesPerPixel = (unsigned int)DirectX::BitsPerPixel(dxTextureFormat) / 8;
			m_bResourceInDescriptorTable = (rtDescriptorInfo.d3dDescriptorHeap != nullptr);

			//initialize the descriptor handles
			if (m_bResourceInDescriptorTable)
			{
				m_d3dCPUDescriptor = new D3D12_CPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dCPUDescriptor) return false;
				m_d3dGPUDescriptor = new D3D12_GPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dGPUDescriptor) return false;

				unsigned int iDescriptorIncrementSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					m_d3dCPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart();
					m_d3dGPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetGPUDescriptorHandleForHeapStart();
					m_d3dCPUDescriptor[i].ptr += (SIZE_T)iDescriptorIncrementSize * (SIZE_T)(rtDescriptorInfo.iOffsetInDescriptor);
					m_d3dGPUDescriptor[i].ptr += (UINT64)iDescriptorIncrementSize * (UINT64)(rtDescriptorInfo.iOffsetInDescriptor);
				}
			}

			//fill the resource desc
			m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			m_d3dResourceDesc.Format = dxTextureFormat;
			m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			m_d3dResourceDesc.Width = (UINT64)iWidth;
			m_d3dResourceDesc.Height = (UINT)iHeight;
			m_d3dResourceDesc.DepthOrArraySize = (UINT16)iArraySize;
			m_d3dResourceDesc.MipLevels = (UINT16)iMipLevels;
			m_d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			m_d3dResourceDesc.SampleDesc.Count = 1;
			m_d3dResourceDesc.SampleDesc.Quality = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

			//create the resource
			if (rtHeapInfo.d3dResourceHeap)
			{
				const UINT64 iIncrementSize = (UINT64)(((m_iBytesPerPixel * GetResourceSize()) + 65535) & ~65535);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreatePlacedResource1(rtHeapInfo.d3dResourceHeap, rtHeapInfo.iOffsetInHeap + (UINT64)i * iIncrementSize,
						&m_d3dResourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}
			else
			{
				D3D12_HEAP_PROPERTIES d3dHeapProperties{};
				d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
				d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				d3dHeapProperties.CreationNodeMask = 0;
				d3dHeapProperties.VisibleNodeMask = 0;

				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}

			//create the resource view
			if (m_bResourceInDescriptorTable)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					D3D12_UNORDERED_ACCESS_VIEW_DESC d3dTextureViewDesc{};
					d3dTextureViewDesc.Format = GetFormat();
					d3dTextureViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
					d3dTextureViewDesc.Texture2DArray.ArraySize = GetDepthOrArraySize();
					d3dTextureViewDesc.Texture2DArray.FirstArraySlice = 0;
					d3dTextureViewDesc.Texture2DArray.PlaneSlice = 0;
					d3dTextureViewDesc.Texture2DArray.MipSlice = 0;

					d3dDevice->CreateUnorderedAccessView(m_d3dResource[i], nullptr, &d3dTextureViewDesc, m_d3dCPUDescriptor[i]);
				}
			}

			return true;
		}

		bool Readback(ReadbackBuffer* rtDestinationBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			return rtDestinationBuffer->Readback(m_d3dResource[iIndex], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, iNumBytes, iSourceOffset, iDestinationOffset);
		}

		bool ReadbackAll(ReadbackBuffer* rtDestinationBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			for (unsigned int i = 0; i < m_iNumTextures; i++)
			{
				if (!(rtDestinationBuffer->Readback(m_d3dResource[i], D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					iNumBytes, iSourceOffset, iDestinationOffset))) return false;
			}

			return true;
		}

		void Bind(UINT iRootParameterIndex, bool bBindToCS = false)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			if (bBindToCS)
			{
				d3dCommandList->SetComputeRootUnorderedAccessView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
			else
			{
				d3dCommandList->SetGraphicsRootUnorderedAccessView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
		}

		void Release()
		{
			if (m_d3dResource)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (m_d3dResource[i]) m_d3dResource[i]->Release();
				}
				delete m_d3dResource;
				m_d3dResource = nullptr;
			}
			m_iNumTextures = 0;

			if (m_d3dCPUDescriptor) delete m_d3dCPUDescriptor;
			m_d3dCPUDescriptor = nullptr;
			if (m_d3dGPUDescriptor) delete m_d3dGPUDescriptor;
			m_d3dGPUDescriptor = nullptr;
		}
	};



	class RWTexture3D : public BaseShaderResource
	{
	private:

		//the data of the texture
		D3D12_CPU_DESCRIPTOR_HANDLE* m_d3dCPUDescriptor;
		D3D12_GPU_DESCRIPTOR_HANDLE* m_d3dGPUDescriptor;
		unsigned int m_iNumTextures;
		unsigned int m_iBytesPerPixel;


	public:

		RWTexture3D() :
			BaseShaderResource(),
			m_d3dCPUDescriptor(nullptr),
			m_d3dGPUDescriptor(nullptr),
			m_iNumTextures(0),
			m_iBytesPerPixel(0) {};
		~RWTexture3D()
		{
			Release();
		};

		bool Initialize(GPUScheduler* rtScheduler, DXGI_FORMAT dxTextureFormat, unsigned int iWidth, unsigned int iHeight,
			unsigned int iDepth, unsigned int iMipLevels = 1, DescriptorHeapInfo rtDescriptorInfo = {}, ResourceHeapInfo rtHeapInfo = {})
		{
			//initialize member variables
			m_rtScheduler = rtScheduler;
			if (!m_rtScheduler) return false;
			ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
			m_iNumTextures = m_rtScheduler->GetNumMaxTasks();
			if (m_iNumTextures < 1) return false;
			m_d3dResource = new ID3D12Resource2 * [m_iNumTextures];
			if (!m_d3dResource) return false;
			m_iBytesPerPixel = (unsigned int)DirectX::BitsPerPixel(dxTextureFormat) / 8;
			m_bResourceInDescriptorTable = (rtDescriptorInfo.d3dDescriptorHeap != nullptr);

			//initialize the descriptor handles
			if (m_bResourceInDescriptorTable)
			{
				m_d3dCPUDescriptor = new D3D12_CPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dCPUDescriptor) return false;
				m_d3dGPUDescriptor = new D3D12_GPU_DESCRIPTOR_HANDLE[m_iNumTextures];
				if (!m_d3dGPUDescriptor) return false;

				unsigned int iDescriptorIncrementSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					m_d3dCPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart();
					m_d3dGPUDescriptor[i] = rtDescriptorInfo.d3dDescriptorHeap[i]->GetGPUDescriptorHandleForHeapStart();
					m_d3dCPUDescriptor[i].ptr += (SIZE_T)iDescriptorIncrementSize * (SIZE_T)(rtDescriptorInfo.iOffsetInDescriptor);
					m_d3dGPUDescriptor[i].ptr += (UINT64)iDescriptorIncrementSize * (UINT64)(rtDescriptorInfo.iOffsetInDescriptor);
				}
			}

			//fill in the resource desc
			m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			m_d3dResourceDesc.Format = dxTextureFormat;
			m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			m_d3dResourceDesc.Width = (UINT64)iWidth;
			m_d3dResourceDesc.Height = (UINT)iHeight;
			m_d3dResourceDesc.DepthOrArraySize = (UINT16)iDepth;
			m_d3dResourceDesc.MipLevels = (UINT16)iMipLevels;
			m_d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			m_d3dResourceDesc.SampleDesc.Count = 1;
			m_d3dResourceDesc.SampleDesc.Quality = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
			m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

			//create the resource
			if (rtHeapInfo.d3dResourceHeap)
			{
				const UINT64 iIncrementSize = (UINT64)(((m_iBytesPerPixel * GetResourceSize()) + 65535) & ~65535);
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreatePlacedResource1(rtHeapInfo.d3dResourceHeap, rtHeapInfo.iOffsetInHeap + (UINT64)i * iIncrementSize,
						&m_d3dResourceDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}
			else
			{
				D3D12_HEAP_PROPERTIES d3dHeapProperties{};
				d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
				d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
				d3dHeapProperties.CreationNodeMask = 0;
				d3dHeapProperties.VisibleNodeMask = 0;

				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + i)) < 0) return false;
				}
			}

			//create the resource view
			if (m_bResourceInDescriptorTable)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					D3D12_UNORDERED_ACCESS_VIEW_DESC d3dTextureViewDesc{};
					d3dTextureViewDesc.Format = GetFormat();
					d3dTextureViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
					d3dTextureViewDesc.Texture3D.WSize = GetDepthOrArraySize();
					d3dTextureViewDesc.Texture3D.FirstWSlice = 0;
					d3dTextureViewDesc.Texture3D.MipSlice = 0;

					d3dDevice->CreateUnorderedAccessView(m_d3dResource[i], nullptr, &d3dTextureViewDesc, m_d3dCPUDescriptor[i]);
				}
			}

			return true;
		}

		bool Readback(ReadbackBuffer* rtDestinationBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			return rtDestinationBuffer->Readback(m_d3dResource[iIndex], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, iNumBytes, iSourceOffset, iDestinationOffset);
		}

		bool ReadbackAll(ReadbackBuffer* rtDestinationBuffer, UINT64 iNumBytes = 0, UINT64 iSourceOffset = 0, UINT64 iDestinationOffset = 0)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

			const UINT64 iTextureSize = (UINT64)m_iBytesPerPixel * GetResourceSize();
			if (iNumBytes == 0) iNumBytes = iTextureSize;
			if (iNumBytes + iDestinationOffset > iTextureSize) return false;

			for (unsigned int i = 0; i < m_iNumTextures; i++)
			{
				if (!(rtDestinationBuffer->Readback(m_d3dResource[i], D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					iNumBytes, iSourceOffset, iDestinationOffset))) return false;
			}

			return true;
		}

		void Bind(UINT iRootParameterIndex, bool bBindToCS = false)
		{
			ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
			unsigned int iIndex = m_rtScheduler->GetCurrentTaskIndex();

			if (bBindToCS)
			{
				d3dCommandList->SetComputeRootUnorderedAccessView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
			else
			{
				d3dCommandList->SetGraphicsRootUnorderedAccessView(iRootParameterIndex, m_d3dResource[iIndex]->GetGPUVirtualAddress());
			}
		}

		void Release()
		{
			if (m_d3dResource)
			{
				for (unsigned int i = 0; i < m_iNumTextures; i++)
				{
					if (m_d3dResource[i]) m_d3dResource[i]->Release();
				}
				delete m_d3dResource;
				m_d3dResource = nullptr;
			}
			m_iNumTextures = 0;

			if (m_d3dCPUDescriptor) delete m_d3dCPUDescriptor;
			m_d3dCPUDescriptor = nullptr;
			if (m_d3dGPUDescriptor) delete m_d3dGPUDescriptor;
			m_d3dGPUDescriptor = nullptr;
		}
	};
}