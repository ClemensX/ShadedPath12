#include "Line.hlsli"

[RootSignature(LinesRS)]

PSInput main( VSInput input )
{
	PSInput result;

	result.position.w = 1.0;
	float4x4 wvp = cbv.wvp;
	float4 v;
	v.xyz = input.position;
	v.w = 1.0;
	result.position = mul(v, wvp);
	result.color = input.color;

	return result;
}