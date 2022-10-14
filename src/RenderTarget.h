#pragma once

//include our dx12 device wrapper
#include "GPUScheduler.h"



namespace RT::GraphicsAPI
{

	class RenderTarget
	{
	private:

		//static member variables
		GPUScheduler*	m_rtScheduler;
		ID3D12DescriptorHeap*	m_d3dRTVsDescriptorHeap;
		unsigned int			m_iRTVDescriptorSize;
		ID3D12Resource2**	m_d3dBackBuffers;
		unsigned int		m_iBufferCount;


	public: // = usable outside of the class

		//constructor and destructor
		RenderTarget();
		~RenderTarget();


		//public class functions
		bool Initialize(GPUScheduler* rtScheduler);
		void ClearTargets(float fR = 0.0f, float fG = 0.0f, float fB = 0.0f, float fA = 1.0f);
		void BeginRender(bool bClearTargets = true);
		void EndRender();
		void Release();


		//helper functions

	};
}