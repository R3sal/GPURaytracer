
#include "Raytracer.hlsli"


#define GROUPSIZE_X 256
#define GROUPSIZE_Y 1
#define GROUPSIZE_Z 1


//shader resources and UAVs
ConstantBuffer<AABB> SceneAABB : register(b0, space0);
StructuredBuffer<Index> Indices : register(t0, space0);
StructuredBuffer<Vertex> Vertices : register(t1, space0);
RWStructuredBuffer<uint4> MortonCodes : register(u5, space0);



[numthreads(GROUPSIZE_X, GROUPSIZE_Y, GROUPSIZE_Z)]
void main(CSInput Input)
{
	Index Index1 = 0;
	Index Index2 = 0;
	Index Index3 = 0;
	
	if (Input.GlobalThreadID.x < SceneAABB.Padding.x)
	{
		//get the vertices
		uint CurrentIndex = MortonCodes[Input.GlobalThreadID.x].z;
		Index1 = Indices[CurrentIndex];
		Index2 = Indices[CurrentIndex + 1];
		Index3 = Indices[CurrentIndex + 2];
	}
	
	AllMemoryBarrierWithGroupSync();

	if (Input.GlobalThreadID.x < SceneAABB.Padding.x)
	{
		//get the vertices
		Indices[Input.GlobalThreadID.x] = Index1;
		Indices[Input.GlobalThreadID.x + 1] = Index2;
		Indices[Input.GlobalThreadID.x + 2] = Index3;
	}
}