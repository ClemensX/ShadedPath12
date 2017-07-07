#include "lights_basic.hlsi"
#include "MObject.hlsli"

// root signature: CBV with MVP matrix, Descriptor Table with texture SRV
#define ObjectCS "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
            "UAV(u0, space = 0), " \
            "CBV(b0, space = 0), " \
            "CBV(b1, space = 0), " \
            "DescriptorTable(SRV(t0, space = 0)), " \
            "StaticSampler(s0, filter=FILTER_MIN_MAG_MIP_LINEAR, "\
			"addressU = TEXTURE_ADDRESS_WRAP, addressV = TEXTURE_ADDRESS_WRAP, addressW = TEXTURE_ADDRESS_CLAMP, " \
			"minLOD = 0, maxLOD = 0, mipLODBias = 0, " \
			"maxAnisotropy = 8, comparisonFunc = COMPARISON_LESS_EQUAL " \
			")"


RWStructuredBuffer<ObjectConstantBuffer> cbvResult	: register(u0);	// UAV

[RootSignature(ObjectCS)]

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float4x4 m = cbv.wvp;
	m[0][0] = 0.0;
	m[0][1] = 0.0;
	float4x4 m2 = { 0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0 };
	//float4x4 m2 = { 1, 0, 0, 0,
	//	0, 1, 0, 0,
	//	0, 0, 1, 0,
	//	0, 0, 0, 1 };
	//cbv.wvp = m;
	//cbvResult[0].wvp = m2;
	for (uint tile = 0; tile < 500; tile++)
	{
		cbvResult[tile].wvp = m2;
		cbvResult[tile].world = m2;
		cbvResult[tile].cameraPos = m2[0].xyz;
		cbvResult[tile].alpha = 0;
	}
}