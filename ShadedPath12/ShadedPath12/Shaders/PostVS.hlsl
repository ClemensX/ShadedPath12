#include "Post.hlsli"

[RootSignature(PostRS)]

PSInput main( VSInput input )
{
	PSInput result;

	result.position.w = 1.0;
	result.position.xyz = input.position.xyz;
	result.uv = input.uv;
	return result;
}