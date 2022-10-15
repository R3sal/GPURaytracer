#pragma once

//include the glfw and dxgi headers
#include <dxgi1_6.h>
#include <d3d12.h>

//include the window class
#include "GLWindow.h"

#define RT_NUM_BUFFERS 3



namespace RT::GraphicsAPI
{

	class DX12Device
	{
	private:

		//static member variables
		static IDXGIFactory7*	s_dxFactory;
		static ID3D12Debug3*	s_d3dDebugInterface;


		//private member variables
		ID3D12Device8*		m_d3dDevice;
		ID3D12CommandQueue*	m_d3dCommandQueue;
		IDXGISwapChain4*	m_dxSwapChain;

		//adapter info
		D3D12_FEATURE_DATA_D3D12_OPTIONS m_d3dFeatureOptions;
		


	public: // = usable outside of the class

		//constructor and destructor
		DX12Device();
		~DX12Device();


		//public class functions
		bool Initialize(WND::Window* rtWindow);
		void Release();


		//helper functions
		//static IDXGIFactory7* GetFactory() { return m_dxFactory; };
		//static ID3D12Debug3* GetDebugInterface() { return m_d3dDebugInterface; };
		ID3D12Device8* GetDevice() { return m_d3dDevice; };
		ID3D12CommandQueue* GetCommandQueue() { return m_d3dCommandQueue; };
		IDXGISwapChain4* GetSwapChain() { return m_dxSwapChain; };

		const D3D12_FEATURE_DATA_D3D12_OPTIONS& GetFeatureOptions() { return m_d3dFeatureOptions; };

	};
}