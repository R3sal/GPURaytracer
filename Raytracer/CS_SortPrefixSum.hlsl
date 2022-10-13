
#include "Sort.hlsli"


#define GROUPSIZE_X 256
#define GROUPSIZE_Y 1
#define GROUPSIZE_Z 1


//shader resources and UAVs
ConstantBuffer<SortInfo> InfoBuffer : register(b0, space0);
RWStructuredBuffer<uint4> CodeFrequencies : register(u7, space0);


groupshared uint IntermediateBuffer[256];



//prefix sum based on: https://developer.nvidia.com/gpugems/gpugems3/part-vi-gpu-computing/chapter-39-parallel-prefix-sum-scan-cuda
[numthreads(GROUPSIZE_X, GROUPSIZE_Y, GROUPSIZE_Z)]
void main(CSInput Input)
{
	[branch]
	if (Input.GlobalThreadID.x == 0)
	{
		IntermediateBuffer[Input.GlobalThreadID.x] = 0;
	}
	else
	{
		IntermediateBuffer[Input.GlobalThreadID.x] = CodeFrequencies[Input.GlobalThreadID.x - 1].x;
	}
	
	[unroll]
	for (uint i = 0; i < 8; i++)
	{
		GroupMemoryBarrierWithGroupSync();
		
		uint Exp2i = 1 << i;
		uint CurrentValue = 0;
		if (Input.GlobalThreadID.x > Exp2i)
		{
			CurrentValue = IntermediateBuffer[Input.GlobalThreadID.x - Exp2i];
		}
		
		GroupMemoryBarrierWithGroupSync();
		
		if (Input.GlobalThreadID.x > Exp2i)
		{
			InterlockedAdd(IntermediateBuffer[Input.GlobalThreadID.x], CurrentValue);
		}
	}
	
	GroupMemoryBarrierWithGroupSync();
	
	CodeFrequencies[Input.GlobalThreadID.x].y = IntermediateBuffer[Input.GlobalThreadID.x];
}