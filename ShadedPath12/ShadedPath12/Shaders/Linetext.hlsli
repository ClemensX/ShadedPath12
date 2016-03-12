/*
* Draw 3D Text out of lines
*
* Vertex Shader --> Geometry Shader --> Pixel Shader
*/

// shared data for Dotcross VS, GS and PS

// root signature: only has CBV that will hold the MVP matrix and linelength
#define LinetextRS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
              "CBV(b0, space = 0), " \
              "RootConstants(num32BitConstants = 3, b3)" // avoid GRFXTool exception, use different number for each root signature

//cbuffer cbPerObject {
//	float4x4 wvp;
//};

struct VertexShaderInput
{
	//float4x4 rot : BINORMAL;
	float4 pos : POSITION;
	float4 info : NORMAL;
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float4 info: NORMAL;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
};

// effect variables
struct LinetextConstantBuffer {
	float4x4 wvp;
	float4x4 rot_xy;
	float4x4 rot_zy;
	float4x4 rot_yx;
	float4x4 rot_cs;
	float du;
	float dv;
};

ConstantBuffer<LinetextConstantBuffer> cbv: register(b0);

struct TextElement {
	float4 pos : SV_POSITION;
	float4 info : NORMAL;
	//int ch : DUDELHUPF; // letter to draw
	//int charPos : DUDELHUPF; // draw letter on character position charPos from start of text line
};
