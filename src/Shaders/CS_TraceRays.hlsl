
#include "PerRayShading.hlsli"
#include "Raytracer.hlsli"
#include "Random.hlsli"


#define GROUPSIZE_X 256
#define GROUPSIZE_Y 1
#define GROUPSIZE_Z 1

#define USE_BVH 1


struct TraceRaysInfo
{
	int2 ScreenDimensions;
	uint NumIndices;
	uint NumRays;
	uint MaxRaysPerPixel;
	uint3 RNGSeed;
};

struct Triangle
{
	float3 Vertex1;
	float3 Vertex2;
	float3 Vertex3;
};


//shader resources and UAVs
ConstantBuffer<TraceRaysInfo> InfoBuffer : register(b0, space0);
StructuredBuffer<Index> Indices : register(t0, space0);
StructuredBuffer<Vertex> Vertices : register(t1, space0);
RWStructuredBuffer<Ray> Rays : register(u0, space0);
RWStructuredBuffer<Ray> OldRays : register(u1, space0); // here, TMin represents the t value in R = t * Direction + Origin
RWStructuredBuffer<uint4> RayPixels : register(u2, space0);
RWStructuredBuffer<float4> ScatteredLight : register(u3, space0);
RWStructuredBuffer<float4> EmittedLight : register(u4, space0);
RWStructuredBuffer<AABB> BoundingVolumeHierarchy : register(u6, space0);



//a fast ray-triangle intersection algorithm, providing a lot of speed and small memory usage
//the original paper: https://cadxfem.org/inf/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf
float4 Intersect(Ray TestRay, Triangle TestTriangle)
{
	//first define all the needed variables
	float3 Result, Edge1, Edge2, tVec, pVec, qVec;
	float Determinant, InverseDeterminant;
	float TriangleIsClockwise;

	//compute some nescessary vectors
	Edge1 = TestTriangle.Vertex2 - TestTriangle.Vertex1;
	Edge2 = TestTriangle.Vertex3 - TestTriangle.Vertex1;
	tVec = TestRay.Origin - TestTriangle.Vertex1;

	//secondly, compute all the cross products
	pVec = cross(TestRay.Direction, Edge2);
	qVec = cross(tVec, Edge1);
	
	//calculate the determinant and its inverse
	Determinant = dot(Edge1, pVec);
	InverseDeterminant = (abs(Determinant) < EPSILON) ? 0.0f : 1.0f / Determinant;
	TriangleIsClockwise = round(abs(Determinant) * InverseDeterminant); // 1 = clockwise; 0 = parallel to ray; -1 = counterclockwise

	//calculate the result
	Result.x = dot(Edge2, qVec); //the parameter 't' from the equation of a ray (O + t * R)
	Result.y = dot(tVec, pVec); // 'u', one barycentric coordinate
	Result.z = dot(TestRay.Direction, qVec); // 'v', the other barycentric coordinate
	Result *= InverseDeterminant; // if InverseDeterminant is 0, the next check will fail
	
	//final check, if the ray intersects the triangle
	Result = any(bool4(Result.x < EPSILON, Result.y < 0.0f, Result.z < 0.0f, Result.y + Result.z > 1.0f)) ? float3(-1.0f, -1.0f, -1.0f) : Result;

	return float4(Result, TriangleIsClockwise);
}


//based on: https://jacco.ompf2.com/2022/04/18/how-to-build-a-bvh-part-2-faster-rays/
float IntersectAABB(Ray TestRay, AABB TestAABB)
{
	float3 t1 = (TestAABB.Min - TestRay.Origin) / TestRay.Direction;
	float3 t2 = (TestAABB.Max - TestRay.Origin) / TestRay.Direction;
	
	float3 tMinVals = min(t1, t2);
	float3 tMaxVals = max(t1, t2);
	float tMin = max(tMinVals.x, max(tMinVals.y, tMinVals.z));
	float tMax = min(tMaxVals.x, min(tMaxVals.y, tMaxVals.z));
	
	if ((tMax < tMin) || (tMax <= 0) || (tMin < TestRay.TMin) || (tMax > TestRay.TMax))
	{
		tMin = 1e30f;
	}
	
	return tMin;
}


void CheckIntersection(Ray CurrentRay, uint CurrentIndex, inout float4 Result, inout uint3 CurrentIndices)
{
	Index Index1 = Indices[CurrentIndex];
	Index Index2 = Indices[CurrentIndex + 1];
	Index Index3 = Indices[CurrentIndex + 2];
	Triangle CurrentTriangle;
	CurrentTriangle.Vertex1 = Vertices[Index1].Position;
	CurrentTriangle.Vertex2 = Vertices[Index2].Position;
	CurrentTriangle.Vertex3 = Vertices[Index3].Position;
	float4 CurrentResult = Intersect(CurrentRay, CurrentTriangle);
	bool UseNewResult = (CurrentRay.TMin <= CurrentResult.x) && (CurrentRay.TMax > CurrentResult.x) && (CurrentResult.x < Result.x);
	Result = UseNewResult ? CurrentResult : Result;
	CurrentIndices = UseNewResult ? uint3(Index1, Index2, Index3) : CurrentIndices;
}


