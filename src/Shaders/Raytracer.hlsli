#pragma once


#define EPSILON 1e-6f


struct CSInput
{
	uint3 GroupID : SV_GroupID;
	uint3 GroupThreadID : SV_GroupThreadID;
	uint3 GlobalThreadID : SV_DispatchThreadID;
	uint GroupThreadIndex : SV_GroupIndex;
};

struct Ray
{
	float3 Origin;
	float3 Direction;
	float TMin;
	float TMax;
};

typedef uint Index;

struct Vertex
{
	float3 Position;
	float2 UV;
	float3 Normal;
	float3 Tangent;
	uint MaterialID;
};


struct AABB
{
	float3 Min;
	float3 Max;
	uint2 Padding;
};