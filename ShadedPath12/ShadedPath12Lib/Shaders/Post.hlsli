// shared data for Post VS and PS


// root signature: only has CBV that will hold the MVP matrix
#define PostRS_1_0 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
            "DescriptorTable(SRV(t0, space = 0)), " \
            "StaticSampler(s0, filter=FILTER_MIN_MAG_MIP_LINEAR, "\
			"addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_CLAMP, " \
			"minLOD = 0, maxLOD = 0, mipLODBias = 0, " \
			"maxAnisotropy = 1, comparisonFunc = COMPARISON_NEVER " \
			")"
#define PostRS_1_1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
            "DescriptorTable(SRV(t0, space = 0, flags=DATA_VOLATILE)), " \
            "StaticSampler(s0, filter=FILTER_MIN_MAG_MIP_LINEAR, "\
			"addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_CLAMP, " \
			"minLOD = 0, maxLOD = 0, mipLODBias = 0, " \
			"maxAnisotropy = 1, comparisonFunc = COMPARISON_NEVER " \
			")"
/*
samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
//samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
//samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
samplerDesc.MinLOD = -FLT_MAX;
samplerDesc.MaxLOD = FLT_MAX;
samplerDesc.MipLODBias = 0.0f;
samplerDesc.MaxAnisotropy = 1;
samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
samplerDesc.BorderColor[0] = 1.0f;
samplerDesc.BorderColor[1] = 1.0f;
samplerDesc.BorderColor[2] = 1.0f;
samplerDesc.BorderColor[3] = 1.0f;
ThrowIfFailed(xapp->device->CreateSamplerState(&samplerDesc, &samplerStateLinear));
samplerDesc.MinLOD = 0;
samplerDesc.MaxLOD = 0;
*/

Texture2D<float4> screenTex : register(t0);
SamplerState s : register(s0);

struct VSInput
{
	float3 position : POSITION;
	float2 uv : TEXCOORD;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};
