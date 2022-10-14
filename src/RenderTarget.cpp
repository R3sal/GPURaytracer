//include-files
#include "RenderTarget.h"



namespace RT::GraphicsAPI
{

	//RenderTarget constructor and destructor
	//constructor: initializes all the variables (at least with "0", "nullptr" or "")
	RenderTarget::RenderTarget() :
		//initialize the class variables
		m_rtScheduler(nullptr),
		m_d3dRTVsDescriptorHeap(nullptr),
		m_iRTVDescriptorSize(0),
		m_d3dBackBuffers(nullptr),
		m_iBufferCount(0)
	{

	}

	//destructor: uninitializes all our pointers
	RenderTarget::~RenderTarget()
	{
		Release();
	}



	//private class functions



	//public class functions
	bool RenderTarget::Initialize(GPUScheduler* rtScheduler)
	{
		//copy the contents of rtDevice
		m_rtScheduler = rtScheduler;
		m_iBufferCount = m_rtScheduler->GetNumMaxTasks();
		DX12Device* rtDevice = m_rtScheduler->GetDX12Device();
		ID3D12Device8* d3dDevice = rtDevice->GetDevice();
		IDXGISwapChain4* dxSwapChain = rtDevice->GetSwapChain();


		//create the render targets
		//create the descriptor heap for the render targets
		D3D12_DESCRIPTOR_HEAP_DESC d3dRTVDescriptorHeapDesc{};
		d3dRTVDescriptorHeapDesc.NodeMask = 0;
		d3dRTVDescriptorHeapDesc.NumDescriptors = m_iBufferCount;
		d3dRTVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		d3dRTVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (d3dDevice->CreateDescriptorHeap(&d3dRTVDescriptorHeapDesc, IID_PPV_ARGS(&m_d3dRTVsDescriptorHeap)) < 0) return false;

		//create the render targets themselves
		m_d3dBackBuffers = new ID3D12Resource2*[m_iBufferCount];
		if (!m_d3dBackBuffers) return false;
		m_iRTVDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE d3dCPUDescriptorHandle = m_d3dRTVsDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		for (unsigned int i = 0; i < m_iBufferCount; i++)
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
			d3dCPUDescriptorHandle.ptr += (SIZE_T)m_iRTVDescriptorSize;
		}


		return true;
	}


	void RenderTarget::ClearTargets(float fR, float fG, float fB, float fA)
	{
		float fColor[4] = { fR, fG, fB, fA };
		D3D12_CPU_DESCRIPTOR_HANDLE d3dRTVDescriptor = m_d3dRTVsDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		d3dRTVDescriptor.ptr += (SIZE_T)(m_rtScheduler->GetCurrentTaskIndex()) * (SIZE_T)m_iRTVDescriptorSize;

		m_rtScheduler->GetCommandList()->ClearRenderTargetView(d3dRTVDescriptor, fColor, 0, nullptr);
	}


	void RenderTarget::BeginRender(bool bClearTargets)
	{
		ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();


		//render a frame
		//make the resource transition of the RT from the present state to the render target state
		D3D12_RESOURCE_BARRIER d3dResourceBarrier{};
		d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		d3dResourceBarrier.Transition.pResource = m_d3dBackBuffers[m_rtScheduler->GetCurrentTaskIndex()];
		d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		d3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

		//set the different components of the pipeline
		D3D12_CPU_DESCRIPTOR_HANDLE d3dRTVDescriptor = m_d3dRTVsDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		d3dRTVDescriptor.ptr += (SIZE_T)(m_rtScheduler->GetCurrentTaskIndex()) * (SIZE_T)m_iRTVDescriptorSize;

		//d3dCommandList->OMSetDepthBounds(0.0f, 1.0f);
		//d3dCommandList->OMSetStencilRef(0);
		//d3dCommandList->OMSetBlendFactor();
		d3dCommandList->OMSetRenderTargets(1, &d3dRTVDescriptor, true, nullptr);

		//clear the targets, if desired
		if (bClearTargets)
		{
			float fClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			d3dCommandList->ClearRenderTargetView(d3dRTVDescriptor, fClearColor, 0, nullptr);
		}
	}


	void RenderTarget::EndRender()
	{
		ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

		//unbind the render target
		//d3dCommandList->OMSetDepthBounds(0.0f, 1.0f);
		//d3dCommandList->OMSetStencilRef(0);
		//d3dCommandList->OMSetBlendFactor();
		d3dCommandList->OMSetRenderTargets(0, nullptr, false, nullptr);

		//make the resource transition of the RT from the present state to the render target state
		D3D12_RESOURCE_BARRIER d3dResourceBarrier{};
		d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		d3dResourceBarrier.Transition.pResource = m_d3dBackBuffers[m_rtScheduler->GetCurrentTaskIndex()];
		d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		d3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);
	}


	void RenderTarget::Release()
	{
		if (m_d3dRTVsDescriptorHeap) m_d3dRTVsDescriptorHeap->Release();
		m_d3dRTVsDescriptorHeap = nullptr;

		if (m_d3dBackBuffers)
		{
			for (unsigned int i = 0; i < m_iBufferCount; i++)
			{
				if (m_d3dBackBuffers[i]) m_d3dBackBuffers[i]->Release();
				m_d3dBackBuffers[i] = nullptr;
			}
		}
	}
}