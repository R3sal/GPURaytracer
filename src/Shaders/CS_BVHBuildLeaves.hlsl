
#include "Raytracer.hlsli"


#define GROUPSIZE_X 256
#define GROUPSIZE_Y 1
#define GROUPSIZE_Z 1


struct BVHInfo
{
	uint NumChildren;
	uint PreviousIndex;
	uint CurrentIndex;
	uint Padding;
};


//shader resources and UAVs
ConstantBuffer<BVHInfo> InfoBuffer : register(b0, space0);
StructuredBuffer<Index> Indices : register(t0, space0);
StructuredBuffer<Vertex> Vertices : register(t1, space0);
RWStructuredBuffer<uint4> MortonCodes : register(u5, space0);
RWStructuredBuffer<AABB> BoundingVolumeHierarchy : register(u6, space0);



[numthreads(GROUPSIZE_X, GROUPSIZE_Y, GROUPSIZE_Z)]
void main(CSInput Input)
{
	if (Input.GlobalThreadID.x < ((InfoBuffer.NumChildren + 1) / 2))
	{
		uint CurrentIndices[2];
		CurrentIndices[0] = MortonCodes[2 * Input.GlobalThreadID.x].x;
		CurrentIndices[1] = 0xffffffff;
		
		uint Iterations = 3;
		if ((2 * Input.GlobalThreadID.x + 1) < InfoBuffer.NumChildren)
		{
			Iterations = 6;
			CurrentIndices[1] = MortonCodes[2 * Input.GlobalThreadID.x + 1].x;
		}
		
		//get the minimum and maximum positions
		float3 Minimum = float3(1e30f, 1e30f, 1e30f);
		float3 Maximum = float3(-1e30f, -1e30f, -1e30f);
		
		[unroll]
		for (uint i = 0; i < Iterations; i++)
		{
			Vertex CurrentVertex = Vertices[Indices[CurrentIndices[i / 3] + (i % 3)]];
			Minimum = min(Minimum, CurrentVertex.Position);
			Maximum = max(Maximum, CurrentVertex.Position);
		}
		
		AABB FinalAABB;
		FinalAABB.Min = Minimum;
		FinalAABB.Max = Maximum;
		FinalAABB.Padding.x = CurrentIndices[0] | 0x80000000;
		FinalAABB.Padding.y = CurrentIndices[1] | 0x80000000;
		
		BoundingVolumeHierarchy[Input.GlobalThreadID.x + 1] = FinalAABB;
	}
}