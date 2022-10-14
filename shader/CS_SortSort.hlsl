
#include "Sort.hlsli"


#define GROUPSIZE_X 256
#define GROUPSIZE_Y 1
#define GROUPSIZE_Z 1


//shader resources and UAVs
ConstantBuffer<SortInfo> InfoBuffer : register(b0, space0);
RWStructuredBuffer<uint4> MortonCodes : register(u5, space0);
RWStructuredBuffer<uint4> TempMortonCodes : register(u6, space0);
RWStructuredBuffer<uint4> CodeFrequencies : register(u7, space0);



[numthreads(GROUPSIZE_X, GROUPSIZE_Y, GROUPSIZE_Z)]
void main(CSInput Input)
{
	uint NextElementIndex = CodeFrequencies[Input.GlobalThreadID.x].y;
	
	for (uint i = 0; i < InfoBuffer.NumElements; i++)
	{
		uint2 CurrentElement = TempMortonCodes[i].xy;
		uint CurrentIndex = (CurrentElement.x >> (8 * InfoBuffer.SortPassIndex)) & 0xff;
		if (CurrentIndex == Input.GlobalThreadID.x)
		{
			MortonCodes[NextElementIndex].xy = CurrentElement;
			NextElementIndex++;
		}
	}
	
	CodeFrequencies[Input.GlobalThreadID.x].x = 0;
}