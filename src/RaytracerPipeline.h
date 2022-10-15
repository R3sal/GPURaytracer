#pragma once

#include <random>

#include <DirectXMath.h>

//include the dx12 device class
#include "Settings.h"
#include "GPUDevice.h"
#include "GPUScheduler.h"
#include "RenderTarget.h"
#include "PipelineState.h"
#include "ShaderResources.h"
#include "RaytracerMesh.h"
#include "TextureAtlas.h"
#include "TextureToScreenPass.h"



namespace RT::GraphicsAPI
{
	//global constants
	//customizeable parameters
	const unsigned int MAX_RAYS_PER_PIXEL = RT_MAX_RAYS_PER_PIXEL; //number of rays per pixel, the higher this value, the better AA and DOF effects will be
	const unsigned int MAX_RAY_DEPTH = RT_MAX_RAY_DEPTH; //after shooting this number of rays, we return to shooting a ray from the camera
	const float AA_SAMPLE_SPREAD = RT_AA_SAMPLE_SPREAD; //anti-aliasing: the bigger the value, the blurrier the image, disabled at 0.0f, default is 1.0f
	const float DOF_SAMPLE_SPREAD = RT_DOF_SAMPLE_SPREAD; //depth of field: the bigger the value, the stronger the DOF effect, disabled at 0.0f, default is 1.0f

	//strictly defined parameters
	const unsigned int MAX_RAYS = RT_WINDOW_WIDTH * RT_WINDOW_HEIGHT * MAX_RAYS_PER_PIXEL;
	const unsigned int SIZEOF_RAY = 8 * 4;
	const unsigned int SIZEOF_RAYPIXEL = 4 * 4;


	//the camera ray generation modules
	struct CameraRayGenInfo
	{
		DirectX::XMFLOAT4X4 InverseView;
		DirectX::XMFLOAT4X4 InverseProjection;
		DirectX::XMINT2 ScreenSize;
		float AASampleSpread;
		float DOFSampleSpread;
		uint32_t MaxRaysPerPixel;
		DirectX::XMUINT3 RNGSeed;
	};

