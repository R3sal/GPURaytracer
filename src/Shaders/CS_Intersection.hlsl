
#include "Raytracer.hlsli"


#define GROUPSIZE_X 16
#define GROUPSIZE_Y 16
#define GROUPSIZE_Z 1

#define EPSILON 1e-6f

//the pipeline state
#define INTERSECTION_BASIC 0
#define ENABLE_DEPTH_CLIPPING 1
#define DEPTH_CLIP_NEAR 0.01f
#define DEPTH_CLIP_FAR 1000.0f
#define INTERSECT_CLOCKWISE_FACES 1
#define INTERSECT_COUNTERCLOCKWISE_FACES 1


/*

Ray-Binning in Battlefield V:

Bin index:
4 high bits screen space position / screen offset (two vertical, two horizontal)
16 low bits direction (8 longitude and 8 latitude)

Binning:
Sizes of bins through atomic inc
sideeffect local offset (ray index in bin)

Exclusive parallel sum + some magic
-->
look up table


*/


struct ObjectMatrix
{
	float4x4 Values;
};

struct Triangle
{
	float3 Vertex1;
	float3 Vertex2;
	float3 Vertex3;
};


//shader resources and UAVs
ConstantBuffer<ObjectMatrix> PositionDelta : register(b0, space0);
StructuredBuffer<Triangle> Triangles : register(t0, space0);
RWTexture2D<float4> OutputTexture : register(u0, space0);



#if INTERSECTION_BASIC
//only checks for intersection (supports basic face culling and nothing else (no depth clipping, Interpolation, etc.))
bool Intersects(Ray TestRay, Triangle TestTriangle)
{
	//first calculate some basic values
	bool IntersectionResult = false;
	float3 Edge1 = TestTriangle.Vertex1 - TestRay.Origin;
	float3 Edge2 = TestTriangle.Vertex2 - TestRay.Origin;
	float3 Edge3 = TestTriangle.Vertex3 - TestRay.Origin;

	//calculate, how much the ray points towards the triangle
	float3 Result;
	Result.x = dot(TestRay.Direction, cross(Edge1, Edge2));
	Result.y = dot(TestRay.Direction, cross(Edge2, Edge3));
	Result.z = dot(TestRay.Direction, cross(Edge3, Edge1));

	//do the intersection checks
#if INTERSECT_CLOCKWISE_FACES
	IntersectionResult = all(Result > EPSILON);
#endif
#if INTERSECT_COUNTERCLOCKWISE_FACES
	IntersectionResult = IntersectionResult ? true : all(Result < -EPSILON);
#endif

	//check, if the triangle is in front of the user and abandon this algorithm maybe?????

	return IntersectionResult;
}

#else

//an algorithm with more functionality, while still providing a lot of speed and small memory usage
//the original paper: https://cadxfem.org/inf/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf
float4 Intersects(Ray TestRay, Triangle TestTriangle)
{
	//first define all the needed variables
	float3 Result, Edge1, Edge2, tVec, pVec, qVec;
	float Determinant, InverseDeterminant;
	int TriangleIsClockwise;

	//compute some nescessary vectors
	Edge1 = TestTriangle.Vertex2 - TestTriangle.Vertex1;
	Edge2 = TestTriangle.Vertex3 - TestTriangle.Vertex1;
	tVec = TestRay.Origin - TestTriangle.Vertex1;

	//secondly, compute all the cross products
	pVec = cross(TestRay.Direction, Edge2);
	qVec = cross(tVec, Edge1);
	
	//calculate the determinant and its inverse
	Determinant = dot(Edge1, pVec);
#if INTERSECT_CLOCKWISE_FACES //no face culling
#if INTERSECT_COUNTERCLOCKWISE_FACES
	InverseDeterminant = (abs(Determinant) < EPSILON) ? 0.0f : 1.0f / Determinant;
	TriangleIsClockwise = int(round(abs(Determinant) * InverseDeterminant)); // 1 = clockwise; 0 = parallel to ray; -1 = counterclockwise
#endif
#endif
#if INTERSECT_CLOCKWISE_FACES //counterclockwise faces culling
#if !INTERSECT_COUNTERCLOCKWISE_FACES
	InverseDeterminant = (Determinant > EPSILON) ? 0.0f : 1.0f / Determinant;
#endif
#endif
#if !INTERSECT_CLOCKWISE_FACES // clockwise faces culling
#if INTERSECT_COUNTERCLOCKWISE_FACES
	InverseDeterminant = (Determinant < -EPSILON) ? 0.0f : 1.0f / Determinant;
#endif
#endif

	//calculate the result
	Result.x = dot(Edge2, qVec); //the parameter 't' from the equation of a ray (O + t * R)
	Result.y = dot(tVec, pVec); // 'u', one barycentric coordinate
	Result.z = dot(TestRay.Direction, qVec); // 'v', the other barycentric coordinate
	Result *= InverseDeterminant; // if InverseDeterminant is 0, the next check will fail
	
	//final check, if the ray intersects the triangle
	Result = any(bool4(Result.x < EPSILON, Result.y < 0.0f, Result.z < 0.0f, Result.y + Result.z > 1.0f)) ? float3(-1.0f, -1.0f, -1.0f) : Result;

	return float4(Result, float(TriangleIsClockwise));
}
#endif



[numthreads(GROUPSIZE_X, GROUPSIZE_Y, GROUPSIZE_Z)]
void main(CSInput Input)
{
	uint Width, Height;
	OutputTexture.GetDimensions(Width, Height);
	uint2 FinalTextureSize = uint2(Width, Height);
	
	
	if (all(Input.GlobalThreadID.xy < FinalTextureSize))
	{
		float2 ScreenCoords = float2(Input.GlobalThreadID.xy) / float2(FinalTextureSize);
		float2 ViewportCoords = float2(1.0f - ScreenCoords.x, 1.0f - ScreenCoords.y) * 2.0f - 1.0f;

		Ray CurrRay;
		CurrRay.Origin = float3(0.0f, 0.0f, -2.0f);
		float2 Jitter[4] = { float2(0.25f, 0.25f), float2(-0.25f, 0.25f), float2(0.25f, -0.25f), float2(-0.25f, -0.25f) };

		Triangle CurrTriangle = Triangles[0];
		float4x4 RotMat = PositionDelta.Values;
		CurrTriangle.Vertex1 = mul(RotMat, float4(CurrTriangle.Vertex1, 1.0f)).xyz;
		CurrTriangle.Vertex2 = mul(RotMat, float4(CurrTriangle.Vertex2, 1.0f)).xyz;
		CurrTriangle.Vertex3 = mul(RotMat, float4(CurrTriangle.Vertex3, 1.0f)).xyz;

		float4 Color = float4(0.0f, 0.0f, 0.0f, 0.0f);
		[unroll]
		for (uint i = 0; i < 4; i++)
		{
			CurrRay.Direction = float3(ViewportCoords + (Jitter[i] / float2(FinalTextureSize)), 1.0f);

#if INTERSECTION_BASIC
			Color += float4(1.0f, 1.0f, 0.0f, 1.0f) * Intersects(CurrRay, CurrTriangle);
#else
			Color += float4(1.0f, 1.0f, 1.0f, 1.0f) * saturate(Intersects(CurrRay, CurrTriangle));
#endif
		}

		OutputTexture[Input.GlobalThreadID.xy] = Color * 0.25f;
	}
}