uint2 GetRayPixel(uint RayIndex)
{
	uint2 Position;
	uint4 SavedValue = RayPixels[RayIndex >> 2]; // = RayIndex / 4
	
	SavedValue.xy = (RayIndex % 2) ? SavedValue.yw : SavedValue.xz;
	SavedValue.x = ((RayIndex % 4) < 2) ? SavedValue.x : SavedValue.y;
	Position = uint2(SavedValue.x >> 16, SavedValue.x & 0xffff);
	
	return Position;
}


float1 Interpolate(float1 Attr1, float1 Attr2, float1 Attr3, float2 Barycentrics)
{
	return Attr1 * (1.0f - Barycentrics.x - Barycentrics.y) + Attr2 * (Barycentrics.x) + Attr3 * (Barycentrics.y);
}
float2 Interpolate(float2 Attr1, float2 Attr2, float2 Attr3, float2 Barycentrics)
{
	return Attr1 * (1.0f - Barycentrics.x - Barycentrics.y) + Attr2 * (Barycentrics.x) + Attr3 * (Barycentrics.y);
}
float3 Interpolate(float3 Attr1, float3 Attr2, float3 Attr3, float2 Barycentrics)
{
	return Attr1 * (1.0f - Barycentrics.x - Barycentrics.y) + Attr2 * (Barycentrics.x) + Attr3 * (Barycentrics.y);
}
float4 Interpolate(float4 Attr1, float4 Attr2, float4 Attr3, float2 Barycentrics)
{
	return Attr1 * (1.0f - Barycentrics.x - Barycentrics.y) + Attr2 * (Barycentrics.x) + Attr3 * (Barycentrics.y);
}



