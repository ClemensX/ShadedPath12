#include "Line.hlsli"

float4 PSMain(PSInput input) : SV_TARGET
{
	return input.color;
}


[RootSignature(LinesRS)]

PSInput main( VSInput input )
{
	PSInput result;

	result.position = input.position;
	result.color = input.color;

	return result;
}