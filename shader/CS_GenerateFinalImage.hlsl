
#include "Raytracer.hlsli"
#include "Random.hlsli"


#define GROUPSIZE_X 16
#define GROUPSIZE_Y 16
#define GROUPSIZE_Z 1


struct ImageGenerationInfo
{
	int2 ScreenDimensions;
	uint MaxRaysPerPixel;
	uint NumSamples;
};


//shader resources and UAVs
ConstantBuffer<ImageGenerationInfo> InfoBuffer : register(b0, space0);
RWStructuredBuffer<float4> ScatteredLight : register(u3, space0);
RWStructuredBuffer<float4> EmittedLight : register(u4, space0);
RWStructuredBuffer<float4> ResultBuffer : register(u5, space0);
RWTexture2D<float4> OutputTexture : register(u6, space0);



[numthreads(GROUPSIZE_X, GROUPSIZE_Y, GROUPSIZE_Z)]
void main(CSInput Input)
{
	if (all(Input.GlobalThreadID.xy < InfoBuffer.ScreenDimensions))
	{
		uint ResultBufferIndex = mad(InfoBuffer.ScreenDimensions.x, Input.GlobalThreadID.y, Input.GlobalThreadID.x);
		float4 TotalColor = ResultBuffer[ResultBufferIndex];
		float4 NewColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
		
		uint Index = ResultBufferIndex * InfoBuffer.MaxRaysPerPixel;
		for (uint i = 0; i < InfoBuffer.MaxRaysPerPixel; i++)
		{
			//add up all results that contribute to this pixel
			NewColor += EmittedLight[Index + i];
		}
		
		//average with the new color
		NewColor /= float(InfoBuffer.MaxRaysPerPixel);
		uint NumSamples = InfoBuffer.NumSamples & 0x7fffffff;
		float InverseNumSamples = rcp(float(NumSamples));
		float4 DisplayedColor = (float(NumSamples - 1) * InverseNumSamples) * TotalColor + InverseNumSamples * NewColor;
		if ((InfoBuffer.NumSamples & 0x80000000) > 0)
		{
			TotalColor = DisplayedColor;
		}
		
		//if (length(NewColor.xyz) > 100.0f)
		//	DisplayedColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
		//OutputTexture[Input.GlobalThreadID.xy] = NewColor;
		//OutputTexture[Input.GlobalThreadID.xy] = DisplayedColor;
		OutputTexture[Input.GlobalThreadID.xy] = DisplayedColor / (1.0f + DisplayedColor); // basic tone mapping
		ResultBuffer[ResultBufferIndex] = TotalColor;
	}
}