#include "Object.hlsli"

[RootSignature(ObjectRS)]

PSInput main( VSInput vin )
{
	PSInput vout;

	// Transform to homogeneous clip space.
	vout.Pos = mul(float4(vin.Pos, 1.0f), cbv.wvp);
	vout.Tex = vin.Tex;

	return vout;
}

