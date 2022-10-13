#pragma once

#include <string>
#include <unordered_map>
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