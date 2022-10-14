#pragma once

#include <vector>

#include "GPUScheduler.h"
#include "ShaderResources.h"
#include "RaytracerMesh.h"



namespace RT::GraphicsAPI
{

	struct TextureID
	{
		uint32_t Offset;
		uint32_t Width;
		uint32_t Height;
		uint32_t RowPitch;
	};


	class TextureAtlas
	{
	private:

		//declare variables, which store some useful data
		GPUScheduler* m_rtScheduler;
		std::vector<TextureID>	m_stdTextureIDs;
		std::vector<TextureInfo> m_stdTextureData;
		ID3D12Resource2* m_d3dTextureAtlas;
		StructuredBuffer* m_rtTextureIDs;
		unsigned int	m_iBufferSize;


		//private functions

	public: // = usable outside of the class

		//constructor and destructor
		TextureAtlas();
		~TextureAtlas();


		//class functions
		bool AddTexture(uint32_t* iTextureID, TextureInfo rtProperties);
		bool Initialize(GPUScheduler* rtScheduler, DescriptorHeapInfo rtDescriptorHeapInfo);
		void Bind(UINT iTextureIDsRootParameterIndex, bool bBindToCS = false);


		//helper functions
		unsigned int GetTextureCount() { return m_stdTextureIDs.size(); };

		TextureID* GetTextureIDs() { return m_stdTextureIDs.data(); };
		unsigned int GetTextureIDCount() { return m_stdTextureIDs.size(); };

	};

}