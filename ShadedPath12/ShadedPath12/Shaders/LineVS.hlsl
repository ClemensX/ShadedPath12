#include "Line.hlsli"

float4 PSMain(PSInput input) : SV_TARGET
{
	return input.color;
}


[RootSignature(LinesRS)]

PSInput main( VSInput input )
{
	PSInput result;

	result.position.w = 1.0;
	result.position.xyz = input.position;
	result.color = input.color;

	return result;
}