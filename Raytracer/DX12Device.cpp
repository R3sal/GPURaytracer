//include-files
#include "DX12Device.h"



namespace RT::GraphicsAPI
{

	//initialize the private static members
	IDXGIFactory7* DX12Device::s_dxFactory = nullptr;
	ID3D12Debug3* DX12Device::s_d3dDebugInterface = nullptr;



	//AppWindow constructor and destructor
	//constructor: initializes all the variables (at least with "0", "nullptr" or "")
	DX12Device::DX12Device() :
		//initialize the class variables
		m_d3dDevice(nullptr),
		m_d3dCommandQueue(nullptr),
		m_dxSwapChain(nullptr)
	{

	}

	//destructor: uninitializes all our pointers
	DX12Device::~DX12Device()
	{
		Release();
	}



	//private class functions



	//public class functions
	bool DX12Device::Initialize(WND::Window& rtWindow)
	{
		//initialize the static members
		if (!s_dxFactory)
		{
			if (CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&s_dxFactory)) < 0) return false;
		}
		if (!s_d3dDebugInterface)
		{
			if (D3D12GetDebugInterface(IID_PPV_ARGS(&s_d3dDebugInterface)) < 0) return false;
			s_d3dDebugInterface->EnableDebugLayer();
		}
		
		//initialize all the other members
		if (D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_d3dDevice)) < 0) return false;

		D3D12_COMMAND_QUEUE_DESC d3dQueueDesc{};
		d3dQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		d3dQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;         
		d3dQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		d3dQueueDesc.NodeMask = 0;
		if (m_d3dDevice->CreateCommandQueue(&d3dQueueDesc, IID_PPV_ARGS(&m_d3dCommandQueue)) < 0) return false;
		
		DXGI_SWAP_CHAIN_DESC1 dxSwapChainDesc{};
		dxSwapChainDesc.Width = rtWindow.GetWidth();
		dxSwapChainDesc.Height = rtWindow.GetHeight();
		dxSwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		dxSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		dxSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		dxSwapChainDesc.SampleDesc.Count = 1;
		dxSwapChainDesc.SampleDesc.Quality = 0;
		dxSwapChainDesc.Stereo = false;
		dxSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		dxSwapChainDesc.BufferCount = 3;
		dxSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		dxSwapChainDesc.Flags = 0;

		if (s_dxFactory->CreateSwapChainForHwnd(m_d3dCommandQueue, rtWindow.GetHWND(),
			&dxSwapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)(&m_dxSwapChain)) < 0) return false;

		return true;
	}


	void DX12Device::Release()
	{
		//release all the resources
		if (m_d3dDevice) m_d3dDevice->Release();
		if (m_d3dCommandQueue) m_d3dCommandQueue->Release();
		if (m_dxSwapChain) m_dxSwapChain->Release();

		m_d3dDevice = nullptr;
		m_d3dCommandQueue = nullptr;
		m_dxSwapChain = nullptr;
	}

}