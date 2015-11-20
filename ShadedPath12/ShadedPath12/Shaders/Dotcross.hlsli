/*
* Make a 3d cross along all axes out of a point (== produce 6 vertices for 3 lines)
* Input: size(== length of lines), List of points
* Output: three lines forming the cross
* (Color must be handled in different stages)
*
* Vertex Shader --> Geometry Shader --> Pixel Shader
*/

// shared data for Dotcross VS, GS and PS

// root signature: only has CBV that will hold the MVP matrix and linelength
#define DotcrossRS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
              "CBV(b0, space = 0) "

struct DotcrossConstantBuffer {
	float4x4 wvp;
	float linelen;
};

ConstantBuffer<DotcrossConstantBuffer> cbv: register(b0);

struct VertexShaderInput
{
	float4 pos : POSITION;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
};
