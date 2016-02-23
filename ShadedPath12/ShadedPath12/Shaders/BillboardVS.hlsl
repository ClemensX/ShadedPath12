#include "Billboard.hlsli"

[RootSignature(BillboardRS)]

PSInput main( VSInput input )
{
	PSInput result;

	float3 pos = input.position.xyz;
	float factorx = input.normal.x * input.normal.z;
	float factory = input.normal.y * input.normal.w;

	float3 look = normalize(cbv.cam - pos);
	float3 up = float3(0, 1, 0);
	float3 right = normalize(cross(up, look));
	float3 up2 = normalize(cross(look, right));

	//right = float3(1, 0, 0);
	up2 = float3(0, 1, 0);
	float3 pos2 = pos + factorx * right + factory * up2;
	//input.position.xyz = pos2;

	//result.position.w = 1.0;
	//input.position.y -= 4.0;
	//input.position.z += 0.8 * input.normal.x;
	result.uv = input.uv;
	float4x4 wvp = cbv.wvp;
	float4 v;
	v.xyz = input.position.xyz;
	v.w = 1.0;
	result.position = mul(v, wvp);
	return result;
}