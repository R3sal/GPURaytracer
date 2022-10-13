
#include "Sort.hlsli"


#define GROUPSIZE_X 256
#define GROUPSIZE_Y 1
#define GROUPSIZE_Z 1


//shader resources and UAVs
ConstantBuffer<SortInfo> InfoBuffer : register(b0, space0);
RWStructuredBuffer<uint4> MortonCodes : register(u5, space0);
RWStructuredBuffer<uint4> TempMortonCodes : register(u6, space0);
RWStructuredBuffer<uint4> CodeFrequencies : register(u7, space0);


groupshared uint TempCodeFrequencies[GROUPSIZE_X];



[numthreads(GROUPSIZE_X, GROUPSIZE_Y, GROUPSIZE_Z)]
void main(CSInput Input)
{
	//reset the code frequencies
	TempCodeFrequencies[Input.GroupThreadID.x] = 0;
	
	//a barrier, to prevent threads from counting before the frequencies are reset
	GroupMemoryBarrierWithGroupSync();
	
	if (Input.GlobalThreadID.x < InfoBuffer.NumElements)
	{
		uint2 CurrentElement = MortonCodes[Input.GlobalThreadID.x].xy;
		TempMortonCodes[Input.GlobalThreadID.x].xy = CurrentElement;
		uint CurrentIndex = (CurrentElement.x >> (8 * InfoBuffer.SortPassIndex)) & 0xff;
		InterlockedAdd(TempCodeFrequencies[CurrentIndex], 1);
	}
	
	GroupMemoryBarrierWithGroupSync();
	
	//update the output buffer with the code frequencies
	InterlockedAdd(CodeFrequencies[Input.GroupThreadID.x].x, TempCodeFrequencies[Input.GroupThreadID.x]);
}