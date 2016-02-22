// shared data for Billboard VS and PS


// root signature: CBV with MVP matrix, Descriptor Table with texture SRV
#define BillboardRS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
            "CBV(b0, space = 0), " \
            "DescriptorTable(SRV(t0, space = 0)), " \
            "StaticSampler(s0, filter=FILTER_ANISOTROPIC, "\
			"addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_CLAMP, " \
			"minLOD = 0, maxLOD = 0, mipLODBias = 0, " \
			"maxAnisotropy = 8, comparisonFunc = COMPARISON_LESS_EQUAL " \
			")"

Texture2D<float4> screenTex : register(t0);
SamplerState s : register(s0);

struct BillboardConstantBuffer {
	float4x4 wvp;
	float3   cam; //camera world position
};
ConstantBuffer<BillboardConstantBuffer> cbv: register(b0);


struct VSInput
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};
