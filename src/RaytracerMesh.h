#pragma once

#include <string>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <DirectXMath.h>
#include "ShaderResources.h"



namespace RT::GraphicsAPI
{

	struct AABB
	{
		DirectX::XMFLOAT3 Min;
		DirectX::XMFLOAT3 Max;
		DirectX::XMUINT2 Padding;
	};

	//define indices and vertices
	typedef uint32_t Index;

	struct Vertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT2 UV;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT3 Tangent;
		uint32_t MaterialID; // = 12 * 4 bytes = 36 bytes = 384 bits
	};

	struct PBRMaterial
	{
		DirectX::XMFLOAT3 Albedo;
		float Roughness;
		DirectX::XMFLOAT3 F0Color;
		float Metallic;
		DirectX::XMFLOAT3 Emissive;

		uint32_t AlbedoTextureID;
		uint32_t RoughnessTextureID;
		uint32_t F0TextureID;
		uint32_t MetallicTextureID;
		uint32_t EmissiveTextureID;
	};

	struct TextureInfo
	{
		void* Data;
		uint32_t Width;
		uint32_t Height;
		uint16_t BytesPerChannel;
		uint16_t ChannelCount;
	};

	struct MeshInfo
	{
		uint64_t IndexCount;
		Index* Indices;
		uint64_t VertexCount;
		Vertex* Vertices;
		uint64_t MaterialCount;
		PBRMaterial* Materials;
		uint64_t TextureNameCount;
		std::string* TextureNames;
		AABB SceneAABB;
	};


	TextureInfo LoadTextureFromFile(const std::string& sFileName, int iDesiredNumChannels = 4, bool bHighPrecision = true);
	MeshInfo LoadMeshFromFile(const std::string& sFileName);



	class RaytracerMesh : public BaseShaderResource
	{
	private:

		//the data of the buffer
		D3D12_RESOURCE_DESC1 m_d3dResource2Desc;
		MeshInfo m_rtMesh;
		

	public:

		RaytracerMesh();
		~RaytracerMesh();

		bool Initialize(GPUScheduler* rtScheduler, MeshInfo rtMesh);
		void Bind(UINT iIndexRootParameterIndex, UINT iVertexRootParameterIndex, bool bBindToCS);
		void Release();
		
		//helper functions
		uint64_t GetIndexCount() { return m_rtMesh.IndexCount; };
		uint64_t GetVertexCount() { return m_rtMesh.VertexCount; };

	};
}

namespace std
{
	template<>
	struct hash<RT::GraphicsAPI::Vertex>
	{
		size_t operator()(const RT::GraphicsAPI::Vertex& key)
		{
			uint64_t iResult = 0;
			iResult |= ((uint64_t)(key.Position.x * 32.0f) & 31);
			iResult |= ((uint64_t)(key.Position.y * 32.0f) & 31) << 5;
			iResult |= ((uint64_t)(key.Position.z * 32.0f) & 31) << 10;
			iResult |= ((uint64_t)(key.Position.x * 0.000001f) & 128) << 15;
			iResult |= ((uint64_t)(key.Position.y * 0.000001f) & 128) << 23;
			iResult |= ((uint64_t)(key.Position.z * 0.000001f) & 128) << 31;
			iResult |= ((uint64_t)(key.Normal.x * 512.0f) & 511) << 39;
			iResult |= ((uint64_t)(key.Normal.y * 512.0f) & 511) << 48;
			iResult |= ((uint64_t)(key.MaterialID / 2) & 255) << 57;
			hash<string>()("f");
			return iResult;
		}
	};
}