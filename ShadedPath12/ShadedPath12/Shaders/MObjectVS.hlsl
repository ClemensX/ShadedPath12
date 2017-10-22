#include "lights_basic.hlsi"
#include "MObject.hlsli"

// named ObjectConstantBuffer for historic reasons, is UAV here, really
RWStructuredBuffer<ObjectConstantBuffer> cbvResult	: register(u0);	// read-only UAV

[RootSignature(ObjectRS)]

PSInput main(VSInput vin)
{
	PSInput vout;

	uint id = vin.Id + rootConstants.start;
	ObjectConstantBuffer c = cbvResult[NonUniformResourceIndex(id)];
	vout.Pos = mul(float4(vin.Pos, 1.0f), c.wvp);
	vout.PosW = mul(float4(vin.Pos, 1.0f), c.world).xyz; //vin.Pos.xyz;//
	vout.Normal = mul(vin.Normal, (float3x3)c.world);
	vout.Tex = vin.Tex;
	float f = id;
	vout.Id = asfloat(f + 0.5);
	return vout;
}


