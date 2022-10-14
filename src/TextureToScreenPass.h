#pragma once

#include "GPUDevice.h"
#include "GPUScheduler.h"
#include "RenderTarget.h"
#include "PipelineState.h"
#include "ShaderResources.h"



namespace RT::GraphicsAPI
{

	class TextureToScreenPass
	{
	private:

		//static member variables
		GPUScheduler* m_rtFrameScheduler;
		RenderTarget* m_rtRenderTarget;
		PipelineState* m_rtPipelineState;



	public: // = usable outside of the class

		//constructor and destructor
		TextureToScreenPass();
		~TextureToScreenPass();


		//public class functions
		bool Initialize(GPUScheduler* rtScheduler, unsigned int iWidth, unsigned int iHeight, DXGI_FORMAT dxBackBufferFormat);
		void Render(DescriptorHeap* rtTextureDescriptor, unsigned int iOffsetInDescriptor = 0);
		void Release();


		//helper functions

	};
}