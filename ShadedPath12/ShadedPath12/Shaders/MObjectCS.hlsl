static const float4 g_XMOneHalf = { 0.5, 0.5, 0.5, 0.5 };
static const float4 g_XMIdentityR3 = { 0.0, 0.0, 0.0, 1.0 };
static const float4 Sign = { 1.0, -1.0, -1.0, 1.0 };
static const float4 Constant1110 = { 1.0, 1.0, 1.0, 0.0 };

/*
float vectorsEqual(float4 a, float4 b) {
	return length(a - b) < 0.00001;
}

float matrixEqual(float4x4 a, float4x4 b) {
	float diff = length(a[0] - b[0]) + length(a[1] - b[1]) + length(a[2] - b[2]) + length(a[3] - b[3]);
	return diff < 0.0001;
}
*/

float4 QuaternionRotationRollPitchYaw(float4 Angles) {
	float4 HalfAngles = Angles * g_XMOneHalf; // component wise multiplication
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
	float4 q1 = p1 * Sign; // component wise mult
	//XMVECTOR Q0 = XMVectorMultiply(P0, Y0);
	float4 q0 = p0 * y0; // component wise mult
	//Q1 = XMVectorMultiply(Q1, Y1);
	q1 = q1 * y1; // component wise mult
	//Q0 = XMVectorMultiply(Q0, R0);
	q0 = q0 * r0; // component wise mult
	//XMVECTOR Q = XMVectorMultiplyAdd(Q1, R1, Q0);
	float4 q = q1 * r1 + q0; // component wise mult and add
	return q;
}

float4x4 MatrixScalingFromVector(float4 scale) {
	float4x4 s;
	s[0] = float4(scale.x, 0, 0, 0);
	s[1] = float4(0, scale.y, 0, 0);
	s[2] = float4(0, 0, scale.z, 0);
	s[3] = float4(0, 0, 0, 1);
	return s;
}

float4x4 MatrixRotationQuaternion(float4 Quaternion) {

	//static const XMVECTORF32 Constant1110 = { 1.0f, 1.0f, 1.0f, 0.0f };

	//XMVECTOR Q0 = XMVectorAdd(Quaternion, Quaternion);
	float4 q0 = Quaternion + Quaternion;
	//XMVECTOR Q1 = XMVectorMultiply(Quaternion, Q0);
	float4 q1 = Quaternion * q0;

	//XMVECTOR V0 = XMVectorPermute<XM_PERMUTE_0Y, XM_PERMUTE_0X, XM_PERMUTE_0X, XM_PERMUTE_1W>(Q1, Constant1110.v);
	float4 v0 = float4(q1.y, q1.x, q1.x, 0);
	//XMVECTOR V1 = XMVectorPermute<XM_PERMUTE_0Z, XM_PERMUTE_0Z, XM_PERMUTE_0Y, XM_PERMUTE_1W>(Q1, Constant1110.v);
	float4 v1 = float4(q1.z, q1.z, q1.y, 0);
	//XMVECTOR R0 = XMVectorSubtract(Constant1110, V0);
	float4 r0 = Constant1110 - v0;
	//R0 = XMVectorSubtract(R0, V1);
	r0 = r0 - v1;

	//V0 = XMVectorSwizzle<XM_SWIZZLE_X, XM_SWIZZLE_X, XM_SWIZZLE_Y, XM_SWIZZLE_W>(Quaternion);
	v0 = float4(Quaternion.x, Quaternion.x, Quaternion.y, Quaternion.w);
	//V1 = XMVectorSwizzle<XM_SWIZZLE_Z, XM_SWIZZLE_Y, XM_SWIZZLE_Z, XM_SWIZZLE_W>(Q0);
	v1 = float4(q0.z, q0.y, q0.z, q0.w);
	//V0 = XMVectorMultiply(V0, V1);
	v0 = v0 * v1; // component wise mult

	//V1 = XMVectorSplatW(Quaternion);
	v1 = float4(Quaternion.w, Quaternion.w, Quaternion.w, Quaternion.w);
	//XMVECTOR V2 = XMVectorSwizzle<XM_SWIZZLE_Y, XM_SWIZZLE_Z, XM_SWIZZLE_X, XM_SWIZZLE_W>(Q0);
	float4 v2 = float4(q0.y, q0.z, q0.x, q0.w);
	//V1 = XMVectorMultiply(V1, V2);
	v1 = v1 * v2; // component wise mult

	//XMVECTOR R1 = XMVectorAdd(V0, V1);
	float4 r1 = v0 + v1;
	//XMVECTOR R2 = XMVectorSubtract(V0, V1);
	float4 r2 = v0 - v1;

	//V0 = XMVectorPermute<XM_PERMUTE_0Y, XM_PERMUTE_1X, XM_PERMUTE_1Y, XM_PERMUTE_0Z>(R1, R2);
	v0 = float4(r1.y, r2.x, r2.y, r1.z);
	//V1 = XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_1Z, XM_PERMUTE_0X, XM_PERMUTE_1Z>(R1, R2);
	v1 = float4(r1.x, r2.z, r2.x, r2.z);

	//XMMATRIX M;
	float4x4 m;
	//M.r[0] = XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_1X, XM_PERMUTE_1Y, XM_PERMUTE_0W>(R0, V0);
	m[0] = float4(r0.x, v0.x, v0.y, r0.w);
	//M.r[1] = XMVectorPermute<XM_PERMUTE_1Z, XM_PERMUTE_0Y, XM_PERMUTE_1W, XM_PERMUTE_0W>(R0, V0);
	m[1] = float4(v0.z, r0.y, v0.w, r0.w);
	//M.r[2] = XMVectorPermute<XM_PERMUTE_1X, XM_PERMUTE_1Y, XM_PERMUTE_0Z, XM_PERMUTE_0W>(R0, V1);
	m[2] = float4(v1.x, v1.y, r0.z, r0.w);
	//M.r[3] = g_XMIdentityR3.v;
	m[3] = g_XMIdentityR3;
	return m;
	//return M;
}

