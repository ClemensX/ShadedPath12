#include "Object.hlsli"

[RootSignature(ObjectRS)]

float4 main(PSInput input) : SV_TARGET
{
	float2 thisTexCoord = input.Tex;
	float3 texColor;
	texColor = screenTex.Sample(s, thisTexCoord).xyz;
	float3 finalColor;
	finalColor = applyLighting(texColor, input.Pos.xyz, input.PosW, input.Normal, cbv.cameraPos);

	float4 alphaColor;
	alphaColor.rgb = finalColor;
	alphaColor.a = cbv.alpha;
	//return alphaColor;
	//return float4(0.003125, 0.003125, 0.003125, 1);
	//return float4(0.0021784, 0.0021784, 0.0021784, 1);
	//float f = 0.0021973;
	//float f = 0;
	//return clamp(float4(f, f, f, 1), 0.0021973, 1.0);
	return clamp(alphaColor, 0.0021973, 1.0);  // prevent strange smearing effect for total black pixels (only in HMD)
}

