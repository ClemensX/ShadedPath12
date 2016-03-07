#include "Object.hlsli"

[RootSignature(ObjectRS)]

float4 main(PSInput input) : SV_TARGET
{
	float2 thisTexCoord = input.Tex;
	float3 texColor;
	texColor = screenTex.Sample(s, thisTexCoord).xyz;
	float3 finalColor;
	finalColor = applyLighting(texColor, input.Pos.xyz, input.Normal, cbv.cameraPos);

	float4 alphaColor;
	alphaColor.rgb = finalColor;
	alphaColor.a = cbv.alpha;
	return alphaColor;
}

