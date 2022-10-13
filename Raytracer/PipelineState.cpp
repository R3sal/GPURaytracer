//include-files
#include "PipelineState.h"



namespace RT::GraphicsAPI
{
	//load a shader
	bool LoadShader(const std::string& sShaderFile, char** pShaderData, SIZE_T& iShaderSize)
	{
		std::ifstream stdShaderCodeStream = std::ifstream();
		stdShaderCodeStream.open(sShaderFile, std::ios::ate | std::ios::binary);
		if (!stdShaderCodeStream.is_open()) return false;
		iShaderSize = (SIZE_T)(stdShaderCodeStream.tellg());
		stdShaderCodeStream.seekg(0, std::ios::beg);
		*pShaderData = new char[iShaderSize];
		stdShaderCodeStream.read(*pShaderData, iShaderSize);
		stdShaderCodeStream.close();

		return true;
	}



	//GPUScheduler constructor and destructor
	//constructor: initializes all the variables (at least with "0", "nullptr" or "")
	PipelineState::PipelineState() :
		//initialize the class variables
		m_rtScheduler(nullptr),
		m_d3dPipelineStateObject(nullptr),
		m_d3dViewport(),
		m_d3dScissorRect(),
		m_bComputeState(false),
		m_d3dGraphicsState()
	{

	}

	//destructor: uninitializes all our pointers
	PipelineState::~PipelineState()
	{
		Release();
	}



	//private class functions



	//public class functions
	void PipelineState::Initialize(GPUScheduler* rtScheduler, bool bComputeState)
	{
		//set the member variables to some default values
		m_rtScheduler = rtScheduler;
		m_bComputeState = bComputeState;

		if (m_bComputeState)
		{
			m_d3dComputeState.CachedPSO.CachedBlobSizeInBytes = 0;
			m_d3dComputeState.CachedPSO.pCachedBlob = nullptr;
			m_d3dComputeState.CS.BytecodeLength = 0;
			m_d3dComputeState.CS.pShaderBytecode = nullptr;
			m_d3dComputeState.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			m_d3dComputeState.pRootSignature = nullptr;
			m_d3dComputeState.NodeMask = 0;
		}
		else
		{
			//before filling out the pipeline state desc, just fill out some of the structures it contains
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
			d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
			d3dRasterizerDesc.FrontCounterClockwise = false;
			d3dRasterizerDesc.DepthClipEnable = false;
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
			m_d3dGraphicsState.NodeMask = 0;
			m_d3dGraphicsState.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			m_d3dGraphicsState.CachedPSO.CachedBlobSizeInBytes = 0;
			m_d3dGraphicsState.CachedPSO.pCachedBlob = nullptr;
			m_d3dGraphicsState.pRootSignature = nullptr;
			m_d3dGraphicsState.InputLayout.NumElements = 0;
			m_d3dGraphicsState.InputLayout.pInputElementDescs = nullptr;
			m_d3dGraphicsState.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			m_d3dGraphicsState.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			m_d3dGraphicsState.VS.BytecodeLength = 0;
			m_d3dGraphicsState.VS.pShaderBytecode = nullptr;
			m_d3dGraphicsState.HS.BytecodeLength = 0;
			m_d3dGraphicsState.HS.pShaderBytecode = nullptr;
			m_d3dGraphicsState.DS.BytecodeLength = 0;
			m_d3dGraphicsState.DS.pShaderBytecode = nullptr;
			m_d3dGraphicsState.GS.BytecodeLength = 0;
			m_d3dGraphicsState.GS.pShaderBytecode = nullptr;
			m_d3dGraphicsState.PS.BytecodeLength = 0;
			m_d3dGraphicsState.PS.pShaderBytecode = nullptr;
			m_d3dGraphicsState.DSVFormat = DXGI_FORMAT_UNKNOWN;
			m_d3dGraphicsState.NumRenderTargets = 1;
			m_d3dGraphicsState.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			m_d3dGraphicsState.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
			m_d3dGraphicsState.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
			m_d3dGraphicsState.RTVFormats[3] = DXGI_FORMAT_UNKNOWN;
			m_d3dGraphicsState.RTVFormats[4] = DXGI_FORMAT_UNKNOWN;
			m_d3dGraphicsState.RTVFormats[5] = DXGI_FORMAT_UNKNOWN;
			m_d3dGraphicsState.RTVFormats[6] = DXGI_FORMAT_UNKNOWN;
			m_d3dGraphicsState.RTVFormats[7] = DXGI_FORMAT_UNKNOWN;
			m_d3dGraphicsState.StreamOutput = d3dStreamOutputDesc;
			m_d3dGraphicsState.RasterizerState = d3dRasterizerDesc;
			m_d3dGraphicsState.DepthStencilState = d3dDepthStencilDesc;
			m_d3dGraphicsState.BlendState = d3dBlendDesc;
			m_d3dGraphicsState.SampleDesc.Count = 1;
			m_d3dGraphicsState.SampleDesc.Quality = 0;
			m_d3dGraphicsState.SampleMask = 0xffffffff;
		}
	}


