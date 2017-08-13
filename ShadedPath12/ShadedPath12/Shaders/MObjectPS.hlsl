#include "lights_basic.hlsi"
#include "MObject.hlsli"

// named ObjectConstantBuffer for historic reasons, is UAV here, really
RWStructuredBuffer<ObjectConstantBuffer> cbvResult	: register(u0);	// read-only UAV

[RootSignature(ObjectRS)]

float4 main(PSInput input) : SV_TARGET
{
/*	Material m;
	m.ambient = float4(1,1,1,1);
	m.specExp = 0;
	m.specIntensity = 0;
	m.isLightSource = 0;
	m.fill2 = 0;
*/	float2 thisTexCoord = input.Tex;
	float3 texColor;
	texColor = screenTex.Sample(s, thisTexCoord).xyz;
	float3 finalColor = float4(1,1,1,1);
	  //return float4(texColor,1);
	uint id = asuint(input.Id);
	id = 1;
	//finalColor = applyLighting(texColor, input.Pos.xyz, input.PosW, input.Normal, cbv[id].cameraPos, cbv[id].material);
	finalColor = applyLighting(texColor, input.Pos.xyz, input.PosW, input.Normal, cbvResult[id].cameraPos, cbvResult[id].material);
	//finalColor = float4(1,1,1,1);

	float4 alphaColor;
	alphaColor.rgb = finalColor;
	//alphaColor.a = cbv[id].alpha;
	alphaColor.a = cbvResult[id].alpha;
	//return float4(0.003125, 0.003125, 0.003125, 1);
	return clamp(alphaColor, 0.0021973, 1.0);  // prevent strange smearing effect for total black pixels (only in HMD)
}

