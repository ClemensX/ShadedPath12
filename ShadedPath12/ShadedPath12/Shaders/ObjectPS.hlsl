#include "Object.hlsli"

[RootSignature(ObjectRS)]

float3 calcDirectional(float3 pos, float3 normal, Material material, int index, float3 camera)
{
	float3 dirToLight = directionalLights[index].pos.xyz;// - pos;
	dirToLight = normalize(dirToLight);
	// Phong diffuse:
	float NDotL = dot(dirToLight, normal);
	float3 finalColor = directionalLights[index].color.rgb * saturate(NDotL);

	// Blinn specular:
	float3 toEye = camera - pos;
	toEye = normalize(toEye);
	float3 halfWay = normalize(toEye + dirToLight);
	float NDotH = saturate(dot(halfWay, normal));
	finalColor += directionalLights[index].color.rgb * pow(NDotH, material.specExp) * material.specIntensity;

	return finalColor;

	//return float3(0.5,0.5,0.5);
	//return directionalLights[index].color;
	//return cbvLights.ambient[0].ambient;
	//return material.ambient;
	//return ambientLights[0].ambient.rgb;
}

float4 main(PSInput input) : SV_TARGET
{
	//float s = something[0].x;
	float2 thisTexCoord = input.Tex;
	//thisTexCoord = float2(1, 1);
	float3 texColor;
	texColor = screenTex.Sample(s, thisTexCoord).xyz;
	float3 finalColor;
	finalColor = applyLighting(texColor);

	float3 directionalColor = calcDirectional(input.Pos.xyz, input.Normal, material, 0, cbv.cameraPos);
	finalColor += directionalColor * texColor;
	finalColor = saturate(finalColor);
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
	alphaColor.rgb = finalColor;
	alphaColor.a = cbv.alpha;// +something[0].x;
	return alphaColor;
}

