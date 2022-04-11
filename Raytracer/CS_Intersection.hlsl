
#define GROUPSIZE_X 16
#define GROUPSIZE_Y 16
#define GROUPSIZE_Z 1

#define EPSILON 1e-6f

//the pipeline state
#define INTERSECTION_BASIC 0
#define ENABLE_DEPTH_CLIPPING 1
#define DEPTH_CLIP_NEAR 0.01f
#define DEPTH_CLIP_FAR 1000.0f
#define INTERSECT_CLOCKWISE_FACES
#define INTERSECT_COUNTERCLOCKWISE_FACES


//the input
struct CSInput
{
	uint3 GroupID : SV_GroupID;
	uint3 GroupThreadID : SV_GroupThreadID;
	uint3 GlobalThreadID : SV_DispatchThreadID;
	uint GroupThreadIndex : SV_GroupIndex;
};


//shader resources and UAVs
RWTexture2D<float4> OutputTexture : register(u0, space0);



//some additional stuff
struct Ray
{
	float3 Origin;
	float3 Direction;
};

struct Triangle
{
	float3 Vertex1;
	float3 Vertex2;
	float3 Vertex3;
};


#if INTERSECTION_BASIC
//only checks for intersection (supports basic face culling and nothing else (no depth clipping, Interpolation, etc.))
bool Intersects(Ray TestRay, Triangle TestTriangle)
{
	//first calculate some basic values
	bool IntersectionResult = true;
	float3 Edge1 = TestTriangle.Vertex1 - TestRay.Origin;
	float3 Edge2 = TestTriangle.Vertex2 - TestRay.Origin;
	float3 Edge3 = TestTriangle.Vertex3 - TestRay.Origin;

	//calculate, how much the ray points towards the triangle
	float Result1 = dot(TestRay.Direction, cross(Edge1, Edge2));
	float Result2 = dot(TestRay.Direction, cross(Edge2, Edge3));
	float Result3 = dot(TestRay.Direction, cross(Edge3, Edge1));

	//do the intersection checks
#ifdef INTERSECT_CLOCKWISE_FACES
	IntersectionResult = all(float3(Result1, Result2, Result3) > EPSILON);
#endif
#ifdef INTERSECT_COUNTERCLOCKWISE_FACES
	IntersectionResult = IntersectionResult ? true : all(float3(Result1, Result2, Result3) < -EPSILON);
#endif

	//check, if the triangle is in front of the user and abandon this algorithm maybe?????

	return IntersectionResult;
}

#else

//an algorithm with more functionality, while still providing a lot of speed and small memory usage
//the original paper: https://cadxfem.org/inf/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf
float3 Intersects(Ray TestRay, Triangle TestTriangle)
{
	//first define all the needed variables
	float3 Result, Edge1, Edge2, tVec, pVec, qVec;
	float Determinant, InverseDeterminant;

	//compute some nescessary vectors
	Edge1 = TestTriangle.Vertex2 - TestTriangle.Vertex1;
	Edge2 = TestTriangle.Vertex3 - TestTriangle.Vertex1;
	tVec = TestRay.Origin - TestTriangle.Vertex1;

	//secondly, compute all the cross products
	pVec = cross(TestRay.Direction, Edge2);
	qVec = cross(tVec, Edge1);

	//calculate the determinant and its inverse
	Determinant = dot(Edge1, pVec);
#ifdef INTERSECT_CLOCKWISE_FACES //no face culling
#ifdef INTERSECT_COUNTERCLOCKWISE_FACES
	InverseDeterminant = ((Determinant > -EPSILON) && (Determinant < EPSILON)) ? 0.0f : 1.0f / Determinant;
#endif
#endif
#ifdef INTERSECT_CLOCKWISE_FACES //counterclockwise faces culling
#ifndef INTERSECT_COUNTERCLOCKWISE_FACES
	InverseDeterminant = (Determinant > -EPSILON) ? 0.0f : 1.0f / Determinant;
#endif
#endif
#ifndef INTERSECT_CLOCKWISE_FACES // clockwise faces culling
#ifdef INTERSECT_COUNTERCLOCKWISE_FACES
	InverseDeterminant = (Determinant < EPSILON) ? 0.0f : 1.0f / Determinant;
#endif
#endif

	//calculate the result
	Result.x = -dot(Edge2, qVec); //the parameter 't' from the equation of a ray (O + t * R) // !!!!!!!!!!!!!!!!!!!!!!!!!something strange!!!!!!!!!!!!!!! (the minus)
	Result.y = dot(tVec, pVec); // 'u', one barycentric coordinate
	Result.z = dot(TestRay.Direction, qVec); // 'v', the other barycentric coordinate
	Result = (InverseDeterminant == 0.0f) ? float3(-1.0f, -1.0f, -1.0f) : Result * InverseDeterminant;

	//final check, if the ray intersects the triangle
	Result = ((Result.x < EPSILON) || (Result.y < 0.0f) || (Result.z < 0.0f) || (Result.y + Result.z > 1.0f)) ? float3(-1.0f, -1.0f, -1.0f) : Result;

	return Result;
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
		float2 ViewportCoords = float2(ScreenCoords.x, 1.0f - ScreenCoords.y) * 2.0f - 1.0f;

		Ray CurrRay;
		CurrRay.Direction = float3(0.0f, 0.0f, -1.0f);
		float2 Jitter[4] = { float2(0.25f, 0.25f), float2(-0.25f, 0.25f), float2(0.25f, -0.25f), float2(-0.25f, -0.25f) };

		Triangle CurrTriangle;
		CurrTriangle.Vertex1 = float3(0.0f, 0.5f, 1.0f);
		CurrTriangle.Vertex2 = float3(0.5f, -0.5f, 1.0f);
		CurrTriangle.Vertex3 = float3(-0.5f, -0.5f, 1.0f);

		float4 Color = float4(0.0f, 0.0f, 0.0f, 0.0f);
		[unroll]
		for (uint i = 0; i < 4; i++)
		{
			CurrRay.Origin = float3(ViewportCoords + Jitter[i] / float2(FinalTextureSize), 0.0f);

#if INTERSECTION_BASIC
			Color += float4(1.0f, 1.0f, 0.0f, 1.0f) * Intersects(CurrRay, CurrTriangle);
#else
			Color += float4(1.0f, 1.0f, 1.0f, 1.0f) * float4(saturate(Intersects(CurrRay, CurrTriangle)), 1.0f);
#endif
		}

		OutputTexture[Input.GlobalThreadID.xy] = Color * 0.25f;
	}
}