#pragma once

//include our dx12 device wrapper
#include "DX12GPUScheduler.h"



namespace RT::GraphicsAPI
{

	class PipelineState
	{
	private:

		//static member variables
		GPUScheduler*	m_rtScheduler;
		D3D12_VIEWPORT		m_d3dViewport;
		D3D12_RECT			m_d3dScissorRect;
		ID3D12RootSignature*		m_d3dRootSignature;
		ID3D12PipelineState*		m_d3dPipelineStateObject;



	public: // = usable outside of the class

		//constructor and destructor
		PipelineState();
		~PipelineState();


		//public class functions
		bool Initialize(GPUScheduler& rtScheduler, ID3D12RootSignature* d3dRootSignature);
		void Release();


		//helper functions

	};
}