float4x4 MatrixAffineTransformation(float4 Scaling, float4 RotationOrigin, float4 RotationQuaternion, float4 Translation) {

	//XMMATRIX MScaling = XMMatrixScalingFromVector(Scaling);
	float4x4 MScaling = MatrixScalingFromVector(Scaling);
	//XMVECTOR VRotationOrigin = XMVectorSelect(g_XMSelect1110.v, RotationOrigin, g_XMSelect1110.v);
	float4 VRotationOrigin = float4(RotationOrigin.xyz, 0);
	//XMMATRIX MRotation = XMMatrixRotationQuaternion(RotationQuaternion);
	float4x4 MRotation = MatrixRotationQuaternion(RotationQuaternion);
	//XMVECTOR VTranslation = XMVectorSelect(g_XMSelect1110.v, Translation, g_XMSelect1110.v);
	float4 VTranslation = float4(Translation.xyz, 0);

	//XMMATRIX M;
	float4x4 m;
	//M = MScaling;
	m = MScaling;
	//M.r[3] = XMVectorSubtract(M.r[3], VRotationOrigin);
	m[3] = m[3] - VRotationOrigin;
	//M = XMMatrixMultiply(M, MRotation);
	m = mul(MRotation, m);  // change order for HLSL matrix mult
	//M.r[3] = XMVectorAdd(M.r[3], VRotationOrigin);
	m[3] = m[3] + VRotationOrigin;
	//M.r[3] = XMVectorAdd(M.r[3], VTranslation);
	m[3] = m[3] + VTranslation;
	//return M;
	return m;
}

float4x4 calcToWorld(float4 pos, float4 rot) {

/*	float4 q_origin = float4(0,0,0,0);
	q = q_origin;
	float4 rot_roll_pitch_yaw = float4(q.y, q.x, q.z, 0);
	q = QuaternionRotationRollPitchYaw(rot_roll_pitch_yaw);
	q = normalize(q);
	// scalar
	float4 s = float4(1, 1, 1, 1);
	// translation
	float4 t = float4(p.x, p.y, p.z, 0);
	*/
	//// original code
	//// quaternion
	//XMVECTOR q = XMQuaternionIdentity();
	//float4 q = g_XMIdentityR3;
	//XMVECTOR q_origin = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	float4 q_origin = float4(0, 0, 0, 0);
	//XMFLOAT3 p = XMFLOAT3(0.0f, 0.0f, 0.0f);//rot();
	float4 p = float4(0, 0, 0, 0);//rot(); initially no rotation specified
	//XMMATRIX rotateM = XMMatrixRotationRollPitchYaw(p.y, p.x, p.z);
	//q = XMQuaternionRotationMatrix(rotateM);
	float4 rot_roll_pitch_yaw = float4(p.y, p.x, p.z, 0);
    float4 q = QuaternionRotationRollPitchYaw(rot_roll_pitch_yaw);
	//if (useQuaternionRotation) {
	//	q = XMLoadFloat4(&rot_quaternion);
	//}
	//q = XMQuaternionNormalize(q);
	q = normalize(q);
	//// scalar
	//XMVECTOR s = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	float4 s = float4(1, 1, 1, 1);
	//// translation
	//XMVECTOR t = XMVectorSet(pos().x, pos().y, pos().z, 0.0f);
	float4 t = float4(pos.x, pos.y, pos.z, 0);
	//// toWorld matrix:
	//XMMATRIX r = XMMatrixTranspose(XMMatrixAffineTransformation(s, q_origin, q, t));
	float4x4 r = MatrixAffineTransformation(s, q_origin, q, t);
	return r;
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

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	//for (uint tile = 0; tile < 50000; tile++)
	//{
		uint tile = DTid.x;
		float4 rot = float4(0, 0, 0, 0);
		float4 pos;
		pos.x = cbvResult[tile].cameraPos.x;
		pos.y = cbvResult[tile].cameraPos.y;
		pos.z = cbvResult[tile].cameraPos.z;
		float4x4 toWorld = calcToWorld(pos, rot);
		float4x4 wvp = mul(toWorld, cbvCS.vp); // change order for matrix mult
		cbvResult[tile].wvp = wvp;
	//}
}