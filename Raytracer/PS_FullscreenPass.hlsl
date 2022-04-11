
#include "FullscreenPass.hlsli"



RWTexture2D<float4> InputTexture : register(u0, space0);



float4 main(PSInput Input) : SV_TARGET
{
	/**/
	uint Width;
	uint Height;
	InputTexture.GetDimensions(Width, Height);

	float4 Color = InputTexture.Load(int3(floor(Input.UV * float2(Width, Height)), 0));
	/*/
	float4 Color = float4(0.2f, 0.4f, 0.8f, 1.0f);
	/**/

	return Color;
}