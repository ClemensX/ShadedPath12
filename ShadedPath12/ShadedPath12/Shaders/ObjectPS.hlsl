#include "Object.hlsli"

[RootSignature(ObjectRS)]

float3 calcDirectional(float3 pos, float3 normal, Material material)
{
	return pos;
}

float4 main(PSInput input) : SV_TARGET
{
	//float s = something[0].x;
	float2 thisTexCoord = input.Tex;
	//thisTexCoord = float2(1, 1);
	float3 texColor;
	texColor = screenTex.Sample(s, thisTexCoord);
	texColor = applyLighting(texColor);
	//texColor = saturate(texColor);
	//texColor.r = texColor.r + 1;
	//texColor.g = texColor.g + 1;
	//texColor.b = texColor.b + 1;
	//texColor = float4(34.0f/256.0f, 177.0f/256.0f, 76.0f/256.0f, 1);
	//texColor = float4(1.0f, 0.0f, 0.0f, 1);
	//texColor = float4(0.0f/256.0f, 0.0f/256.0f, 1.0f/256.0f, 1);
	//texColor = float4(0, 0, 100, 1);
	//return saturate(2.0f * texColor);
	//return texColor * texColor;  // gamma correction of 2.0
	float4 alphaColor;
	alphaColor.rgb = texColor;
	alphaColor.a = cbv.alpha;// +something[0].x;
	return alphaColor;
}

