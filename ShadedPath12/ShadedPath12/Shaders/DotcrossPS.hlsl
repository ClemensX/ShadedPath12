

#include "Dotcross.hlsli"

[RootSignature(DotcrossRS)]

float4 main(GSOutput input) : SV_TARGET
{
	// just return yellow
	return float4(1.0f, 1.0f, 0.0f, 1.0f);
}