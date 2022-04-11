
struct VSInput
{
	float2 Position : POSITION;
	float2 UV : TEXCOORD0;
};

struct PSInput
{
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
};