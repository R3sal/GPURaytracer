
#include "FullscreenPass.hlsli"



PSInput main(VSInput Input)
{
	PSInput Output;

	Output.Position = float4(Input.Position, 0.0f, 1.0f);
	Output.UV = Input.UV;

	return Output;
}