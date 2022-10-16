//include-files
#include "RaytracerMesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>



namespace RT::GraphicsAPI
{
	//load a texture
	TextureInfo LoadTextureFromFile(const std::string& sFileName, int iDesiredNumChannels, bool bHighPrecision)
	{
		int iWidth = 0;
		int iHeight = 0;
		int iNumComponents = 0;
		int iBytesPerChannel = 0;
		unsigned char* pData = nullptr;

		if (bHighPrecision)
		{
			iBytesPerChannel = 2;
			pData = (unsigned char*)stbi_load_16(sFileName.c_str(), &iWidth, &iHeight, &iNumComponents, iDesiredNumChannels);
		}
		else
		{
			iBytesPerChannel = 1;
			pData = stbi_load(sFileName.c_str(), &iWidth, &iHeight, &iNumComponents, iDesiredNumChannels);
		}
		if (iDesiredNumChannels > 0) iNumComponents = iDesiredNumChannels;
		if ((!pData) || (iNumComponents < 1) || (iBytesPerChannel < 1) || (iWidth < 1) || (iHeight < 1))
		{
			std::cout << "Error loading textures: " << stbi_failure_reason() << "\n";
			return (TextureInfo)0;
		}

		unsigned int iMemorySize = (unsigned int)iWidth * (unsigned int)iHeight * (unsigned int)iNumComponents * (unsigned int)iBytesPerChannel;
		TextureInfo rtTextureData{};
		rtTextureData.Data = new byte[iMemorySize];
		memcpy(rtTextureData.Data, pData, iMemorySize);
		rtTextureData.Width = iWidth;
		rtTextureData.Height = iHeight;
		rtTextureData.BytesPerChannel = (unsigned short)iBytesPerChannel;
		rtTextureData.ChannelCount = (unsigned short)iNumComponents;

		stbi_image_free(pData);

		std::cout << "Successfully loaded a texture with following parameters:\n Width:           " << rtTextureData.Width << "\n Height:          "
			<< rtTextureData.Height << "\n Bytes per pixel: " << (rtTextureData.BytesPerChannel * rtTextureData.ChannelCount) << "\n";

		return rtTextureData;
	}


	//helper functions for mesh loading
	void CalculateNormals(MeshInfo* ionMeshInfo)
	{
		//store the data of the mesh in local variables
		uint64_t& iIndexCount = ionMeshInfo->IndexCount;
		Vertex* ionVertexData = ionMeshInfo->Vertices;
		Index* iIndexData = ionMeshInfo->Indices;

		const uint64_t iTriangleCount = iIndexCount / 3;
		DirectX::XMVECTOR* xmAveragedNormals = new DirectX::XMVECTOR[iIndexCount];
		memset(xmAveragedNormals, 0, iIndexCount * sizeof(DirectX::XMVECTOR));

		//calculate the normals
		for (unsigned int i = 0; i < iTriangleCount; i++)
		{
			unsigned int iIndex1 = iIndexData[i * 3];
			unsigned int iIndex2 = iIndexData[i * 3 + 1];
			unsigned int iIndex3 = iIndexData[i * 3 + 2];

			DirectX::XMVECTOR xmPosition1 = DirectX::XMLoadFloat3(&(ionVertexData[iIndex1].Position));
			DirectX::XMVECTOR xmPosition2 = DirectX::XMLoadFloat3(&(ionVertexData[iIndex2].Position));
			DirectX::XMVECTOR xmPosition3 = DirectX::XMLoadFloat3(&(ionVertexData[iIndex3].Position));
			DirectX::XMVECTOR xmEdge1 = DirectX::XMVectorSubtract(xmPosition3, xmPosition1);
			DirectX::XMVECTOR xmEdge2 = DirectX::XMVectorSubtract(xmPosition2, xmPosition1);

			DirectX::XMVECTOR xmNewNormal = DirectX::XMVector3Cross(xmEdge2, xmEdge1);
			xmAveragedNormals[iIndex1] = DirectX::XMVectorAdd(xmAveragedNormals[iIndex1], xmNewNormal);
			xmAveragedNormals[iIndex2] = DirectX::XMVectorAdd(xmAveragedNormals[iIndex2], xmNewNormal);
			xmAveragedNormals[iIndex3] = DirectX::XMVectorAdd(xmAveragedNormals[iIndex3], xmNewNormal);
		}

		//average the normals
		for (unsigned int i = 0; i < iIndexCount; i++)
		{
			unsigned int iCurrentIndex = iIndexData[i];
			DirectX::XMStoreFloat3(&(ionVertexData[iCurrentIndex].Normal), DirectX::XMVector3Normalize(xmAveragedNormals[iCurrentIndex]));
		}

		delete[] xmAveragedNormals;
	}

