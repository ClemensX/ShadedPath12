#include "stdafx.h"

void Camera::init() {
	// camera view
	pos = XMFLOAT4(0.0f, 0.0f, -2.0f, 0.0f);
	look = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);
	up = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
	right = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
	lookAt(pos, look, up);
	XMStoreFloat4x4(&view, XMMatrixIdentity());
	// frustum
	fieldOfViewAngleY = XM_PIDIV4;
	aspectRatio = 1920.0f / 1080.0f;
	nearZ = 0.1f;
	farZ = 1000.0f;
	//projectionTransform();
	speed = 1.0f;
	pitch = yaw = 0.0f;
}


Camera::~Camera() {
}

Camera::Camera() {
}

void Camera::lookAt(XMFLOAT4 posp, XMFLOAT4 targetp, XMFLOAT4 upp) {
	posp.w = targetp.w = upp.w = 0.0f;
	XMVECTOR p = XMLoadFloat4(&posp);
	XMVECTOR t = XMLoadFloat4(&targetp);
	XMVECTOR u = XMLoadFloat4(&upp);
	XMVECTOR l = XMVector3Normalize(t-p);
	XMVECTOR r = XMVector3Normalize(XMVector3Cross(u,l));
	u = XMVector3Cross(l, r);
	XMStoreFloat4(&pos, p);
	XMStoreFloat4(&look, l);
	XMStoreFloat4(&look_straight, l);
	XMStoreFloat4(&right, r);
	XMStoreFloat4(&right_straight, r);
	XMStoreFloat4(&up, u);
	XMStoreFloat4(&up_straight, u);
}

void Camera::viewTransform() {
	XMMATRIX v = XMMatrixLookToLH(XMLoadFloat4(&pos), XMLoadFloat4(&look), XMLoadFloat4(&up));
	XMStoreFloat4x4(&view, v);
	//if (ovrCamera) {
	//	if (eyeNumUse)
	//		view = xapp().vr.getOVRViewMatrixByIndex(eyeNum);
	//	else
	//		view = xapp().vr.getOVRViewMatrix();
	//}
}

void Camera::projectionTransform() {
	//Log("aspect == " << aspectRatio << "\n");
	XMMATRIX v = XMMatrixPerspectiveFovLH(fieldOfViewAngleY, aspectRatio, nearZ, farZ);
	XMStoreFloat4x4(&projection, v);
	//if (ovrCamera) {
	//	if (eyeNumUse)
	//		projection = xapp().vr.getOVRProjectionMatrixByIndex(eyeNum);
	//	else
	//		projection = xapp().vr.getOVRProjectionMatrix();
	//}
}

XMMATRIX Camera::worldViewProjection() {
	XMMATRIX p = XMLoadFloat4x4(&projection);
	//Log("  Pos: " << pos.x << " " << pos.y << " " << pos.z << endl);
	//auto pt2 = look;
	//if (pt2.x != 0.0f)
	//Log("  Look: " << pt2.x << " " << pt2.y << " " << pt2.z << endl);
	XMMATRIX v = XMMatrixLookToLH(XMLoadFloat4(&pos), XMLoadFloat4(&look), XMLoadFloat4(&up));
	//if (ovrCamera) {
	//	XMFLOAT4X4 vxm;
	//	if (eyeNumUse)
	//		vxm = xapp().vr.getOVRViewMatrixByIndex(eyeNum);
	//	else
	//		vxm = xapp().vr.getOVRViewMatrix();
	//	v = XMLoadFloat4x4(&vxm);
	//}
	XMMATRIX wvp = v*p;
	return XMMatrixTranspose(wvp);
}

// called in OVR mode only:
XMMATRIX Camera::worldViewProjection(XMFLOAT4X4 &proj_ovr, XMFLOAT4X4 &view_ovr) {
	if (!ovrCamera) return worldViewProjection();
	XMMATRIX p = XMLoadFloat4x4(&proj_ovr);
	XMMATRIX v = XMLoadFloat4x4(&view_ovr);
	XMMATRIX wvp = v*p;
	return XMMatrixTranspose(wvp);
}

int Camera::calculateVisibility(BoundingBox &box, XMMATRIX &toWorld) {
	//UpdateViewMatrix();
	XMMATRIX v = XMLoadFloat4x4(&view);
	XMVECTOR detView = XMMatrixDeterminant(v);
	XMMATRIX invView = XMMatrixInverse(&detView, v);

	XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(toWorld), toWorld);
	XMMATRIX toLocal = XMMatrixMultiply(invView, invWorld);
	XMVECTOR scale, rotQuat, trans;
	XMMatrixDecompose(&scale, &rotQuat, &trans, toLocal);
	BoundingFrustum mCamFrustum;
	XMMATRIX p = XMLoadFloat4x4(&projection);
	BoundingFrustum::CreateFromMatrix(mCamFrustum, p);
	BoundingFrustum localspaceFrustum;
	mCamFrustum.Transform(localspaceFrustum, XMVectorGetX(scale), rotQuat, trans);
	return localspaceFrustum.Contains(box);
}

void Camera::setSpeed(float s) {
	speed = s;
}

float Camera::getSpeed() {
	return speed;
}

void Camera::walk(double dt) {
	// new position = position + dt*look
	dt *= speed;
	XMVECTOR s = XMVectorReplicate((float)dt);
	XMVECTOR l = XMLoadFloat4(&look);
	XMVECTOR p = XMLoadFloat4(&pos);
	XMStoreFloat4(&pos, XMVectorMultiplyAdd(s, l, p));
}

void Camera::strafe(double dt) {
	// new position = position + dt*right
	dt *= speed;
	XMVECTOR s = XMVectorReplicate((float)dt);
	XMVECTOR r = XMLoadFloat4(&right);
	XMVECTOR p = XMLoadFloat4(&pos);
	XMStoreFloat4(&pos, XMVectorMultiplyAdd(s, r, p));
}

void Camera::apply_pitch(float pitch_absolute) {
	// rotate up and look vector around the right vector.

	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat4(&right), pitch_absolute);

	XMStoreFloat4(&up, XMVector3TransformNormal(XMLoadFloat4(&up_straight), R));
	XMStoreFloat4(&look, XMVector3TransformNormal(XMLoadFloat4(&look_straight), R));
}

void Camera::apply_yaw(float yaw_absolute) {
	// rotate right, up and look vector around y axis.

	XMMATRIX R = XMMatrixRotationY(yaw_absolute);

	XMStoreFloat4(&right, XMVector3TransformNormal(XMLoadFloat4(&right_straight), R));
	XMStoreFloat4(&up, XMVector3TransformNormal(XMLoadFloat4(&up_straight), R));
	XMStoreFloat4(&look, XMVector3TransformNormal(XMLoadFloat4(&look_straight), R));
}

void Camera::apply_pitch_yaw() {
	// rotate up and look vector around the right vector.
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat4(&right_straight), -pitch);
	XMStoreFloat4(&up, XMVector3TransformNormal(XMLoadFloat4(&up_straight), R));
	XMStoreFloat4(&look, XMVector3TransformNormal(XMLoadFloat4(&look_straight), R));

	// rotate right, up and look vector around y axis.
	R = XMMatrixRotationY(yaw);
	XMStoreFloat4(&right, XMVector3TransformNormal(XMLoadFloat4(&right_straight), R));
	XMStoreFloat4(&up, XMVector3TransformNormal(XMLoadFloat4(&up), R));
	XMStoreFloat4(&look, XMVector3TransformNormal(XMLoadFloat4(&look), R));
}

