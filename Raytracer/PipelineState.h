#pragma once

#include <string>
#include <fstream>

//include our dx12 device wrapper
#include "GPUScheduler.h"
#include "ShaderResources.h"



namespace RT::GraphicsAPI
{

	class PipelineState
	{
	private:

		//static member variables
		GPUScheduler*	m_rtScheduler;
		ID3D12PipelineState*	m_d3dPipelineStateObject;
		D3D12_VIEWPORT	m_d3dViewport;
		D3D12_RECT		m_d3dScissorRect;
		bool m_bComputeState;
		union
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC m_d3dGraphicsState;
			D3D12_COMPUTE_PIPELINE_STATE_DESC m_d3dComputeState;
		};



	public: // = usable outside of the class

		//constructor and destructor
		PipelineState();
		~PipelineState();


		//public class functions
		void Initialize(GPUScheduler* rtScheduler, bool bComputeState = false);
		void SetRootSignature(ID3D12RootSignature* d3dRootSignature);
		bool SetRootSignature(RootSignature& rtRootSignature, bool bAllowInputLayout = false, bool bAllowStreamOutput = false);
		void SetStreamOutput(D3D12_STREAM_OUTPUT_DESC d3dStreamOutput);
		void SetStreamOutput(UINT iNumEntries = 0, UINT iNumStrides = 0, UINT* pBufferStrides = nullptr);
		void SetRasterizerState(D3D12_RASTERIZER_DESC d3dRasterizerState);
		void SetRasterizerState(bool bWireFrameMode, bool bCullBack, bool bCullFront = false,
			bool bEnableDepthClip = true, bool bEnableConservativeRaster = false);
		void SetDepthStencilState(D3D12_DEPTH_STENCIL_DESC d3dDepthStencilState);
		void SetDepthStencilState(bool bEnableDepth, D3D12_COMPARISON_FUNC d3dDepthComparison = D3D12_COMPARISON_FUNC_LESS, bool bEnableStencil = false);
		void SetBlendState(D3D12_BLEND_DESC d3dBlendState);
		void SetBlendState(bool bEnableAlphaToCoverage = false, bool bEnableIndependentBlend = false);
		void SetGraphicsState(D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dGraphicsPipelineState);
		void SetComputeState(D3D12_COMPUTE_PIPELINE_STATE_DESC d3dComputePipelineState);
		void SetInputLayout(UINT iNumElements, D3D12_INPUT_ELEMENT_DESC* d3dInputElements,
			D3D12_PRIMITIVE_TOPOLOGY_TYPE d3dPrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		void SetRenderTargets(DXGI_FORMAT* dxRTVFormats, UINT iNumRenderTargets = 1, DXGI_FORMAT dxDSVFormat = DXGI_FORMAT_UNKNOWN);
		bool SetVS(const std::string& sShaderFile);
		bool SetHS(const std::string& sShaderFile);
		bool SetDS(const std::string& sShaderFile);
		bool SetGS(const std::string& sShaderFile);
		bool SetPS(const std::string& sShaderFile);
		bool SetCS(const std::string& sShaderFile);
		void SetViewportScissor(D3D12_VIEWPORT d3dViewport, D3D12_RECT d3dScissorRect);
		void SetViewportScissor(FLOAT fWidth, FLOAT fHeight, FLOAT fMinDepth = 0.0f, FLOAT fMaxDepth = 1.0f, FLOAT fTopLeftX = 0.0f, FLOAT fTopLeftY = 0.0f,
			LONG iScissorLeft = 0, LONG iScissorTop = 0, LONG iScissorRight = LONG_MIN, LONG iScissorBottom = LONG_MIN);
		bool CreatePSO();
		void Bind();
		void Release();


		//helper functions
		D3D12_STREAM_OUTPUT_DESC& GetStreamOutputState() { return m_d3dGraphicsState.StreamOutput; };
		D3D12_RASTERIZER_DESC& GetRasterizerState() { return m_d3dGraphicsState.RasterizerState; };
		D3D12_DEPTH_STENCIL_DESC& GetDepthStencilState() { return m_d3dGraphicsState.DepthStencilState; };
		D3D12_BLEND_DESC& GetBlendState() { return m_d3dGraphicsState.BlendState; };
		D3D12_INPUT_LAYOUT_DESC& GetInputLayout() { return m_d3dGraphicsState.InputLayout; };
		D3D12_GRAPHICS_PIPELINE_STATE_DESC& GetGraphicsPipelineState() { return m_d3dGraphicsState; };
		D3D12_COMPUTE_PIPELINE_STATE_DESC& GetComputePipelineState() { return m_d3dComputeState; };
		D3D12_VIEWPORT& GetViewport() { return m_d3dViewport; };
		D3D12_RECT& GetScissorRect() { return m_d3dScissorRect; };

	};
}