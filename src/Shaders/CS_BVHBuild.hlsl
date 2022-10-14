
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
RWStructuredBuffer<AABB> BoundingVolumeHierarchy : register(u6, space0);



[numthreads(GROUPSIZE_X, GROUPSIZE_Y, GROUPSIZE_Z)]
void main(CSInput Input)
{
	if (Input.GlobalThreadID.x < ((InfoBuffer.NumChildren + 1) / 2))
	{
		uint Index1 = 2 * Input.GlobalThreadID.x + InfoBuffer.PreviousIndex;
		uint Index2 = Index1 + 1;
		AABB AABB1 = BoundingVolumeHierarchy[Index1];
		AABB AABB2 = AABB1;
		if (Index2 < InfoBuffer.NumChildren)
		{
			AABB2 = BoundingVolumeHierarchy[Index2];
		}
		else
		{
			Index2 = 0xffffffff;
		}
		AABB1.Min = min(AABB1.Min, AABB2.Min);
		AABB1.Max = max(AABB1.Max, AABB2.Max);
		AABB1.Padding.x = Index1;
		AABB1.Padding.y = Index2;
		//BoundingVolumeHierarchy[InfoBuffer.CurrentIndex + Input.GlobalThreadID.x] = AABB1;
	}
}