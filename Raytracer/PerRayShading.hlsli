
#define EPSILON 1e-6f
#define PI 3.141592654f
#define ZERO float4(0.0f, 0.0f, 0.0f, 0.0f)



struct ShaderInput
{
	float Clockwiseability; // pixel on counterclockwise triangle: -1.0f, else 1.0f
	float2 TextureUV;
	float3 Normal;
	float3 Tangent;
	float3 OldRayDirection; // the V in the equations (normalized)
	float3 NewRayDirection; // the L in the equations (normalized)
	uint MaterialID;
};


struct ShaderOutput
{
	float3 Scattered;
	float3 Emitted;
};


struct PBRMaterialProperties
{
	float3 Albedo;
	float Roughness;
	float3 F0Color;
	float Metallic;
	float3 Emissive;

	uint AlbedoTextureID;
	uint RoughnessTextureID;
	uint F0TextureID;
	uint MetallicTextureID;
	uint EmissiveTextureID;
};

struct TextureID
{
	uint Offset;
	uint Width;
	uint Height;
	uint RowPitch;
};



StructuredBuffer<PBRMaterialProperties> PBRMaterials : register(t2, space0);
StructuredBuffer<TextureID> TextureIDs : register(t3, space0);
StructuredBuffer<uint4> TextureAtlas : register(t4, space0);



//functions for texture sampling
//nearest point sampling with texture wrapping is simulated here
float3 SampleTexture(TextureID TextureSampleInfo, float2 UV)
{
	uint2 SampleLocation = uint2(round(UV * float2(TextureSampleInfo.Width - 1, TextureSampleInfo.Height - 1)));
	uint Location = SampleLocation.x + TextureSampleInfo.RowPitch * SampleLocation.y + TextureSampleInfo.Offset;
	uint4 PixelValues = TextureAtlas.Load(int(Location / 2));
	if (Location & 1) // if "Location" is an odd number
	{
		PixelValues.xy = PixelValues.zw;
	}
	float3 Color = ZERO.xyz;
	Color.xz = float2(PixelValues.xy & 0x0000ffff);
	Color.y = float(PixelValues.x >> 16);
	Color *= 1.5259022e-5f; // = Color / 65535; brings the value of the color into the range from 0 to 1
	return Color * Color; //approximate gamma correction
}


//shading functions
//a fresnel term
float3 Fresnel_Schlick(float VdotH, float3 F0)
{
	return F0 + (1.0f - F0) * pow(saturate(1.0f - VdotH), 5.0f);
}

//a normal distribution function
float NDF_TrowbridgeReitz(float NdotH, float Roughness)
{
	float a2 = pow(Roughness, 4.0f);
	
	float Denominator = (NdotH * NdotH) * (a2 - 1.0f) + 1.0f;
	
	return a2 / (PI * Denominator * Denominator + EPSILON);
}

//a visibility term
float VT_Schlick(float NdotV, float NdotL, float Roughness)
{
	float k = (Roughness * Roughness) * 0.5f;
	
	float G_V = NdotV / (NdotV * (1.0f - k) + 1.0f);
	float G_L = NdotL / (NdotL * (1.0f - k) + 1.0f);
	
	return G_V * G_L;
}

//calculate the specular contribution
float3 ReflectedColor(float Roughness, float3 F0Color, float NdotL, float NdotV, float NdotH, float VdotH)
{
	float3 F = Fresnel_Schlick(VdotH, F0Color);
	float D = NDF_TrowbridgeReitz(NdotH, Roughness);
	float G = VT_Schlick(NdotV, NdotL, Roughness);
	float NormalizationFactor = 4.0f * NdotL * NdotV + EPSILON;
	
	return (F * D * G) / NormalizationFactor;
}

