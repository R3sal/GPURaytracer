#pragma once

//include the C++ file stream for loading teh shader files
#include <fstream>
#include <iostream>

//include the dx12 devcie class
#include "DX12Device.h"
#include "DX12GPUScheduler.h"
#include "DX12RenderTarget.h"



namespace RT::GraphicsAPI
{

	class RaytracingPipeline
	{
	private:

		//static variables
		static const unsigned int s_iNumBuffers = 3;


		//private member variables
		DX12Device*		m_rtDevice;
		GPUScheduler*	m_rtFrameScheduler;
		GPUScheduler*	m_rtComputeScheduler;
		RenderTarget*	m_rtRenderTarget;
		ID3D12DescriptorHeap* m_d3dUAVDescriptorHeap;
		D3D12_VIEWPORT			m_d3dViewport;
		D3D12_RECT				m_d3dScissorRect;
		ID3D12PipelineState* m_d3dPipelineState;
		ID3D12PipelineState* m_d3dComputePipelineState;
		ID3D12RootSignature* m_d3dRootSignature;
		ID3D12RootSignature* m_d3dComputeRootSignature;
		ID3D12Resource2* m_d3dIntersectionTarget;
		ID3D12Resource2* m_d3dVertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW		m_d3dVertexBufferView;


		//private functions


	public: // = usable outside of the class

		//constructor and destructor
		RaytracingPipeline();
		~RaytracingPipeline();


		//public class functions
		bool Initialize(DX12Device& rtDevice);

		//render a frame
		bool Render();


		//helper functions

	};
}