#include "Post.hlsli"

[RootSignature(PostRS)]

float4 main(PSInput input) : SV_TARGET
{
	float2 v = input.position.xy;
	return screenTex.Sample(s1, v);
}