//calculate the diffuse contribution (from https://www.gdcvault.com/play/1024478/PBR-Diffuse-Lighting-for-GGX)
float3 RefractedColor(float Roughness, float Metallic, float3 Albedo, float NdotL, float NdotV, float NdotH, float LdotV)
{
	//todo: fix artifacts (currently using lambertian model)
	float Alpha = Roughness * Roughness;
	float Facing = 0.5f + 0.5f * LdotV;
	
	float Rough = Facing * (0.9f - 0.4f * Facing) * ((0.5f + NdotH) / (NdotH + EPSILON));
	float Smooth = 1.05f * (1.0f - pow(1.0f - NdotL, 5.0f)) * (1.0f - pow(1.0f - NdotV, 5.0f));
	
	float Single = lerp(Smooth, Rough, Alpha) / PI;
	float Multi = 0.1159 * Alpha;
	
	return (Albedo / PI) * (1.0f - Metallic);
	//return (Albedo * (Albedo * Multi + Single)) * (1.0f - Metallic);
}



//the main shading function
ShaderOutput Shader(ShaderInput Input, float3 ScatteredLight, float3 EmittedLight)
{
	//get some direction vectors and the distance to the target point
	float3 V = -(Input.OldRayDirection);
	float3 L = Input.NewRayDirection;
	float3 N = Input.Normal;
	float3 H = normalize(V + L); //both V and L are already normalized
	
	//load the material properties at one specific point
	PBRMaterialProperties CurrentMaterial = PBRMaterials[Input.MaterialID];
	CurrentMaterial.Albedo *= SampleTexture(TextureIDs[CurrentMaterial.AlbedoTextureID], Input.TextureUV);
	CurrentMaterial.Roughness *= SampleTexture(TextureIDs[CurrentMaterial.RoughnessTextureID], Input.TextureUV).x;
	CurrentMaterial.F0Color *= SampleTexture(TextureIDs[CurrentMaterial.F0TextureID], Input.TextureUV);
	CurrentMaterial.Metallic *= SampleTexture(TextureIDs[CurrentMaterial.MetallicTextureID], Input.TextureUV).x;
	CurrentMaterial.Emissive *= SampleTexture(TextureIDs[CurrentMaterial.EmissiveTextureID], Input.TextureUV);
	
	//get some needed dot products for further calculations
	float VdotH = saturate(dot(V, H));
	float NdotH = saturate(dot(N, H));
	float NdotV = saturate(dot(N, V));
	float NdotL = saturate(dot(N, L));
	float LdotV = saturate(dot(L, V));
	
	float3 Fr = ReflectedColor(CurrentMaterial.Roughness, CurrentMaterial.F0Color, NdotL, NdotV, NdotH, VdotH);
	float3 Fd = RefractedColor(CurrentMaterial.Roughness, CurrentMaterial.Metallic, CurrentMaterial.Albedo, NdotL, NdotV, NdotH, LdotV);

	/*
	S: scattered light on one ray = (Fr + Fd) * NdotL
	E: emitted light on one ray = CurrentMaterial.Emissive
	N: number of rays / ray depth
	
	N = 2
	L = S1 * (S2 * (E3) + E2) + E1
	  = S1 * S2 * E3 + S1 * E2 + E1
	
	N = 3
	L = S1 * (S2 * (S3 * (E4) + E3) + E2) + E1
	  = S1 * S2 * (S3 * E4 + E3) + S1 * E2 + E1
	  = S1 * S2 * S3 * E4 + S1 * S2 * E3 + S1 * E2 + E1
	
	S = S1 * S2 * ... * S_N
	E = ((S1 * S2 * ... * S_N) * E_N+1) + ((S1 * S2 * ... * S_N-1) * E_N) + ... + S1 * E2 + E1
	
	S = S * S_N
	E = E + S * E_N+1
	*/
	ShaderOutput Output;
	Output.Scattered = ScatteredLight * max(ZERO.xyz, (Fr + Fd) * NdotL);
	Output.Emitted = max(ZERO.xyz, ScatteredLight * CurrentMaterial.Emissive) + EmittedLight;
	//Output.Scattered = ScatteredLight * max(ZERO.xyz, (Fd) * 1.0f);
	//Output.Scattered = ZERO.xyz;
	//Output.Emitted = abs(Input.NewRayDirection);
	return Output;
}