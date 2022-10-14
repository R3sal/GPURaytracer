
#include "Raytracer.hlsli"


#define GROUPSIZE_X 256
#define GROUPSIZE_Y 1
#define GROUPSIZE_Z 1


struct MortonCodeInfo
{
	float4 SceneMin;
	float4 SceneMax;
	uint NumPrimitives;
	uint3 Padding;
};


//shader resources and UAVs
ConstantBuffer<MortonCodeInfo> InfoBuffer : register(b0, space0);
StructuredBuffer<Index> Indices : register(t0, space0);
StructuredBuffer<Vertex> Vertices : register(t1, space0);
RWStructuredBuffer<uint4> MortonCodes : register(u5, space0);
RWStructuredBuffer<uint4> CodeFrequencies : register(u7, space0);



//make following bitshifts: 0b00000111 --> 0b01001001
//based on https://pbr-book.org/3ed-2018/Primitives_and_Intersection_Acceleration/Bounding_Volume_Hierarchies
uint3 LeftShift3(in uint3 Input)
{
	Input |= (Input << 16) & 0x030000ff; // = 0b00000011000000000000000011111111
	Input |= (Input << 8) & 0x0300f00f; // = 0b00000011000000001111000000001111
	Input |= (Input << 4) & 0x030c30c3; // = 0b00000011000011000011000011000011
	Input |= (Input << 2) & 0x09249249; // = 0b00001001001001001001001001001001
	
	return Input;
}



[numthreads(GROUPSIZE_X, GROUPSIZE_Y, GROUPSIZE_Z)]
void main(CSInput Input)
{
	//if (1331 > InfoBuffer.NumPrimitives)
	if (Input.GlobalThreadID.x < InfoBuffer.NumPrimitives)
	{
		//get the vertices
		uint CurrentIndex = Input.GlobalThreadID.x * 3;
		Index Index1 = Indices[CurrentIndex];
		Index Index2 = Indices[CurrentIndex + 1];
		Index Index3 = Indices[CurrentIndex + 2];
		Vertex Vertex1 = Vertices[Index1];
		Vertex Vertex2 = Vertices[Index2];
		Vertex Vertex3 = Vertices[Index3];
		
		//calculate the centroid of the vertices
		float3 Centroid = 0.333333f * (Vertex1.Position + Vertex2.Position + Vertex3.Position);
		
		//normalize the centroid
		float3 NormalizedCentroid = saturate((Centroid - InfoBuffer.SceneMin.xyz) / (InfoBuffer.SceneMax.xyz - InfoBuffer.SceneMin.xyz)) * 1023.0f;
		uint3 IntegerizedCentroid = uint3(NormalizedCentroid); //hlsl limit on maximum written float
		
		//generate the morton code
		//also based on https://pbr-book.org/3ed-2018/Primitives_and_Intersection_Acceleration/Bounding_Volume_Hierarchies
		uint3 ShiftedCentroid = LeftShift3(IntegerizedCentroid);
		uint MortonCode = (ShiftedCentroid.z << 2) | (ShiftedCentroid.y << 1) | ShiftedCentroid.x;

		//store the generated morton code
		MortonCodes[Input.GlobalThreadID.x] = uint4(MortonCode, CurrentIndex, 0, 0);
		//MortonCodes[Input.GlobalThreadID.x] = uint4(100000 * uint(abs(InfoBuffer.SceneMin.z)), 100000, 100000, 100000);
	}
	
	if (Input.GlobalThreadID.x < 256)
	{
		CodeFrequencies[Input.GlobalThreadID.x].x = 0;
	}
}