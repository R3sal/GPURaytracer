
#include "Raytracer.hlsli"
#include "Random.hlsli"


#define GROUPSIZE_X 16
#define GROUPSIZE_Y 16
#define GROUPSIZE_Z 1


struct RayGenInfo
{
	int2 ScreenDimensions;
	float MinRayLength;
	float MaxRayLength;
	uint MaxRayPerPixel;
	uint3 RNGSeed;
};


//shader resources and UAVs
ConstantBuffer<RayGenInfo> InfoBuffer : register(b0, space0);
RWStructuredBuffer<Ray> GeneratedRays : register(u0, space0);
RWStructuredBuffer<Ray> OldRays : register(u1, space0); // here, TMin represents the t value in R = t * Direction + Origin, TMax is the Index of the ray in the pixel
RWStructuredBuffer<uint4> RayPixels : register(u2, space0);



void SetRayPixel(uint RayIndex, uint2 Position)
{
	uint BufferIndex = RayIndex >> 2; // = RayIndex / 4
	uint PositionValue = (Position.x << 16) | (Position.y & 0xffff);
	
	[branch]
	switch (RayIndex % 4)
	{
		case 0:
			RayPixels[BufferIndex].x = PositionValue;
			break;
		case 1:
			RayPixels[BufferIndex].y = PositionValue;
			break;
		case 2:
			RayPixels[BufferIndex].z = PositionValue;
			break;
		case 3:
			RayPixels[BufferIndex].w = PositionValue;
			break;
	}
}



[numthreads(GROUPSIZE_X, GROUPSIZE_Y, GROUPSIZE_Z)]
void main(CSInput Input)
{
	if (all(Input.GlobalThreadID.xy < InfoBuffer.ScreenDimensions))
	{
		Ray RayInfo;
		Ray OldRayInfo;
		
		for (uint i = 0; i < InfoBuffer.MaxRayPerPixel; i++)
		{
			uint3 RNGSeed = InfoBuffer.RNGSeed;
			
			uint FlattenedIndex = mad(Input.GlobalThreadID.y, InfoBuffer.ScreenDimensions.x, Input.GlobalThreadID.x) * InfoBuffer.MaxRayPerPixel + i;
			OldRayInfo = GeneratedRays[FlattenedIndex];
			
			RayInfo.Direction = float3(0.0f, 1.0f, 0.0f); //RotatedRandomDirection(RNGSeed, Normal);
			RayInfo.Origin = OldRayInfo.Origin + OldRayInfo.Direction;
			RayInfo.TMin = InfoBuffer.MinRayLength;
			RayInfo.TMax = InfoBuffer.MaxRayLength;
		
			OldRayInfo.Direction = normalize(OldRayInfo.Direction);
			OldRayInfo.TMax = asfloat(i);
		
			OldRays[FlattenedIndex] = OldRayInfo;
			GeneratedRays[FlattenedIndex] = RayInfo;
			SetRayPixel(FlattenedIndex, Input.GlobalThreadID.xy);
		}
	}
}