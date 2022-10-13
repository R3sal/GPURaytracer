
#include "TextureAtlas.h"



namespace RT::GraphicsAPI
{

	//constructor: initializes all the variables (at least with "0", "nullptr" or "")
	TextureAtlas::TextureAtlas() :
		//initialize the variables
		m_rtScheduler(nullptr),
		m_stdTextureIDs(),
		m_stdTextureData(),
		m_d3dTextureAtlas(nullptr),
		m_rtTextureIDs(nullptr),
		m_iBufferSize(0)
	{
		
	}

	//destructor: uninitializes all our pointers
	TextureAtlas::~TextureAtlas()
	{
		
	}



	//private class functions



	//public class functions
	//add a texture to the arrays
	bool TextureAtlas::AddTexture(uint32_t* iTextureID, TextureInfo rtProperties)
	{
		if (iTextureID)
		{
			*iTextureID = 0;
		}

		//make a default texture for  meshes that don't use any textures
		if (m_iBufferSize == 0)
		{
			TextureID rtTextureID{};
			rtTextureID.Offset = 0;
			rtTextureID.Width = 1;
			rtTextureID.Height = 1;
			rtTextureID.RowPitch = 1;

			TextureInfo rtTextureData{};
			rtTextureData.Data = (void*)(new uint8_t[8]);
			memset(rtTextureData.Data, 0xffffffff, 8);
			rtTextureData.Width = 1;
			rtTextureData.Height = 1;
			rtTextureData.BytesPerChannel = 2;
			rtTextureData.ChannelCount = 4;

			m_stdTextureIDs.push_back(rtTextureID);
			m_stdTextureData.push_back(rtTextureData);
			m_iBufferSize += 16;
		}

		//some safety checks
		if ((rtProperties.ChannelCount != 4) || (rtProperties.BytesPerChannel != 2)) return false;
		if ((unsigned int)(rtProperties.ChannelCount) * (unsigned int)(rtProperties.BytesPerChannel) != 8) return false;

		//generate the texture ID and store the texture data
		TextureID rtTextureID{};
		rtTextureID.Offset = m_iBufferSize;
		rtTextureID.Width = rtProperties.Width;
		rtTextureID.Height = rtProperties.Height;
		rtTextureID.RowPitch = rtProperties.Width;

		m_stdTextureIDs.push_back(rtTextureID);
		m_stdTextureData.push_back(rtProperties);
		m_iBufferSize += ((rtTextureID.Width * rtTextureID.Height * 8) + 15) & ~15; //align the buffer to a multiple of 16

		return true;
	}