	void CalculateTangents(MeshInfo* ionMeshInfo)
	{
		//store the data of the mesh in local variables
		uint64_t& iIndexCount = ionMeshInfo->IndexCount;
		Index* iIndexData = ionMeshInfo->Indices;
		Vertex* ionVertexData = ionMeshInfo->Vertices;

		const uint64_t iTriangleCount = iIndexCount / 3;
		DirectX::XMVECTOR* xmAveragedTangents = new DirectX::XMVECTOR[iIndexCount];
		memset(xmAveragedTangents, 0, sizeof(DirectX::XMVECTOR) * iIndexCount);

		//calculate the tangents
		for (unsigned int i = 0; i < iTriangleCount; i++)
		{
			//load some required data from the given mesh
			unsigned int iIndex1 = iIndexData[i * 3];
			unsigned int iIndex2 = iIndexData[i * 3 + 1];
			unsigned int iIndex3 = iIndexData[i * 3 + 2];

			DirectX::XMVECTOR xmPosition1 = DirectX::XMLoadFloat3(&(ionVertexData[iIndex1].Position));
			DirectX::XMVECTOR xmPosition2 = DirectX::XMLoadFloat3(&(ionVertexData[iIndex2].Position));
			DirectX::XMVECTOR xmPosition3 = DirectX::XMLoadFloat3(&(ionVertexData[iIndex3].Position));
			DirectX::XMVECTOR xmEdge1 = DirectX::XMVectorSubtract(xmPosition2, xmPosition1);
			DirectX::XMVECTOR xmEdge2 = DirectX::XMVectorSubtract(xmPosition3, xmPosition1);

			DirectX::XMVECTOR xmTextureUV1 = DirectX::XMLoadFloat2(&(ionVertexData[iIndex1].UV));
			DirectX::XMVECTOR xmTextureUV2 = DirectX::XMLoadFloat2(&(ionVertexData[iIndex2].UV));
			DirectX::XMVECTOR xmTextureUV3 = DirectX::XMLoadFloat2(&(ionVertexData[iIndex3].UV));
			DirectX::XMVECTOR xmTextureEdge1 = DirectX::XMVectorSubtract(xmTextureUV2, xmTextureUV1);
			DirectX::XMVECTOR xmTextureEdge2 = DirectX::XMVectorSubtract(xmTextureUV3, xmTextureUV1);

			//compute a nominator and denominator for the final calculations
			DirectX::XMVECTOR xmDenominator = DirectX::XMVector2Cross(xmTextureEdge1, xmTextureEdge2);
			DirectX::XMVECTOR xmTextureEdge1y = DirectX::XMVectorSplatY(xmTextureEdge1);
			DirectX::XMVECTOR xmTextureEdge2y = DirectX::XMVectorSplatY(xmTextureEdge2);
			DirectX::XMVECTOR xmNominatorA = DirectX::XMVectorMultiply(xmTextureEdge2y, xmEdge1);
			DirectX::XMVECTOR xmNominatorB = DirectX::XMVectorMultiply(xmTextureEdge1y, xmEdge2);
			DirectX::XMVECTOR xmNominator = DirectX::XMVectorSubtract(xmNominatorA, xmNominatorB);

			//check if the denominator isn't almost equal to 0 and compute the tangent
			DirectX::XMVECTOR xmDivideByZero = DirectX::XMVectorInBounds(xmDenominator, DirectX::g_XMEpsilon);
			DirectX::XMVECTOR xmNewTangent = DirectX::XMVectorSelect(
				DirectX::XMVectorDivide(xmNominator, xmDenominator), DirectX::XMVectorZero(), xmDivideByZero);
			xmAveragedTangents[iIndex1] = DirectX::XMVectorAdd(xmAveragedTangents[iIndex1], xmNewTangent);
			xmAveragedTangents[iIndex2] = DirectX::XMVectorAdd(xmAveragedTangents[iIndex2], xmNewTangent);
			xmAveragedTangents[iIndex3] = DirectX::XMVectorAdd(xmAveragedTangents[iIndex3], xmNewTangent);
		}

		//average the tangents
		for (unsigned int i = 0; i < iIndexCount; i++)
		{
			unsigned int iCurrentIndex = iIndexData[i];
			DirectX::XMStoreFloat3(&(ionVertexData[iCurrentIndex].Tangent), DirectX::XMVector3Normalize(xmAveragedTangents[iCurrentIndex]));
		}

		delete[] xmAveragedTangents;
	}