	void PipelineState::SetRootSignature(ID3D12RootSignature* d3dRootSignature)
	{
		if (m_bComputeState)
		{
			if (m_d3dComputeState.pRootSignature) m_d3dComputeState.pRootSignature->Release();
			m_d3dComputeState.pRootSignature = d3dRootSignature;
		}
		else
		{
			if (m_d3dGraphicsState.pRootSignature) m_d3dGraphicsState.pRootSignature->Release();
			m_d3dGraphicsState.pRootSignature = d3dRootSignature;
		}
	}


	bool PipelineState::SetRootSignature(RootSignature& rtRootSignature, bool bAllowInputLayout, bool bAllowStreamOutput)
	{
		ID3D12RootSignature* d3dRootSignature = nullptr;
		if (!(rtRootSignature.GenerateRootSignature(m_rtScheduler->GetDX12Device()->GetDevice(),
			&d3dRootSignature, bAllowInputLayout, bAllowStreamOutput))) return false;

		if (m_bComputeState)
		{
			if (m_d3dComputeState.pRootSignature) m_d3dComputeState.pRootSignature->Release();
			m_d3dComputeState.pRootSignature = d3dRootSignature;
		}
		else
		{
			if (m_d3dGraphicsState.pRootSignature) m_d3dGraphicsState.pRootSignature->Release();
			m_d3dGraphicsState.pRootSignature = d3dRootSignature;
		}

		return true;
	}


	void PipelineState::SetStreamOutput(D3D12_STREAM_OUTPUT_DESC d3dStreamOutput)
	{
		if (!m_bComputeState) m_d3dGraphicsState.StreamOutput = d3dStreamOutput;
	}


	void PipelineState::SetStreamOutput(UINT iNumEntries, UINT iNumStrides, UINT* pBufferStrides)
	{
		D3D12_STREAM_OUTPUT_DESC d3dStreamOutputDesc{};
		d3dStreamOutputDesc.NumEntries = iNumEntries;
		d3dStreamOutputDesc.NumStrides = iNumStrides;
		d3dStreamOutputDesc.pBufferStrides = pBufferStrides;
		d3dStreamOutputDesc.pSODeclaration = nullptr;
		d3dStreamOutputDesc.RasterizedStream = 0;

		SetStreamOutput(d3dStreamOutputDesc);
	}


	void PipelineState::SetRasterizerState(D3D12_RASTERIZER_DESC d3dRasterizerState)
	{
		if (!m_bComputeState) m_d3dGraphicsState.RasterizerState = d3dRasterizerState;
	}


	void PipelineState::SetRasterizerState(bool bWireFrameMode, bool bCullBack, bool bCullFront, bool bEnableDepthClip, bool bEnableConservativeRaster)
	{
		D3D12_RASTERIZER_DESC d3dRasterizerDesc{};
		d3dRasterizerDesc.FillMode = bWireFrameMode ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
		d3dRasterizerDesc.CullMode = bCullBack ? D3D12_CULL_MODE_BACK : (bCullFront ? D3D12_CULL_MODE_FRONT : D3D12_CULL_MODE_NONE);
		d3dRasterizerDesc.FrontCounterClockwise = false;
		d3dRasterizerDesc.DepthClipEnable = bEnableDepthClip;
		d3dRasterizerDesc.DepthBias = 0;
		d3dRasterizerDesc.DepthBiasClamp = 0.0f;
		d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
		d3dRasterizerDesc.ConservativeRaster = bEnableConservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		d3dRasterizerDesc.AntialiasedLineEnable = false;
		d3dRasterizerDesc.MultisampleEnable = false;
		d3dRasterizerDesc.ForcedSampleCount = 0;

		SetRasterizerState(d3dRasterizerDesc);
	}


