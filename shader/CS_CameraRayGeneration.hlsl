
#include "Raytracer.hlsli"
#include "Random.hlsli"


#define GROUPSIZE_X 16
#define GROUPSIZE_Y 16
#define GROUPSIZE_Z 1


struct RayGenInfo
{
	float4x4 InverseView;
	float4x4 InverseProjection;
	int2 ScreenDimensions;
	float AASampleSpread;
	float DOFSampleSpread;
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
		//initialize the random number generation seed
		uint3 RNGSeed = Input.GlobalThreadID.xyz;
		RNGSeed += InfoBuffer.RNGSeed;
		
		Ray RayInfo;
		Ray OldRayInfo;
		
		for (uint i = 0; i < InfoBuffer.MaxRayPerPixel; i++)
		{
			//get the normalized device coordinates (NDC)
			float2 InvScreenSize = rcp(float2(InfoBuffer.ScreenDimensions));
			float2 ScreenCoords = float2(Input.GlobalThreadID.xy) * InvScreenSize;
			float2 NDC = -2.0f * float2(ScreenCoords.x, ScreenCoords.y) + 1.0f;
			float2 NearNDC = NDC + (Random(RNGSeed.xy) - 0.5f) * InvScreenSize * InfoBuffer.DOFSampleSpread; //for depth of field
			float2 FarNDC = NDC + (Random(RNGSeed.xy) - 0.5f) * InvScreenSize * InfoBuffer.AASampleSpread; //for anti-aliasing
		
			float4x4 InverseViewProjection = mul(InfoBuffer.InverseProjection, InfoBuffer.InverseView);
		
			float4 NearPoint = mul(float4(NearNDC, 0.0f, 1.0f), InverseViewProjection);
			float4 FarPoint = mul(float4(FarNDC, 1.0f, 1.0f), InverseViewProjection);
			NearPoint = NearPoint / NearPoint.w;
			FarPoint = FarPoint / FarPoint.w;
		
			RayInfo.Direction = normalize(FarPoint.xyz - NearPoint.xyz);
			RayInfo.Origin = NearPoint.xyz + mul(InfoBuffer.InverseView, float4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
			RayInfo.TMin = 0.0f;
			RayInfo.TMax = length(FarPoint.xyz);
			OldRayInfo.Direction = float3(0.0f, 0.0f, 0.0f);
			OldRayInfo.Origin = float3(0.0f, 0.0f, 0.0f);
			OldRayInfo.TMin = 0.0f;
			OldRayInfo.TMax = asfloat(i);
		
			uint FlattenedIndex = mad(Input.GlobalThreadID.y, InfoBuffer.ScreenDimensions.x, Input.GlobalThreadID.x) * InfoBuffer.MaxRayPerPixel + i;
			GeneratedRays[FlattenedIndex] = RayInfo;
			OldRays[FlattenedIndex] = OldRayInfo;
			SetRayPixel(FlattenedIndex, Input.GlobalThreadID.xy);
		}
	}
}