	bool VerticesAreEqual(const Vertex& rtVertex1, const Vertex& rtVertex2)
	{
		if ((rtVertex1.Position.x != rtVertex2.Position.x) ||
			(rtVertex1.Position.y != rtVertex2.Position.y) ||
			(rtVertex1.Position.z != rtVertex2.Position.z)) return false;
		if ((rtVertex1.UV.x != rtVertex2.UV.x) ||
			(rtVertex1.UV.y != rtVertex2.UV.y)) return false;
		if ((rtVertex1.Normal.x != rtVertex2.Normal.x) ||
			(rtVertex1.Normal.y != rtVertex2.Normal.y) ||
			(rtVertex1.Normal.z != rtVertex2.Normal.z)) return false;
		if (rtVertex1.MaterialID != rtVertex2.MaterialID) return false;
		return true;
	}

	void GetTexture(const std::string& sTextureName, std::unordered_map<std::string, uint32_t>& stdTextureNames)
	{
		if (!(stdTextureNames.contains(sTextureName)))
		{
			uint64_t iTextureIndex = stdTextureNames.size();
			stdTextureNames[sTextureName] = iTextureIndex;
		}
	}

	//the actual mesh loading function
	MeshInfo LoadMeshFromFile(const std::string& sFileName)
	{
		tinyobj::ObjReaderConfig tolReaderConfig{};
		tolReaderConfig.triangulate = false;
		tolReaderConfig.triangulation_method = "simple";
		tolReaderConfig.vertex_color = false;
		tolReaderConfig.mtl_search_path = "";

		tinyobj::ObjReader tolReader{};
		if (!(tolReader.ParseFromFile(sFileName, tolReaderConfig)))
		{
			std::cout << "Error loading scene: " << tolReader.Error() << "\n";
			return (MeshInfo)0;
		}

		auto& tolAttributes = tolReader.GetAttrib();
		auto& tolShapes = tolReader.GetShapes();
		auto& tolMaterials = tolReader.GetMaterials();

		//get the total number of vertices 
		uint64_t iNumVertices = 0;
		for (auto& CurrentShape : tolShapes)
		{
			iNumVertices += 3 * (uint64_t)(CurrentShape.mesh.num_face_vertices.size());
		}

		//generate the vertices
		Vertex* rtVertices = new Vertex[iNumVertices];
		memset(rtVertices, 0, sizeof(Vertex) * iNumVertices);
		bool bCalculateNormals = false;
		uint64_t iIndexOffset = 0;
		for (unsigned int s = 0; s < tolShapes.size(); s++) //the shapes / meshes
		{
			auto& tolCurrentMesh = tolShapes[s];
			for (unsigned int f = 0; f < tolCurrentMesh.mesh.num_face_vertices.size(); f++) //the individual faces (triangles)
			{
				auto& tolCurrentFace = tolCurrentMesh.mesh.num_face_vertices[f];
				for (unsigned int v = 0; v < 3; v++) //the three points, that form a triangle (we expect only triangles)
				{
					auto& tolCurrentIndex = tolCurrentMesh.mesh.indices[3 * f + v];

					//get the position
					rtVertices[iIndexOffset].Position.x = tolAttributes.vertices[3 * tolCurrentIndex.vertex_index];
					rtVertices[iIndexOffset].Position.y = tolAttributes.vertices[3 * tolCurrentIndex.vertex_index + 1];
					rtVertices[iIndexOffset].Position.z = tolAttributes.vertices[3 * tolCurrentIndex.vertex_index + 2];

					//get the texture uv
					rtVertices[iIndexOffset].UV.x = tolAttributes.texcoords[2 * tolCurrentIndex.texcoord_index];
					rtVertices[iIndexOffset].UV.y = tolAttributes.texcoords[2 * tolCurrentIndex.texcoord_index + 1];
					
					//get the normal
					rtVertices[iIndexOffset].Normal.x = tolAttributes.normals[3 * tolCurrentIndex.normal_index];
					rtVertices[iIndexOffset].Normal.y = tolAttributes.normals[3 * tolCurrentIndex.normal_index + 1];
					rtVertices[iIndexOffset].Normal.z = tolAttributes.normals[3 * tolCurrentIndex.normal_index + 2];
					
					if (tolCurrentIndex.normal_index < 0)
					{
						bCalculateNormals = true;
					}

					//get the material index
					rtVertices[iIndexOffset].MaterialID = tolCurrentMesh.mesh.material_ids[f];

					iIndexOffset++;
				}
			}
		}

		//remove any vertex duplicates
		Index* rtIndices = new Index[iNumVertices];
		Vertex* rtUniqueVertices = new Vertex[iNumVertices];
		uint64_t iNumUniqueVertices = 0;
		for (uint64_t i = 0; i < iNumVertices; i++)
		{
			rtIndices[i] = iNumUniqueVertices;
			bool bVertexUnique = true;

			for (uint64_t j = 0; j < iNumUniqueVertices; j++)
			{
				if (VerticesAreEqual(rtVertices[i], rtUniqueVertices[j]))
				{
					rtIndices[i] = j;
					bVertexUnique = false;
					break;
				}
			}

			if (bVertexUnique)
			{
				rtUniqueVertices[iNumUniqueVertices] = rtVertices[i];
				iNumUniqueVertices++;
			}
		}
		delete[] rtVertices;

		//get the materials
		uint64_t iNumMaterials = tolMaterials.size();
		PBRMaterial* rtMaterials = new PBRMaterial[iNumMaterials];
		std::unordered_map<std::string, uint32_t> stdTextureNames;
		stdTextureNames[""] = 0;
		for (uint64_t i = 0; i < iNumMaterials; i++)
		{
			auto& tolCurrentMaterial = tolMaterials[i];

			rtMaterials[i].Albedo.x = tolCurrentMaterial.diffuse[0];
			rtMaterials[i].Albedo.y = tolCurrentMaterial.diffuse[1];
			rtMaterials[i].Albedo.z = tolCurrentMaterial.diffuse[2];

			float fRoughness = tolCurrentMaterial.shininess;
			//converts shininess to a value between 0 and 1, which is better suited for the PBR lighting model
			fRoughness = 1.0f - ((fRoughness > 1.0f) ? (log2(min(max(tolCurrentMaterial.shininess, 1.0f), 1448.15f)) / 10.5f) : fRoughness);

			rtMaterials[i].Roughness = fRoughness;
			rtMaterials[i].F0Color.x = tolCurrentMaterial.specular[0];
			rtMaterials[i].F0Color.y = tolCurrentMaterial.specular[1];
			rtMaterials[i].F0Color.z = tolCurrentMaterial.specular[2];
			rtMaterials[i].Metallic = tolCurrentMaterial.metallic;
			rtMaterials[i].Emissive.x = tolCurrentMaterial.emission[0];
			rtMaterials[i].Emissive.y = tolCurrentMaterial.emission[1];
			rtMaterials[i].Emissive.z = tolCurrentMaterial.emission[2];
			
			GetTexture(tolCurrentMaterial.diffuse_texname, stdTextureNames);
			GetTexture(tolCurrentMaterial.roughness_texname, stdTextureNames);
			GetTexture(tolCurrentMaterial.specular_texname, stdTextureNames);
			GetTexture(tolCurrentMaterial.metallic_texname, stdTextureNames);
			GetTexture(tolCurrentMaterial.emissive_texname, stdTextureNames);

			rtMaterials[i].AlbedoTextureID = stdTextureNames[tolCurrentMaterial.diffuse_texname];
			rtMaterials[i].RoughnessTextureID = stdTextureNames[tolCurrentMaterial.roughness_texname];
			rtMaterials[i].F0TextureID = stdTextureNames[tolCurrentMaterial.specular_texname];
			rtMaterials[i].MetallicTextureID = stdTextureNames[tolCurrentMaterial.metallic_texname];
			rtMaterials[i].EmissiveTextureID = stdTextureNames[tolCurrentMaterial.emissive_texname];
		}

		//fill in the meshinfo structure
		MeshInfo rtMesh{};
		rtMesh.IndexCount = iNumVertices;
		rtMesh.Indices = rtIndices;
		rtMesh.VertexCount = iNumUniqueVertices;
		rtMesh.Vertices = new Vertex[rtMesh.VertexCount];
		rtMesh.MaterialCount = max(1, iNumMaterials);
		rtMesh.Materials = iNumMaterials > 0 ? rtMaterials : nullptr;
		rtMesh.TextureNameCount = stdTextureNames.size();
		rtMesh.TextureNames = new std::string[rtMesh.TextureNameCount];
		rtMesh.SceneAABB = AABB();
		for (auto& [sTextureName, iTextureIndex] : stdTextureNames)
		{
			rtMesh.TextureNames[iTextureIndex] = sTextureName;
		}

		memcpy(rtMesh.Vertices, rtUniqueVertices, sizeof(Vertex)* rtMesh.VertexCount);
		delete[] rtUniqueVertices;

		DirectX::XMVECTOR xmOneThird = DirectX::XMVectorSet(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f);
		DirectX::XMVECTOR xmSceneMin = DirectX::XMVectorSet(D3D12_FLOAT32_MAX, D3D12_FLOAT32_MAX, D3D12_FLOAT32_MAX, D3D12_FLOAT32_MAX);
		DirectX::XMVECTOR xmSceneMax = DirectX::XMVectorSet(-D3D12_FLOAT32_MAX, -D3D12_FLOAT32_MAX, -D3D12_FLOAT32_MAX, -D3D12_FLOAT32_MAX);
		for (unsigned int i = 0; i < rtMesh.IndexCount; i += 3)
		{
			//get the vertex positions
			Vertex rtVertex1 = rtMesh.Vertices[rtMesh.Indices[i]];
			Vertex rtVertex2 = rtMesh.Vertices[rtMesh.Indices[i + 1]];
			Vertex rtVertex3 = rtMesh.Vertices[rtMesh.Indices[i + 2]];
			DirectX::XMVECTOR xmPosition1 = DirectX::XMLoadFloat3(&(rtVertex1.Position));
			DirectX::XMVECTOR xmPosition2 = DirectX::XMLoadFloat3(&(rtVertex2.Position));
			DirectX::XMVECTOR xmPosition3 = DirectX::XMLoadFloat3(&(rtVertex3.Position));

			//expand the scene AABB according to the centroid value
			DirectX::XMVECTOR xmCentroid = DirectX::XMVectorAdd(xmPosition1, xmPosition2);
			xmCentroid = DirectX::XMVectorAdd(xmCentroid, xmPosition3);
			xmCentroid = DirectX::XMVectorMultiply(xmCentroid, xmOneThird);
			xmSceneMin = DirectX::XMVectorMin(xmSceneMin, xmCentroid);
			xmSceneMax = DirectX::XMVectorMax(xmSceneMax, xmCentroid);
		}
		//store the scene bounding box
		DirectX::XMStoreFloat3(&(rtMesh.SceneAABB.Min), xmSceneMin);
		DirectX::XMStoreFloat3(&(rtMesh.SceneAABB.Max), xmSceneMax);
		
		//make a default material, if there are no materials (it slightly glows so the scene isn't completely dark)
		if (iNumMaterials < 1)
		{
			rtMesh.Materials = new PBRMaterial[1];
			memset(rtMesh.Materials, 0, sizeof(PBRMaterial));
			rtMesh.Materials[0].Albedo = { 0.0f, 0.0f, 0.0f };
			rtMesh.Materials[0].Roughness = 0.0f;
			rtMesh.Materials[0].F0Color = { 0.0f, 0.0f, 0.0f };
			rtMesh.Materials[0].Metallic = 0.0f;
			rtMesh.Materials[0].Emissive = { 0.5f, 0.5f, 0.5f };
		}

		//calculate the normals and tangents
		if (bCalculateNormals)
		{
			CalculateNormals(&rtMesh);
		}
		CalculateTangents(&rtMesh);

		std::cout << "Successfully loaded the scene with:\n " << rtMesh.VertexCount << " vertices\n " << rtMesh.IndexCount << " indices\n "
			<< rtMesh.MaterialCount << " materials\n " << (rtMesh.TextureNameCount - 1) << " textures\n";

		return rtMesh;
	}



