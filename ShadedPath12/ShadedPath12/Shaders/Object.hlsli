// shared data for Object VS and PS


// root signature: CBV with MVP matrix, Descriptor Table with texture SRV
#define ObjectRS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
            "CBV(b0, space = 0), " \
            "CBV(b1, space = 0), " \
            "DescriptorTable(SRV(t0, space = 0)), " \
            "StaticSampler(s0, filter=FILTER_ANISOTROPIC, "\
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
};

ConstantBuffer<ObjectConstantBuffer> cbv: register(b0);

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