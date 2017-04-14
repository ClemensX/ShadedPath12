#include "Object.hlsli"

[RootSignature(ObjectRS)]

float4 main(PSInput input) : SV_TARGET
{
	float2 thisTexCoord = input.Tex;
	float3 texColor;
	texColor = screenTex.Sample(s, thisTexCoord).xyz;
	float3 finalColor;
	  //return float4(texColor,1);
	finalColor = applyLighting(texColor, input.Pos.xyz, input.PosW, input.Normal, cbv.cameraPos);

	float4 alphaColor;
	alphaColor.rgb = finalColor;
	alphaColor.a = cbv.alpha;
	//return float4(0.003125, 0.003125, 0.003125, 1);
	return clamp(alphaColor, 0.0021973, 1.0);  // prevent strange smearing effect for total black pixels (only in HMD)
}

