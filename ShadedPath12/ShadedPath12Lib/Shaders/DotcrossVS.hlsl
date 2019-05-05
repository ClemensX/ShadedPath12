/*
* Simple passthrough vertex shader - all computation will be done in geometry shader
*
* Vertex Shader --> Geometry Shader --> Pixel Shader
* entrypoint 'main' and profile are set as parameters in VS 13
*/

#include "Dotcross.hlsli"

[RootSignature(DotcrossRS)]

PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput vertexShaderOutput;

	vertexShaderOutput.pos = float4(input.pos.x, input.pos.y, input.pos.z, input.pos.w);

	return vertexShaderOutput;
}