	RaytracerMesh::RaytracerMesh() :
		BaseShaderResource(),
		m_d3dResource2Desc(),
		m_rtMesh()
	{

	}

	//destructor: uninitializes all our pointers
	RaytracerMesh::~RaytracerMesh()
	{
		Release();
	}



	//private class functions



	//public class functions
	bool RaytracerMesh::Initialize(GPUScheduler* rtScheduler, MeshInfo rtMesh)
	{
		//initialize the variables
		m_rtScheduler = rtScheduler;
		if (!m_rtScheduler) return false;
		ID3D12Device8* d3dDevice = m_rtScheduler->GetDX12Device()->GetDevice();
		m_d3dResource = new ID3D12Resource2*[2];
		if (!m_d3dResource) return false;
		m_rtMesh = rtMesh;
		m_bResourceInDescriptorTable = false;

		//fill the resource description
		m_d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		m_d3dResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		m_d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		m_d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		m_d3dResourceDesc.Width = m_rtMesh.IndexCount * sizeof(Index);
		m_d3dResourceDesc.Height = 1;
		m_d3dResourceDesc.DepthOrArraySize = 1;
		m_d3dResourceDesc.MipLevels = 1;
		m_d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // a requirement for buffers
		m_d3dResourceDesc.SampleDesc.Count = 1;
		m_d3dResourceDesc.SampleDesc.Quality = 0;
		m_d3dResourceDesc.SamplerFeedbackMipRegion.Width = 0;
		m_d3dResourceDesc.SamplerFeedbackMipRegion.Height = 0;
		m_d3dResourceDesc.SamplerFeedbackMipRegion.Depth = 0;

		m_d3dResource2Desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		m_d3dResource2Desc.Format = DXGI_FORMAT_UNKNOWN;
		m_d3dResource2Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		m_d3dResource2Desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		m_d3dResource2Desc.Width = m_rtMesh.VertexCount * sizeof(Vertex);
		m_d3dResource2Desc.Height = 1;
		m_d3dResource2Desc.DepthOrArraySize = 1;
		m_d3dResource2Desc.MipLevels = 1;
		m_d3dResource2Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // a requirement for buffers
		m_d3dResource2Desc.SampleDesc.Count = 1;
		m_d3dResource2Desc.SampleDesc.Quality = 0;
		m_d3dResource2Desc.SamplerFeedbackMipRegion.Width = 0;
		m_d3dResource2Desc.SamplerFeedbackMipRegion.Height = 0;
		m_d3dResource2Desc.SamplerFeedbackMipRegion.Depth = 0;

		//create the resource
		D3D12_HEAP_PROPERTIES d3dHeapProperties{};
		d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		d3dHeapProperties.CreationNodeMask = 0;
		d3dHeapProperties.VisibleNodeMask = 0;

		if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResourceDesc,
			D3D12_RESOURCE_STATE_SHADER_RESOURCE, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource)) < 0) return false;
		if (d3dDevice->CreateCommittedResource2(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &m_d3dResource2Desc,
				D3D12_RESOURCE_STATE_SHADER_RESOURCE, nullptr, nullptr, IID_PPV_ARGS(m_d3dResource + 1)) < 0) return false;

		//upload the data
		ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();
		GPUScheduler rtUploadScheduler;
		if (!(rtUploadScheduler.Initialize(m_rtScheduler->GetDX12Device()))) return false;
		UploadBuffer rtUploadBuffer;
		if (!(rtUploadBuffer.Initialize(&rtUploadScheduler, rtMesh.IndexCount * sizeof(Index) + rtMesh.VertexCount * sizeof(Vertex)))) return false;
		if (!(rtUploadBuffer.Update(rtMesh.Indices, rtMesh.IndexCount * sizeof(Index), 0))) return false;
		if (!(rtUploadBuffer.Update(rtMesh.Vertices, rtMesh.VertexCount * sizeof(Vertex), rtMesh.IndexCount * sizeof(Index)))) return false;

		if (!(rtUploadScheduler.Record())) return false;
		if (!(rtUploadBuffer.Upload(m_d3dResource[0], D3D12_RESOURCE_STATE_SHADER_RESOURCE,
			rtMesh.IndexCount * sizeof(Index), 0, 0))) return false;
		if (!(rtUploadBuffer.Upload(m_d3dResource[1], D3D12_RESOURCE_STATE_SHADER_RESOURCE,
			rtMesh.VertexCount * sizeof(Vertex), rtMesh.IndexCount * sizeof(Index), 0))) return false;
		if (!(rtUploadScheduler.Execute())) return false;
		rtUploadScheduler.Flush();

		//delete the mesh data on the cpu (because it is now on the gpu)
		delete[] rtMesh.Indices;
		delete[] rtMesh.Vertices;
		delete[] rtMesh.Materials;

		return true;
	}


	void RaytracerMesh::Bind(UINT iIndexRootParameterIndex, UINT iVertexRootParameterIndex, bool bBindToCS)
	{
		ID3D12GraphicsCommandList6* d3dCommandList = m_rtScheduler->GetCommandList();

		if (bBindToCS)
		{
			d3dCommandList->SetComputeRootShaderResourceView(iIndexRootParameterIndex, m_d3dResource[0]->GetGPUVirtualAddress());
			d3dCommandList->SetComputeRootShaderResourceView(iVertexRootParameterIndex, m_d3dResource[1]->GetGPUVirtualAddress());
		}
		else
		{
			d3dCommandList->SetGraphicsRootShaderResourceView(iIndexRootParameterIndex, m_d3dResource[0]->GetGPUVirtualAddress());
			d3dCommandList->SetGraphicsRootShaderResourceView(iVertexRootParameterIndex, m_d3dResource[1]->GetGPUVirtualAddress());
		}
	}


	void RaytracerMesh::Release()
	{
		if (m_d3dResource)
		{
			for (unsigned int i = 0; i < 2; i++) //one index and one vertex buffer
			{
				if (m_d3dResource[i]) m_d3dResource[i]->Release();
			}
			delete m_d3dResource;
			m_d3dResource = nullptr;
		}
	}

}