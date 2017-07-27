static const float4 g_XMOneHalf = { 0.5, 0.5, 0.5, 0.5 };
static const float4 Sign = { 1.0, -1.0, -1.0, 1.0 };

float vectorsEqual(float4 a, float4 b) {
	return length(a - b) < 0.00001;
}

float4 QuaternionRotationRollPitchYaw(float4 Angles) {
	float4 HalfAngles = Angles * g_XMOneHalf;
	//return HalfAngles;

	float4 SinAngles = sin(HalfAngles);
	float4 CosAngles = cos(HalfAngles);
	//return CosAngles;

	float4 s = SinAngles;
	float4 c = CosAngles;

	//XMVECTOR P0 = XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_1X, XM_PERMUTE_1X, XM_PERMUTE_1X>(SinAngles, CosAngles);
	float4 p0 = float4(s.x, c.x, c.x, c.x);
	//XMVECTOR Y0 = XMVectorPermute<XM_PERMUTE_1Y, XM_PERMUTE_0Y, XM_PERMUTE_1Y, XM_PERMUTE_1Y>(SinAngles, CosAngles);
	float4 y0 = float4(c.y, s.y, c.y, c.y);
	//XMVECTOR R0 = XMVectorPermute<XM_PERMUTE_1Z, XM_PERMUTE_1Z, XM_PERMUTE_0Z, XM_PERMUTE_1Z>(SinAngles, CosAngles);
	float4 r0 = float4(c.z, c.z, s.z, c.z);
	//XMVECTOR P1 = XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_1X, XM_PERMUTE_1X, XM_PERMUTE_1X>(CosAngles, SinAngles);
	float4 p1 = float4(c.x, s.x, s.x, s.x);
	//XMVECTOR Y1 = XMVectorPermute<XM_PERMUTE_1Y, XM_PERMUTE_0Y, XM_PERMUTE_1Y, XM_PERMUTE_1Y>(CosAngles, SinAngles);
	float4 y1 = float4(s.y, c.y, s.y, s.y);
	//XMVECTOR R1 = XMVectorPermute<XM_PERMUTE_1Z, XM_PERMUTE_1Z, XM_PERMUTE_0Z, XM_PERMUTE_1Z>(CosAngles, SinAngles);
	float4 r1 = float4(s.z, s.z, c.z, s.z);

	//XMVECTOR Q1 = XMVectorMultiply(P1, Sign.v);
	float4 q1 = p1 * Sign;
	//XMVECTOR Q0 = XMVectorMultiply(P0, Y0);
	float4 q0 = p0 * y0;
	//Q1 = XMVectorMultiply(Q1, Y1);
	q1 = q1 * y1;
	//Q0 = XMVectorMultiply(Q0, R0);
	q0 = q0 * r0;
	//XMVECTOR Q = XMVectorMultiplyAdd(Q1, R1, Q0);
	float4 q = q1 * r1 + q0;
	return q;
}
float4x4 calcToWorld(float4 p, float4 q) {

	float4 Angles = q;
	//// quaternion
	//XMVECTOR q = XMQuaternionIdentity();
	////XMVECTOR q = XMVectorSet(rot().x, rot().y, rot().z, 0.0f);
	//XMVECTOR q_origin = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	//XMFLOAT3 p = XMFLOAT3(0.0f, 0.0f, 0.0f);//rot();
	//										//WegDamit2.lock();
	//XMMATRIX rotateM = XMMatrixRotationRollPitchYaw(p.y, p.x, p.z);
	////WegDamit2.unlock();
	//q = XMQuaternionRotationMatrix(rotateM);
	//if (useQuaternionRotation) {
	//	q = XMLoadFloat4(&rot_quaternion);
	//}
	//q = XMQuaternionNormalize(q);
	//// scalar
	//XMVECTOR s = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	//// translation
	//XMVECTOR t = XMVectorSet(pos().x, pos().y, pos().z, 0.0f);
	//// toWorld matrix:
	//XMMATRIX r = XMMatrixTranspose(XMMatrixAffineTransformation(s, q_origin, q, t));
	//return r;
	////return XMMatrixTranspose(XMMatrixAffineTransformation(s, q_origin, q, t));
}

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

// named ObjectConstantBuffer for historic reasons, is UAV here, really
RWStructuredBuffer<ObjectConstantBuffer> cbvResult	: register(u0);	// UAV

struct CSConstantBuffer { // offset
	float4x4 vp;        //   0
};
CSConstantBuffer cbvCS: register(b0);

[RootSignature(ObjectCS)]

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float4x4 m = cbv.wvp;
	m[0][0] = 0.0;
	m[0][1] = 0.0;
	//float4x4 m2 = { 0, 0, 0, 0,
	//	0, 0, 0, 0,
	//	0, 0, 0, 0,
	//	0, 0, 0, 0 };
	float4x4 m2 = { 1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1 };
	//cbv.wvp = m;
	//cbvResult[0].wvp = m2;
	float4 rot_q; // rotation quaternion
	rot_q = float4(0, 0, 0, 1);
	rot_q = float4(1, 2, 3, 0);
	rot_q = QuaternionRotationRollPitchYaw(rot_q);
	float4 from_c_code = cbvCS.vp[0];
	// formulate codition to return --> see smoething
	//if (from_c_code.x == rot_q.x) return;
	//if (from_c_code.y == rot_q.y) return;
	if (vectorsEqual(from_c_code, rot_q)) return;
	//if (!from_c_code.x == rot_q.x) return;
	//if (rot_q.x < 0.0000000000000000000000000000000000001) return;
	for (uint tile = 0; tile < 500; tile++)
	{
		cbvResult[tile].wvp = m2;
		//cbvResult[tile].wvp = cbvCS.vp;
		cbvResult[tile].world = m2;
		cbvResult[tile].cameraPos = m2[0].xyz;
		cbvResult[tile].alpha = 120;
	}
}