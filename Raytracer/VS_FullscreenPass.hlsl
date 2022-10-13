
#include "FullscreenPass.hlsli"



PSInput main(uint VertexIndex : SV_VertexID)
{
	PSInput Output;

	float HelperIndex = float(VertexIndex) * 2.0f - 2.0f;
	Output.Position.x = HelperIndex; // = -2, 0, 2
	Output.Position.y = (-HelperIndex) * HelperIndex + 3.0f; // = -1, 3, -1
	Output.Position.zw = float2(0.0f, 1.0f);
	Output.UV.x = float(VertexIndex) - 0.5f; // -0.5, 0.5, 1.5
	Output.UV.y = abs(HelperIndex) - 1.0f; // 1, -1, 1

	return Output;
}