	void PipelineState::SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC d3dDepthStencilState)
	{
		if (!m_bComputeState) m_d3dGraphicsState.DepthStencilState = d3dDepthStencilState;
	}


	void PipelineState::SetDepthStencilState(bool bEnableDepth, D3D12_COMPARISON_FUNC d3dDepthComparison, bool bEnableStencil)
	{
		D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc{};
		d3dDepthStencilDesc.DepthEnable = bEnableDepth;
		d3dDepthStencilDesc.DepthFunc = d3dDepthComparison;
		d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		d3dDepthStencilDesc.StencilEnable = bEnableStencil;
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

		SetDepthStencilState(d3dDepthStencilDesc);
	}


	void PipelineState::SetBlendState(D3D12_BLEND_DESC d3dBlendState)
	{
		if (!m_bComputeState) m_d3dGraphicsState.BlendState = d3dBlendState;
	}


	void PipelineState::SetBlendState(bool bEnableAlphaToCoverage, bool bEnableIndependentBlend)
	{
		D3D12_BLEND_DESC d3dBlendDesc{};
		d3dBlendDesc.AlphaToCoverageEnable = bEnableAlphaToCoverage;
		d3dBlendDesc.IndependentBlendEnable = bEnableIndependentBlend;
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

		SetBlendState(d3dBlendDesc);
	}


	void PipelineState::SetGraphicsState(D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dGraphicsPipelineState)
	{
		if (!m_bComputeState) m_d3dGraphicsState = d3dGraphicsPipelineState;
	}


	void PipelineState::SetComputeState(D3D12_COMPUTE_PIPELINE_STATE_DESC d3dComputePipelineState)
	{
		if (m_bComputeState) m_d3dComputeState = d3dComputePipelineState;
	}


	void PipelineState::SetInputLayout(UINT iNumElements, D3D12_INPUT_ELEMENT_DESC* d3dInputElements, D3D12_PRIMITIVE_TOPOLOGY_TYPE d3dPrimitiveTopology)
	{
		if (!m_bComputeState)
		{
			m_d3dGraphicsState.PrimitiveTopologyType = d3dPrimitiveTopology;
			m_d3dGraphicsState.InputLayout.NumElements = iNumElements;
			m_d3dGraphicsState.InputLayout.pInputElementDescs = d3dInputElements;
		}
	}


	void PipelineState::SetRenderTargets(DXGI_FORMAT* dxRTVFormats, UINT iNumRenderTargets, DXGI_FORMAT dxDSVFormat)
	{
		if (!m_bComputeState)
		{
			m_d3dGraphicsState.NumRenderTargets = iNumRenderTargets;
			m_d3dGraphicsState.DSVFormat = dxDSVFormat;
			for (unsigned int i = 0; i < min(iNumRenderTargets, 8); i++) m_d3dGraphicsState.RTVFormats[i] = dxRTVFormats[i];
			for (unsigned int i = min(iNumRenderTargets, 8); i < 8; i++) m_d3dGraphicsState.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
		}
	}


	bool PipelineState::SetVS(const std::string& sShaderFile)
	{
		if (m_bComputeState) return false;
		
		SIZE_T iShaderSize = 0;
		char* pShaderData = nullptr;
		
		if (!LoadShader(sShaderFile, &pShaderData, iShaderSize)) return false;
		m_d3dGraphicsState.VS.BytecodeLength = iShaderSize;
		m_d3dGraphicsState.VS.pShaderBytecode = pShaderData;

		return true;
	}


	bool PipelineState::SetHS(const std::string& sShaderFile)
	{
		if (m_bComputeState) return false;
		
		SIZE_T iShaderSize = 0;
		char* pShaderData = nullptr;
		
		if (!LoadShader(sShaderFile, &pShaderData, iShaderSize)) return false;
		m_d3dGraphicsState.HS.BytecodeLength = iShaderSize;
		m_d3dGraphicsState.HS.pShaderBytecode = pShaderData;

		return true;
	}


	bool PipelineState::SetDS(const std::string& sShaderFile)
	{
		if (m_bComputeState) return false;
		
		SIZE_T iShaderSize = 0;
		char* pShaderData = nullptr;
		
		if (!LoadShader(sShaderFile, &pShaderData, iShaderSize)) return false;
		m_d3dGraphicsState.DS.BytecodeLength = iShaderSize;
		m_d3dGraphicsState.DS.pShaderBytecode = pShaderData;

		return true;
	}


	bool PipelineState::SetGS(const std::string& sShaderFile)
	{
		if (m_bComputeState) return false;
		
		SIZE_T iShaderSize = 0;
		char* pShaderData = nullptr;
		
		if (!LoadShader(sShaderFile, &pShaderData, iShaderSize)) return false;
		m_d3dGraphicsState.GS.BytecodeLength = iShaderSize;
		m_d3dGraphicsState.GS.pShaderBytecode = pShaderData;

		return true;
	}


	bool PipelineState::SetPS(const std::string& sShaderFile)
	{
		if (m_bComputeState) return false;
		
		SIZE_T iShaderSize = 0;
		char* pShaderData = nullptr;
		
		if (!LoadShader(sShaderFile, &pShaderData, iShaderSize)) return false;
		m_d3dGraphicsState.PS.BytecodeLength = iShaderSize;
		m_d3dGraphicsState.PS.pShaderBytecode = pShaderData;

		return true;
	}


	bool PipelineState::SetCS(const std::string& sShaderFile)
	{
		if (!m_bComputeState) return false;
		
		SIZE_T iShaderSize = 0;
		char* pShaderData = nullptr;
		
		if (!LoadShader(sShaderFile, &pShaderData, iShaderSize)) return false;
		m_d3dComputeState.CS.BytecodeLength = iShaderSize;
		m_d3dComputeState.CS.pShaderBytecode = pShaderData;

		return true;
	}


	void PipelineState::SetViewportScissor(D3D12_VIEWPORT d3dViewport, D3D12_RECT d3dScissorRect)
	{
		m_d3dViewport = d3dViewport;
		m_d3dScissorRect = d3dScissorRect;
	}


	void PipelineState::SetViewportScissor(FLOAT fWidth, FLOAT fHeight, FLOAT fMinDepth, FLOAT fMaxDepth,
		FLOAT fTopLeftX, FLOAT fTopLeftY, LONG iScissorLeft, LONG iScissorTop, LONG iScissorRight, LONG iScissorBottom)
	{
		m_d3dViewport.TopLeftX = fTopLeftX;
		m_d3dViewport.TopLeftY = fTopLeftY;
		m_d3dViewport.Width = fWidth;
		m_d3dViewport.Height = fHeight;
		m_d3dViewport.MinDepth = fMinDepth;
		m_d3dViewport.MaxDepth = fMaxDepth;
		m_d3dScissorRect.left = iScissorLeft;
		m_d3dScissorRect.top = iScissorTop;
		m_d3dScissorRect.right = iScissorRight == LONG_MIN ? (LONG)roundf(fWidth) : iScissorRight;
		m_d3dScissorRect.bottom = iScissorBottom == LONG_MIN ? (LONG)roundf(fHeight) : iScissorBottom;
	}


	bool PipelineState::CreatePSO()
	{
		if (m_d3dPipelineStateObject) m_d3dPipelineStateObject->Release();
		m_d3dPipelineStateObject = nullptr;
		ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();

		if (m_bComputeState)
		{
			if (d3dDevice->CreateComputePipelineState(&m_d3dComputeState, IID_PPV_ARGS(&m_d3dPipelineStateObject)) < 0) return false;
		}
		else
		{
			if (d3dDevice->CreateGraphicsPipelineState(&m_d3dGraphicsState, IID_PPV_ARGS(&m_d3dPipelineStateObject)) < 0) return false;
		}

		return true;
	}


	void PipelineState::Bind()
	{
		ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
		d3dCommandList->SetPipelineState(m_d3dPipelineStateObject);
		if (m_bComputeState)
		{
			d3dCommandList->SetComputeRootSignature(m_d3dComputeState.pRootSignature);
		}
		else
		{
			d3dCommandList->SetGraphicsRootSignature(m_d3dGraphicsState.pRootSignature);
			d3dCommandList->RSSetViewports(1, &m_d3dViewport);
			d3dCommandList->RSSetScissorRects(1, &m_d3dScissorRect);
		}
	}


	void PipelineState::Release()
	{
		SetRootSignature(nullptr);
		if (m_d3dPipelineStateObject) m_d3dPipelineStateObject->Release();
		m_d3dPipelineStateObject = nullptr;
	}
}