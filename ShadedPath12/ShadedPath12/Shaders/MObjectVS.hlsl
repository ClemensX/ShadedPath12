#include "lights_basic.hlsi"
#include "MObject.hlsli"

// named ObjectConstantBuffer for historic reasons, is UAV here, really
RWStructuredBuffer<ObjectConstantBuffer> cbvResult	: register(u0);	// read-only UAV

[RootSignature(ObjectRS)]

PSInput main(VSInput vin)
{
	PSInput vout;

	// Transform to homogeneous clip space.
	//vin.Pos.x += vin.Id * 3.0;
	//vin.Pos.y += vin.Id * 3.0;
	//vin.Pos.z += vin.Id * 3.0;
	ObjectConstantBuffer zeroCBV = cbvResult[NonUniformResourceIndex(0)];
	uint start = floor(zeroCBV.cameraPos.x);
	//uint num = floor(zeroCBV.cameraPos.y); not needed
	uint id = vin.Id + start;
	//if (id > 0)
	//	vin.Pos.x = -10;
	//id = 1;
	//ObjectConstantBuffer c = cbv[NonUniformResourceIndex(id)];
	ObjectConstantBuffer c = cbvResult[NonUniformResourceIndex(id)];
	vout.Pos = mul(float4(vin.Pos, 1.0f), c.wvp);
	vout.PosW = mul(float4(vin.Pos, 1.0f), c.world).xyz; //vin.Pos.xyz;//
	vout.Normal = mul(vin.Normal, (float3x3)c.world);
	vout.Tex = vin.Tex;
	float f = id;
	vout.Id = asfloat(f + 0.5);
	return vout;
}


