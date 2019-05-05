#include "Post.hlsli"

[RootSignature(PostRS_1_0)]

float4 main(PSInput input) : SV_TARGET
{
	float4 col = screenTex.Sample(s, input.uv);
	//float4 col = float4(0.0f, 0.0f, 0.0f, 1.0f);
	return col;
}

