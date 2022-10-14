//include-files
#include "TextureToScreenPass.h"



namespace RT::GraphicsAPI
{

	//constructor and destructor
	//constructor: initializes all the variables (at least with "0", "nullptr" or "")
	TextureToScreenPass::TextureToScreenPass() :
		//initialize the class variables
		m_rtFrameScheduler(nullptr),
		m_rtRenderTarget(nullptr),
		m_rtPipelineState(nullptr)
	{

	}

	//destructor: uninitializes all our pointers
	TextureToScreenPass::~TextureToScreenPass()
	{
		Release();
	}



	//private class functions



	//public class functions
	bool TextureToScreenPass::Initialize(GPUScheduler* rtScheduler, unsigned int iWidth, unsigned int iHeight, DXGI_FORMAT dxBackBufferFormat)
	{
		m_rtFrameScheduler = rtScheduler;
		m_rtRenderTarget = new RenderTarget();
		m_rtPipelineState = new PipelineState();
		if (!m_rtRenderTarget) return false;
		if (!m_rtPipelineState) return false;

		ID3D12Device8* d3dDevice = m_rtFrameScheduler->GetDX12Device()->GetDevice();
		ID3D12RootSignature* d3dRootSignature = nullptr;
		RootSignature rtPassSignature = RootSignature();
		DescriptorTable rtDescriptor = DescriptorTable();
		rtDescriptor.AddUAVRange(0, 0, 1);
		if (!(rtPassSignature.AddDescriptorTable(rtDescriptor, ShaderStagePS))) return false;
		if (!(rtPassSignature.GenerateRootSignature(d3dDevice, &d3dRootSignature))) return false;

		m_rtPipelineState->Initialize(m_rtFrameScheduler);
		m_rtPipelineState->SetViewportScissor((FLOAT)iWidth, (FLOAT)iHeight);
		m_rtPipelineState->SetRootSignature(d3dRootSignature);
		m_rtPipelineState->SetRenderTargets(&dxBackBufferFormat);
		if (!(m_rtPipelineState->SetVS("shader/shaderbin/VS_FullscreenPass.cso"))) return false;
		if (!(m_rtPipelineState->SetPS("shader/shaderbin/PS_FullscreenPass.cso"))) return false;
		if (!(m_rtPipelineState->CreatePSO())) return false;
		
		if (!(m_rtRenderTarget->Initialize(m_rtFrameScheduler))) return false;

		return true;
	}


	void TextureToScreenPass::Render(DescriptorHeap* rtTextureDescriptor, unsigned int iOffsetInDescriptor)
	{
		m_rtRenderTarget->BeginRender();
		m_rtPipelineState->Bind();
		rtTextureDescriptor->Bind(0, iOffsetInDescriptor);
		
		ID3D12GraphicsCommandList6* d3dCommandList = m_rtFrameScheduler->GetCommandList();
		d3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		d3dCommandList->IASetVertexBuffers(0, 1, nullptr);
		d3dCommandList->DrawInstanced(3, 1, 0, 0);

		m_rtRenderTarget->EndRender();
	}


	void TextureToScreenPass::Release()
	{
		if (m_rtRenderTarget) m_rtRenderTarget->Release();
		if (m_rtPipelineState) m_rtPipelineState->Release();

		m_rtFrameScheduler = nullptr;
		m_rtRenderTarget = nullptr;
		m_rtPipelineState = nullptr;
	}
}