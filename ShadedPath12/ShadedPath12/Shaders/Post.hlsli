// shared data for Post VS and PS


// root signature: only has CBV that will hold the MVP matrix
#define PostRS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
              "DescriptorTable(SRV(t0, space = 0)), " \
              "StaticSampler(s0, filter=FILTER_MIN_MAG_MIP_LINEAR)"


Texture2D<float4> screenTex : register(t0);
SamplerState s1;

struct VSInput
{
	float3 position : POSITION;
};

struct PSInput
{
	float4 position : SV_POSITION;
};
