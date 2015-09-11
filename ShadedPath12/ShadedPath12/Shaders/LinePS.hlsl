#include "Line.hlsli"

[RootSignature(LinesRS)]

float4 main(PSInput input) : SV_TARGET
{
	return input.color;
}

