#pragma once

//include our dx12 device wrapper
#include "DX12GPUScheduler.h"



namespace RT::GraphicsAPI
{

	class RenderTarget
	{
	private:

		//static member variables
		GPUScheduler*	m_rtScheduler;
		ID3D12DescriptorHeap*	m_d3dRTVsDescriptorHeap;
		unsigned int				m_iRTVDescriptorSize;
		ID3D12Resource2**	m_d3dBackBuffers;
		unsigned int			m_iBufferCount;
		unsigned int			m_iCurrentBackBuffer;



	public: // = usable outside of the class

		//constructor and destructor
		RenderTarget();
		~RenderTarget();


		//public class functions
		bool Initialize(GPUScheduler& rtScheduler, unsigned int iNumBuffers = 1);
		void ClearTargets(float fR = 0.0f, float fG = 0.0f, float fB = 0.0f, float fA = 1.0f);
		void Bind(bool bClearTargets = true);
		void Unbind();
		void Release();


		//helper functions

	};
}