#include "Post.hlsli"

[RootSignature(PostRS)]

float4 main(PSInput input) : SV_TARGET
{
	float2 v = input.position.xy;
	v.x = (v.x + 1.0) / 2.0;
	v.y = (v.y + 1.0) / 2.0;
	//v = float2(0.0, 0.0);
	float4 col = screenTex.Sample(s, input.uv);
	//float4 col = float4(0.0f, 0.0f, 0.0f, 1.0f);
	return col;
}

