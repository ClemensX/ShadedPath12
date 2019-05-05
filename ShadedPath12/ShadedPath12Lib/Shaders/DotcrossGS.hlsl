
#include "Dotcross.hlsli"

[RootSignature(DotcrossRS)]

[maxvertexcount(6)]
void main(
	point float4 input[1] : SV_POSITION,
	inout LineStream< GSOutput > output
	)
{
	//len = linelen;
	float linelen = cbv.linelen;
	float4x4 wvp = cbv.wvp;
	float add = linelen / 2.0f;
	GSOutput el;
	float4 pos = input[0];
	pos.w = 1.0;
	el.pos = input[0];
	// transform to homogeneous clip space
	//el.pos = mul(el.pos, wvp);
	pos.x -= add;
	el.pos = mul(pos, wvp);
	output.Append(el);
	pos.x += linelen;
	el.pos = mul(pos, wvp);
	output.Append(el);
	pos.x -= add;
	pos.y -= add;
	output.RestartStrip();
	el.pos = mul(pos, wvp);
	output.Append(el);
	pos.y += linelen;
	el.pos = mul(pos, wvp);
	output.Append(el);
	pos.y -= add;
	pos.z -= add;
	output.RestartStrip();
	el.pos = mul(pos, wvp);
	output.Append(el);
	pos.z += linelen;
	el.pos = mul(pos, wvp);
	output.Append(el);
}