	struct CameraInfo
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 FocusPoint;
		DirectX::XMFLOAT3 UpDirection;
		float VerticalFOV;
		float NearZ;
		float FarZ;
	};

	class CameraRayGen
	{
	private:

		//private member variables
		GPUScheduler* m_rtFrameScheduler;
		PipelineState* m_rtCameraRayGenState;
		DescriptorHeap* m_rtUAVDescriptorHeap;
		CameraRayGenInfo m_rtInfoData;
		ConstantBuffer* m_rtCameraRayGenInfoBuffer;
		RWStructuredBuffer* m_rtRayBuffer;
		RWStructuredBuffer* m_rtOldRayBuffer;
		RWStructuredBuffer* m_rtRayPixelsBuffer;
		std::mt19937 m_stdPRNG;


		//private functions


	public: // = usable outside of the class

		//constructor and destructor
		CameraRayGen();
		~CameraRayGen();


		//public class functions
		bool Initialize(GPUScheduler* rtScheduler, DescriptorHeap* rtUAVDescriptorTable, CameraInfo rtCameraData);
		bool Render();


		//helper functions

	};


	//the sorting modules
	struct MortonCodeInfo
	{
		DirectX::XMFLOAT4 SceneMin;
		DirectX::XMFLOAT4 SceneMax;
		uint32_t NumPrimitives;
		DirectX::XMUINT3 Padding;
	};
	struct SortInfo
	{
		uint32_t NumElements;
		uint32_t SortPassIndex;
	};

	class SortPrimitives
	{
	private:

		//private member variables
		GPUScheduler* m_rtFrameScheduler;
		PipelineState* m_rtGenMortonCodeState;
		PipelineState* m_rtSortCountState;
		PipelineState* m_rtSortPrefixSumState;
		PipelineState* m_rtSortSortState;
		MortonCodeInfo m_rtMortonCodeInfoData;
		SortInfo m_rtSortInfoData;
		ConstantBuffer* m_rtMortonCodeInfoBuffer;
		ConstantBuffer* m_rtSortInfoBuffer[4];
		RWStructuredBuffer* m_rtMortonCodeBuffer;
		RWStructuredBuffer* m_rtTempMortonCodeBuffer;
		RWStructuredBuffer* m_rtCodeFrequenciesBuffer;


		//private functions


	public: // = usable outside of the class

		//constructor and destructor
		SortPrimitives();
		~SortPrimitives();


		//public class functions
		bool Initialize(GPUScheduler* rtScheduler, uint32_t iNumPrimitives, AABB rtSceneAABB);
		bool Sort(RaytracerMesh* rtMesh);


		//helper functions
		RWStructuredBuffer* GetMortonCodes() { return m_rtMortonCodeBuffer; };

	};


	//the BVH building
	struct BVHInfo
	{
		uint32_t NumChildren;
		uint32_t PreviousIndex;
		uint32_t CurrentIndex;
		uint32_t Padding;
	};

	class BuildBVH
	{
	private:

		//private member variables
		GPUScheduler* m_rtFrameScheduler;
		PipelineState* m_rtBuildLeavesState;
		PipelineState* m_rtBuildState;
		BVHInfo m_rtBVHInfoData;
		ConstantBuffer* m_rtBuildLeavesInfoBuffer;
		ConstantBuffer* m_rtBVHBuildInfoBuffer[32];
		RWStructuredBuffer* m_rtBVHBuffer;


		//private functions


	public: // = usable outside of the class

		//constructor and destructor
		BuildBVH();
		~BuildBVH();


		//public class functions
		bool Initialize(GPUScheduler* rtScheduler, uint32_t iNumPrimitives);
		bool Build(RaytracerMesh* rtMesh, RWStructuredBuffer* rtMortonCodes);


		//helper functions
		RWStructuredBuffer* GetBVH() { return m_rtBVHBuffer; };

	};


	//the ray tracing modules
	struct TraceRaysInfo
	{
		DirectX::XMINT2 ScreenDimensions;
		uint32_t NumIndices;
		uint32_t NumRays;
		uint32_t MaxRaysPerPixel;
		DirectX::XMUINT3 RNGSeed;
	};

	class TraceRays
	{
	private:

		//private member variables
		GPUScheduler* m_rtFrameScheduler;
		PipelineState* m_rtTraceRaysState;
		RaytracerMesh* m_rtMesh;
		TextureAtlas* m_rtTextures;
		DescriptorHeap* m_rtUAVDescriptorHeap;
		TraceRaysInfo m_rtInfoData;
		ConstantBuffer* m_rtTraceRaysInfoBuffer;
		StructuredBuffer* m_rtMaterialBuffer;
		RWStructuredBuffer* m_rtScatteredLightBuffer;
		RWStructuredBuffer* m_rtEmittedLightBuffer;
		std::mt19937 m_stdPRNG;


		//private functions


	public: // = usable outside of the class

		//constructor and destructor
		TraceRays();
		~TraceRays();


		//public class functions
		bool Initialize(GPUScheduler* rtScheduler, DescriptorHeap* rtUAVDescriptorTable, MeshInfo rtMeshData);
		bool Render(RWStructuredBuffer* rtBVH);


		//helper functions
		RaytracerMesh* GetMesh() { return m_rtMesh; };

	};


	//the final image generation
	struct GenerateFinalImageInfo
	{
		DirectX::XMINT2 ScreenDimensions;
		uint32_t MaxRaysPerPixel;
		uint32_t NumSamples;
	};

	class GenerateFinalImage
	{
	private:

		//private member variables
		GPUScheduler* m_rtFrameScheduler;
		PipelineState* m_rtGenerateImageState;
		DescriptorHeap* m_rtUAVDescriptorHeap;
		GenerateFinalImageInfo m_rtInfoData;
		ConstantBuffer* m_rtGenerateImageInfoBuffer;
		RWStructuredBuffer* m_rtResultBuffer;
		RWTexture2D* m_rtOutputTexture;;


		//private functions


	public: // = usable outside of the class

		//constructor and destructor
		GenerateFinalImage();
		~GenerateFinalImage();


		//public class functions
		bool Initialize(GPUScheduler* rtScheduler, DescriptorHeap* rtUAVDescriptorTable,
			DXGI_FORMAT dxTargetFormat = DXGI_FORMAT_R16G16B16A16_FLOAT);
		bool Render(bool bApplyResults = false);


		//helper functions

	};



	//the raytracer pipeline, which combines all the classes from above
	class RaytracerPipeline
	{
	private:

		//private member variables
		DX12Device*		m_rtDevice;
		GPUScheduler*	m_rtFrameScheduler;
		CameraRayGen*			m_rtCameraRayGen;
		SortPrimitives*			m_rtSortPrimitives;
		BuildBVH*				m_rtBuildBVH;
		TraceRays*				m_rtTraceRays;
		GenerateFinalImage*		m_rtImageGeneration;
		TextureToScreenPass*	m_rtFinalPass;
		DescriptorHeap*	m_rtUAVDescriptorHeap;


		//private functions


	public: // = usable outside of the class

		//constructor and destructor
		RaytracerPipeline();
		~RaytracerPipeline();


		//public class functions
		bool Initialize(DX12Device* rtDevice, MeshInfo rtMeshData);
		bool Render();


		//helper functions

	};
}