#include "Object.hlsli"

[RootSignature(ObjectRS)]

PSInput main( VSInput vin )
{
	PSInput vout;

	// Transform to homogeneous clip space.
	vout.Pos = mul(float4(vin.Pos, 1.0f), cbv.wvp);
	vout.PosW = mul(float4(vin.Pos, 1.0f), cbv.world).xyz; //vin.Pos.xyz;//
	vout.Normal = mul(vin.Normal, (float3x3)cbv.world);
	vout.Tex = vin.Tex;

	return vout;
}