	bool TextureAtlas::Initialize(GPUScheduler* rtScheduler, DescriptorHeapInfo rtDescriptorHeapInfo)
	{
		//make a default texture for  meshes that don't use any textures if it wasn't already created
		if (m_iBufferSize == 0)
		{
			TextureID rtTextureID{};
			rtTextureID.Offset = 0;
			rtTextureID.Width = 1;
			rtTextureID.Height = 1;
			rtTextureID.RowPitch = 1;

			TextureInfo rtTextureData{};
			rtTextureData.Data = (void*)(new uint8_t[8]);
			memset(rtTextureData.Data, 0xffffffff, 8);
			rtTextureData.Width = 1;
			rtTextureData.Height = 1;
			rtTextureData.BytesPerChannel = 2;
			rtTextureData.ChannelCount = 4;

			m_stdTextureIDs.push_back(rtTextureID);
			m_stdTextureData.push_back(rtTextureData);
			m_iBufferSize += 16;
		}


		m_rtScheduler = rtScheduler;
		ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
		unsigned int iNumViews = m_rtScheduler->GetNumMaxTasks();


		//fill the resource description
		D3D12_RESOURCE_DESC1 d3dTextureDesc{};
		d3dTextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		d3dTextureDesc.Format = DXGI_FORMAT_UNKNOWN;
		d3dTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		d3dTextureDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		d3dTextureDesc.Width = m_iBufferSize;
		d3dTextureDesc.Height = 1;
		d3dTextureDesc.DepthOrArraySize = 1;
		d3dTextureDesc.MipLevels = 1;
		d3dTextureDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		d3dTextureDesc.SampleDesc.Count = 1;
		d3dTextureDesc.SampleDesc.Quality = 0;
		d3dTextureDesc.SamplerFeedbackMipRegion.Width = 0;
		d3dTextureDesc.SamplerFeedbackMipRegion.Height = 0;
		d3dTextureDesc.SamplerFeedbackMipRegion.Depth = 0;

		D3D12_HEAP_PROPERTIES d3dHeapProperties{};
		d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		d3dHeapProperties.CreationNodeMask = 0;
		d3dHeapProperties.VisibleNodeMask = 0;
		
		if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dTextureDesc,
			D3D12_RESOURCE_STATE_SHADER_RESOURCE, nullptr, nullptr, IID_PPV_ARGS(&m_d3dTextureAtlas)) < 0) return false;
		
		for (unsigned int i = 0; i < iNumViews; i++)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC d3dTextureViewDesc{};
			d3dTextureViewDesc.Format = DXGI_FORMAT_UNKNOWN;
			d3dTextureViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			d3dTextureViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			d3dTextureViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			d3dTextureViewDesc.Buffer.FirstElement = 0;
			d3dTextureViewDesc.Buffer.NumElements = (UINT)(m_iBufferSize / 16);
			d3dTextureViewDesc.Buffer.StructureByteStride = 16;

			D3D12_CPU_DESCRIPTOR_HANDLE d3dDescriptorHandle = rtDescriptorHeapInfo.d3dDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart();
			d3dDescriptorHandle.ptr += d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * rtDescriptorHeapInfo.iOffsetInDescriptor;
			d3dDevice->CreateShaderResourceView(m_d3dTextureAtlas, &d3dTextureViewDesc, d3dDescriptorHandle);
		}

		m_rtTextureIDs = new StructuredBuffer();
		if (!(m_rtTextureIDs->Initialize(m_rtScheduler, sizeof(TextureID), m_stdTextureIDs.size()))) return false;


		GPUScheduler rtUploadScheduler;
		UploadBuffer rtUploadBuffer;

		if (!(rtUploadScheduler.Initialize(m_rtScheduler->GetDX12Device()))) return false;
		ID3D12GraphicsCommandList6* d3dCommandList = rtUploadScheduler.GetCommandList();
		if (!(rtUploadBuffer.Initialize(&rtUploadScheduler, max(m_iBufferSize, sizeof(TextureID) * m_stdTextureIDs.size())))) return false;

		//upload the textures
		unsigned int iOffset = 0;
		for (unsigned int i = 0; i < m_stdTextureData.size(); i++)
		{
			TextureInfo rtTextureData = m_stdTextureData[i];

			unsigned int iNumBytes = rtTextureData.Width * rtTextureData.Height * 8;
			if (!(rtUploadBuffer.Update(rtTextureData.Data, iNumBytes, iOffset))) return false;
			iOffset += iNumBytes;
		}
		if (!(rtUploadScheduler.Record())) return false;
		if (!(rtUploadBuffer.Upload(m_d3dTextureAtlas, D3D12_RESOURCE_STATE_SHADER_RESOURCE, m_iBufferSize, 0, 0))) return false;
		if (!(rtUploadScheduler.Execute())) return false;
		rtUploadScheduler.Flush();

		//upload the texture IDs
		iOffset = 0;
		for (unsigned int i = 0; i < m_stdTextureIDs.size(); i++)
		{
			TextureID rtTextureID = m_stdTextureIDs[i];

			if (!(rtUploadBuffer.Update(&rtTextureID, sizeof(TextureID), iOffset))) return false;
			iOffset += sizeof(TextureID);
		}
		if (!(rtUploadScheduler.Record())) return false;
		if (!(m_rtTextureIDs->UploadAll(&rtUploadBuffer, sizeof(TextureID) * m_stdTextureIDs.size()))) return false;
		if (!(rtUploadScheduler.Execute())) return false;
		rtUploadScheduler.Flush();

		for (unsigned int i = 0; i < m_stdTextureData.size(); i++)
		{
			delete[] m_stdTextureData[i].Data;
		}
		m_stdTextureIDs.clear();
		m_stdTextureData.clear();

		return true;
	}


	void TextureAtlas::Bind(UINT iTextureIDsRootParameterIndex, bool bBindToCS)
	{
		m_rtTextureIDs->Bind(iTextureIDsRootParameterIndex, bBindToCS);
	}

}