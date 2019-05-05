/*
* Simple passthrough vertex shader - all computation will be done in geometry shader
*
* Vertex Shader --> Geometry Shader --> Pixel Shader
* entrypoint 'main' and profile are set as parameters in VS 13
*/

#include "Linetext.hlsli"

[RootSignature(LinetextRS)]

PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput vertexShaderOutput;

	// For this lesson, set the vertex depth value to 0.5 so it is guaranteed to be drawn.
	//vertexShaderOutput.pos = float4(input.pos.x, input.pos.y, 0.5f, 1.0f);
	vertexShaderOutput.pos = float4(input.pos.x, input.pos.y, input.pos.z, input.pos.w);
	vertexShaderOutput.info = input.info;

	return vertexShaderOutput;
}