[numthreads(GROUPSIZE_X, GROUPSIZE_Y, GROUPSIZE_Z)]
void main(CSInput Input)
{
	if (all(Input.GlobalThreadID.x < InfoBuffer.NumRays))
	{
		Ray CurrentRay = Rays[Input.GlobalThreadID.x];
		Ray OldRay = OldRays[Input.GlobalThreadID.x];
		float4 Result = float4(CurrentRay.TMax, 0.0f, 0.0f, 0.0f);
		uint3 CurrentIndices = uint3(0, 0, 0);
		
#if !USE_BVH //no use of BVH
		
		for (uint i = 0; i < InfoBuffer.NumIndices; i += 3)
		{
			Index Index1 = Indices[i];
			Index Index2 = Indices[i + 1];
			Index Index3 = Indices[i + 2];
			Triangle CurrentTriangle;
			CurrentTriangle.Vertex1 = Vertices[Index1].Position;
			CurrentTriangle.Vertex2 = Vertices[Index2].Position;
			CurrentTriangle.Vertex3 = Vertices[Index3].Position;
			float4 CurrentResult = Intersect(CurrentRay, CurrentTriangle);
			bool UseNewResult = (CurrentRay.TMin <= CurrentResult.x);
			UseNewResult = UseNewResult && (CurrentRay.TMax > CurrentResult.x);
			UseNewResult = UseNewResult && (CurrentResult.x < Result.x);
			Result = UseNewResult ? CurrentResult : Result;
			CurrentIndices = UseNewResult ? uint3(Index1, Index2, Index3) : CurrentIndices;
		}
		
#else //use BVH
		
		uint AABBIndices[48];
		uint NumAABBs = 0;
		
		uint NumTestedAABBs = 0;
		float AABBResult = 1e30f;
		
		AABB TrunkAABB = BoundingVolumeHierarchy[0];
		float AABBIntersectResult = IntersectAABB(CurrentRay, TrunkAABB);
		
		if (AABBIntersectResult != 1e30f)
		{
			AABBIndices[0] = TrunkAABB.Padding.y;
			AABBIndices[1] = TrunkAABB.Padding.x;
			NumAABBs = 2;
		}
		AABBResult = AABBIntersectResult;
		
		while (NumAABBs > 0)
		{
			AABB CurrentAABB = BoundingVolumeHierarchy[AABBIndices[NumAABBs - 1]];
			float CurrentResult = IntersectAABB(CurrentRay, CurrentAABB);
			bool RemoveTestedAABBs = false;
			if ((CurrentResult != 1e30f) && (CurrentResult < Result.x))
			{
				NumTestedAABBs++;
				AABBIntersectResult = min(AABBIntersectResult, CurrentResult);
				AABBIndices[NumAABBs - 1] |= 0x80000000; //indicate that the current AABB was tested for intersection
				if (CurrentAABB.Padding.x & 0x80000000)
				{
					CheckIntersection(CurrentRay, CurrentAABB.Padding.x & 0x7fffffff, Result, CurrentIndices);
					if (CurrentAABB.Padding.y != 0xffffffff)
					{
						CheckIntersection(CurrentRay, CurrentAABB.Padding.y & 0x7fffffff, Result, CurrentIndices);
					}
					RemoveTestedAABBs = true;
					if (CurrentResult < AABBResult)
					{
						AABBResult = CurrentResult;
					}
				}
				else
				{
					if (CurrentAABB.Padding.y == 0xffffffff)
					{
						AABBIndices[NumAABBs] = CurrentAABB.Padding.x;
						NumAABBs++;
					}
					else
					{
						AABBIndices[NumAABBs] = CurrentAABB.Padding.y;
						AABBIndices[NumAABBs + 1] = CurrentAABB.Padding.x;
						NumAABBs += 2;
					}
				}
			}
			else
			{
				RemoveTestedAABBs = true;
			}
			
			if (RemoveTestedAABBs)
			{
				NumAABBs--;
				while ((NumAABBs > 0) && (AABBIndices[NumAABBs - 1] & 0x80000000))
				{
					NumAABBs--;
				}
			}
		}
		
#endif
		
		uint2 Pixel = GetRayPixel(Input.GlobalThreadID.x);
		uint OffsetInPixel = asuint(OldRay.TMax);
		uint Index = mad(InfoBuffer.ScreenDimensions.x, Pixel.y, Pixel.x) * InfoBuffer.MaxRaysPerPixel + OffsetInPixel;
		
		float4 Scattered = ScatteredLight[Index];
		float4 Emitted = EmittedLight[Index];
		
		if (Result.x != CurrentRay.TMax)
		{
			Vertex Vertex1 = Vertices[CurrentIndices.x];
			Vertex Vertex2 = Vertices[CurrentIndices.y];
			Vertex Vertex3 = Vertices[CurrentIndices.z];
			
			//initialize the random number generation seed
			uint3 RNGSeed = Input.GlobalThreadID.xxx;
			RNGSeed *= RNGSeed + 17;
			XorShift(RNGSeed);
			RNGSeed += InfoBuffer.RNGSeed;
			
			//generate an input for our shader function
			ShaderInput ShadingInput;
			ShadingInput.Clockwiseability = Result.w;
			ShadingInput.TextureUV = Interpolate(Vertex1.UV, Vertex2.UV, Vertex3.UV, Result.yz);
			ShadingInput.Normal = Interpolate(Vertex1.Normal, Vertex2.Normal, Vertex3.Normal, Result.yz);
			ShadingInput.Tangent = Interpolate(Vertex1.Tangent, Vertex2.Tangent, Vertex3.Tangent, Result.yz);
			ShadingInput.OldRayDirection = CurrentRay.Direction;
			ShadingInput.NewRayDirection = RotatedRandomDirection(RNGSeed, ShadingInput.Normal);
			ShadingInput.MaterialID = Vertex1.MaterialID;
			
			if (dot(OldRay.Direction, OldRay.Direction) == 0.0f) //this indicates it being the first rays for which we have to reset those values
			{
				Scattered.xyz = float3(1.0f, 1.0f, 1.0f);
				Emitted.xyz = float3(0.0f, 0.0f, 0.0f);
			}
			ShaderOutput Output = Shader(ShadingInput, Scattered.xyz, Emitted.xyz);
			Scattered.xyz = Output.Scattered;
			Emitted.xyz = Output.Emitted;
			
			//generate a new ray
			Ray NewRay;
			NewRay.Direction = ShadingInput.NewRayDirection;
			NewRay.Origin = CurrentRay.Origin + CurrentRay.Direction * Result.x;
			NewRay.TMin = CurrentRay.TMin;
			NewRay.TMax = CurrentRay.TMax;
			CurrentRay.TMax = OldRay.TMax;
			Rays[Input.GlobalThreadID.x] = NewRay;
			OldRays[Input.GlobalThreadID.x] = CurrentRay;
		}
		
		ScatteredLight[Index] = Scattered;
		//EmittedLight[Index] = Emitted;
		EmittedLight[Index] = float4((float(AABBResult)) * 0.01f, 0.0f, 0.0f, 0.0f);
		//EmittedLight[Index] = float4(float(TrunkAABB.Padding.y) * 0.0003, 0.0f, 0.0f, 0.0f);
		/*/if (Input.GlobalThreadID.x < 4096)
		{
			AABB CurrAABB = BoundingVolumeHierarchy[Input.GlobalThreadID.x + 1];
			EmittedLight[Input.GlobalThreadID.x * 8] = float4(0.0f, float(CurrAABB.Padding.x & 0x7fffffff) / 1000000000.0f, 0.0f, 0.0f);
			EmittedLight[Input.GlobalThreadID.x * 8 + 4] = float4(0.0f, float(CurrAABB.Padding.y & 0x7fffffff) / 1000000000.0f, 0.0f, 0.0f);
		}/**/
	}
}