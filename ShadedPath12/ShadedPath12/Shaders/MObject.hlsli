// shared data for Object VS and PS


// root signature: CBV with MVP matrix, Descriptor Table with texture SRV
#define ObjectRS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
            "CBV(b0, space = 0), " \
            "CBV(b1, space = 0), " \
            "DescriptorTable(SRV(t0, space = 0)), " \
            "StaticSampler(s0, filter=FILTER_MIN_MAG_MIP_LINEAR, "\
			"addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_CLAMP, " \
			"minLOD = 0, maxLOD = 0, mipLODBias = 0, " \
			"maxAnisotropy = 8, comparisonFunc = COMPARISON_LESS_EQUAL " \
			")"

Texture2D<float4> screenTex : register(t0);
SamplerState s : register(s0);

struct ObjectConstantBuffer {
	float4x4 wvp;
	float4x4 world;
	float3   cameraPos;
	float    alpha;
	Material material;
};

ConstantBuffer<ObjectConstantBuffer> cbv: register(b0);
// we cannot use array - it exceeds max allowed CBV size of 4096 16-byte entries
// set single CBV pointer to correct offset before calling shader code
//cbuffer cbFixedCBVs : register(b1) {
//	ObjectConstantBuffer cbvs[1000];
//};

struct VSInput
{
	float3 Pos    : POSITION;
	float3 Normal : NORMAL;
	float2 Tex    : TEXCOORD;
};

struct PSInput
{
	float4 Pos   : SV_POSITION;
	float3 PosW  : POSITION;
	float2 Tex   : TEXCOORD;
	float3 Normal: NORMAL;
};

#include "lights.hlsi"