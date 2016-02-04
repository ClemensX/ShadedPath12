#include "Billboard.hlsli"

[RootSignature(BillboardRS)]

PSInput main( VSInput input )
{
	PSInput result;

	//result.position.w = 1.0;
	result.uv = input.uv;
	float4x4 wvp = cbv.wvp;
	float4 v;
	v.xyz = input.position.xyz;
	v.w = 1.0;
	result.position = mul(v, wvp);
	return result;
}