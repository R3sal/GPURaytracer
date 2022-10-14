
//implements a basic xorshift algorithm for pseudo random numbers: http://www.jstatsoft.org/v08/i14/paper
uint1 XorShift(inout uint1 Seed)
{
	Seed ^= Seed << 13;
	Seed ^= Seed >> 17;
	Seed ^= Seed << 5;
	return Seed;
}
uint2 XorShift(inout uint2 Seed)
{
	Seed ^= Seed << 13;
	Seed ^= Seed >> 17;
	Seed ^= Seed << 5;
	return Seed;
}
uint3 XorShift(inout uint3 Seed)
{
	Seed ^= Seed << 13;
	Seed ^= Seed >> 17;
	Seed ^= Seed << 5;
	return Seed;
}
uint4 XorShift(inout uint4 Seed)
{
	Seed ^= Seed << 13;
	Seed ^= Seed >> 17;
	Seed ^= Seed << 5;
	return Seed;
}

//gives the output in the gaussian distribution for a given input
float1 GaussianDistribution(float1 Input, float1 Mean, float1 StandartDeviation)
{
	float1 InverseDeviation = rcp(StandartDeviation);
	float1 Exponent = (Input - Mean) * InverseDeviation;
	float1 ScaleFactor = 0.39894228f * InverseDeviation; // = 1 / (Deviation * sqrt(2 * PI))
	return ScaleFactor * exp(-0.5f * Exponent * Exponent);
}
float2 GaussianDistribution(float2 Input, float2 Mean, float2 StandartDeviation)
{
	float2 InverseDeviation = rcp(StandartDeviation);
	float2 Exponent = (Input - Mean) * InverseDeviation;
	float2 ScaleFactor = 0.39894228f * InverseDeviation; // = 1 / (Deviation * sqrt(2 * PI))
	return ScaleFactor * exp(-0.5f * Exponent * Exponent);
}
float3 GaussianDistribution(float3 Input, float3 Mean, float3 StandartDeviation)
{
	float3 InverseDeviation = rcp(StandartDeviation);
	float3 Exponent = (Input - Mean) * InverseDeviation;
	float3 ScaleFactor = 0.39894228f * InverseDeviation; // = 1 / (Deviation * sqrt(2 * PI))
	return ScaleFactor * exp(-0.5f * Exponent * Exponent);
}
float4 GaussianDistribution(float4 Input, float4 Mean, float4 StandartDeviation)
{
	float4 InverseDeviation = rcp(StandartDeviation);
	float4 Exponent = (Input - Mean) * InverseDeviation;
	float4 ScaleFactor = 0.39894228f * InverseDeviation; // = 1 / (Deviation * sqrt(2 * PI))
	return ScaleFactor * exp(-0.5f * Exponent * Exponent);
}

//returns a random number between 0 and 1 using the xorshift algorithm
float1 Random(inout uint1 Seed)
{
	return saturate(float1(XorShift(Seed)) * 2.3283064e-10f);
}
float2 Random(inout uint2 Seed)
{
	return saturate(float2(XorShift(Seed)) * 2.3283064e-10f);
}
float3 Random(inout uint3 Seed)
{
	return saturate(float3(XorShift(Seed)) * 2.3283064e-10f);
}
float4 Random(inout uint4 Seed)
{
	return saturate(float4(XorShift(Seed)) * 2.3283064e-10f);
}

//returns a random number in a gaussian distribution
float1 RandomGaussian(inout uint1 Seed, float1 Mean, float1 Deviation, float1 InputRange = float1(10.0f))
{
	return GaussianDistribution(Random(Seed) * InputRange + 0.5f * InputRange, Mean, Deviation);
}
float2 RandomGaussian(inout uint2 Seed, float2 Mean, float2 Deviation, float2 InputRange = float2(10.0f, 10.0f))
{
	return GaussianDistribution(Random(Seed) * InputRange + 0.5f * InputRange, Mean, Deviation);
}
float3 RandomGaussian(inout uint3 Seed, float3 Mean, float3 Deviation, float3 InputRange = float3(10.0f, 10.0f, 10.0f))
{
	return GaussianDistribution(Random(Seed) * InputRange + 0.5f * InputRange, Mean, Deviation);
}
float4 RandomGaussian(inout uint4 Seed, float4 Mean, float4 Deviation, float4 InputRange = float4(10.0f, 10.0f, 10.0f, 10.0f))
{
	return GaussianDistribution(Random(Seed) * InputRange + 0.5f * InputRange, Mean, Deviation);
}

//functions for returning a point on a hemisphere
//uniform point sampling on a hemisphere, courtesy of https://raytracing.github.io/books/RayTracingTheRestOfYourLife.html
float3 PointOnHemisphere(inout uint3 Seed)
{
	float3 RandomNumber = Random(Seed);
	float y = sqrt(1.0f - RandomNumber.y);
	float r = sqrt(RandomNumber.y);
	float phi = 6.2831853f * RandomNumber.x;
	return float3(cos(phi) * r, y, sin(phi) * r);
}
float3 RotatedRandomDirection(inout uint3 Seed, float3 Normal)
{
	//this is always perpendicular to the normal since
	// dot(Normal, Perpendicular1)
	// == Normal.x * Perpendicular1.x + Normal.y * Perpendicular1.y + Normal.z * Perpendicular1.z
	// == Normal.x * (-Normal.z) + Normal.y * 0.0f + Normal.z * Normal.x
	// == -(Normal.x * Normal.z) + 0.0f + Normal.x * Normal.z
	// == (Normal.x * Normal.z) - (Normal.x * Normal.z)
	// == 0.0f
	// --> "Perpendicular1" is always perpendicular to "Normal" 
	float3 Perpendicular1 = normalize(float3(-Normal.z, 0.0f, Normal.x));
	if (all(Normal.zx == float2(0.0f, 0.0f))) //if the normal points straight up or down the code above generates an unusable vector
		Perpendicular1 = float3(1.0f, 0.0f, 0.0f);
	float3 Perpendicular2 = normalize(cross(Normal, Perpendicular1));
	Perpendicular1 = normalize(cross(Normal, Perpendicular2));
	
	float3 BasePoint = PointOnHemisphere(Seed);
	return (BasePoint.x * Perpendicular1) + (BasePoint.y * Normal) + (BasePoint.z * Perpendicular2);
}