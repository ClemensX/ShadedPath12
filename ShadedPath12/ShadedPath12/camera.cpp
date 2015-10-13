#include "stdafx.h"

Camera::Camera(World& w) : world(w) {
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
	projectionTransform();
	speed = 1.0f;
	pitch = yaw = 0.0f;
}


Camera::~Camera() {
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

World& Camera::getWorld() {
	return world;
}

void Camera::setWorld(World& w) {
	world = w;
}

#if defined(ENABLE_OVR2)
void Matrix4fToXM(XMFLOAT4X4 &xm, Matrix4f &m) {
	xm._11 = m.M[0][0];
	xm._12 = m.M[0][1];
	xm._13 = m.M[0][2];
	xm._14 = m.M[0][3];
	xm._21 = m.M[1][0];
	xm._22 = m.M[1][1];
	xm._23 = m.M[1][2];
	xm._24 = m.M[1][3];
	xm._31 = m.M[2][0];
	xm._32 = m.M[2][1];
	xm._33 = m.M[2][2];
	xm._34 = m.M[2][3];
	xm._41 = m.M[3][0];
	xm._42 = m.M[3][1];
	xm._43 = m.M[3][2];
	xm._44 = m.M[3][3];
}
#endif

void Camera::recalcOVR(XApp &xapp) {
#if defined(ENABLE_OVR2)
	// now do it the oculus way:
	// Get both eye poses simultaneously, with IPD offset already included. 
	ovrVector3f useHmdToEyeViewOffset[2] = { xapp.EyeRenderDesc[0].HmdToEyeViewOffset, xapp.EyeRenderDesc[1].HmdToEyeViewOffset };
	ovrPosef temp_EyeRenderPose[2];
	ovrHmd_GetEyePoses(xapp.hmd, 0, useHmdToEyeViewOffset, temp_EyeRenderPose, NULL);

	// Render the two undistorted eye views into their render buffers.  
	for (int eye = 0; eye < 2; eye++)
	//for (int eye = 1; eye >= 0; eye--)
		{
		ovrPosef    * useEyePose = &xapp.EyeRenderPose[eye];
		float       * useYaw = &xapp.YawAtRender[eye];
		float Yaw = XM_PI;
		*useEyePose = temp_EyeRenderPose[eye];
		*useYaw = Yaw;

		// Get view and projection matrices (note near Z to reduce eye strain)
		Matrix4f rollPitchYaw = Matrix4f::RotationY(Yaw);
		Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(useEyePose->Orientation);
		// fix finalRollPitchYaw for LH coordinate system:
		Matrix4f s = Matrix4f::Scaling(1.0f, -1.0f, -1.0f);  // 1 1 -1
		finalRollPitchYaw = s * finalRollPitchYaw * s;

		Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
		Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));//0 0 1
		Vector3f Posf;
		Posf.x = pos.x;
		Posf.y = pos.y;
		Posf.z = pos.z;
		//Vector3f shiftedEyePos = Posf + rollPitchYaw.Transform(useEyePose->Position);
		//Vector3f shiftedEyePos = Posf - rollPitchYaw.Transform(useEyePose->Position);
		Vector3f diff = rollPitchYaw.Transform(useEyePose->Position);
		Vector3f shiftedEyePos;
		shiftedEyePos.x = Posf.x - diff.x;
		shiftedEyePos.y = Posf.y + diff.y;
		shiftedEyePos.z = Posf.z + diff.z;
		look.x = finalForward.x;
		look.y = finalForward.y;
		look.z = finalForward.z;

		Matrix4f view = Matrix4f::LookAtLH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
		Matrix4f projO = ovrMatrix4f_Projection(xapp.EyeRenderDesc[eye].Fov, 0.2f, 2000.0f, false);
		Matrix4fToXM(this->viewOVR[eye], view.Transposed());
		Matrix4fToXM(this->projOVR[eye], projO.Transposed());
	}
#else
	// simple fallback if no OVR code included:
	for (int eye = 0; eye < 2; eye++) {
		this->viewOVR[eye] = view;
		this->projOVR[eye] = projection;
	}
#endif
}

void Camera::viewTransform() {
	XMMATRIX v = XMMatrixLookToLH(XMLoadFloat4(&pos), XMLoadFloat4(&look), XMLoadFloat4(&up));
	XMStoreFloat4x4(&view, v);
#if defined(ENABLE_OVR2)
	if (ovrCamera) {
		view = viewOVR[activeEye];
	}
#endif
}

void Camera::projectionTransform() {
	//float mNearWindowHeight = 2.0f * nearZ * tanf(0.5f*fieldOfViewAngleY);
	//float mFarWindowHeight = 2.0f * farZ * tanf(0.5f*fieldOfViewAngleY);
	// TODO check usefullness of plane equations above (from book)

	//Log("aspect == " << aspectRatio << "\n");
	XMMATRIX v = XMMatrixPerspectiveFovLH(fieldOfViewAngleY, aspectRatio, nearZ, farZ);
	//XMMATRIX v = XMMatrixPerspectiveFovLH(fieldOfViewAngleY, aspectRatio, mNearWindowHeight, mFarWindowHeight);
	XMStoreFloat4x4(&projection, v);
#if defined(ENABLE_OVR2)
	ovrFovPort fov;
	//fov.UpTan = 1.32928634f;
	//fov.DownTan = 1.32928634f;
	//fov.LeftTan = 1.05865765f;
	//fov.RightTan = 1.09236801f;
	//projection._31 *= -1.0f;
	//projection._32 *= -1.0f;
	//projection._33 *= -1.0f;
	//projection._34 *= -1.0f;
	if (ovrCamera) {
		Matrix4f projO = ovrMatrix4f_Projection((*world.getApp()).EyeRenderDesc[0].Fov, 0.2f, 1000.0f, false);
		Matrix4fToXM(projection, projO.Transposed());
		projection = projOVR[activeEye];
	}
#endif
}

XMMATRIX Camera::worldViewProjection() {
	XMMATRIX p = XMLoadFloat4x4(&projection);
	//look = pos;
	//look.z += 1000.0f;
	//up = xmfloat4(0.0f, 1.0f, 0.0f, 0.0f);
	//auto pt = pos;
	//Log("  Pos: " << pt.x << " " << pt.y << " " << pt.z);
	//auto pt2 = look;
	//Log("  Look: " << pt2.x << " " << pt2.y << " " << pt2.z << endl);
	XMMATRIX v = XMMatrixLookToLH(XMLoadFloat4(&pos), XMLoadFloat4(&look), XMLoadFloat4(&up));
#if defined(ENABLE_OVR2)
	if (ovrCamera) {
		//recalcOVR(*world.getApp());
		//p = XMLoadFloat4x4(&projOVR[activeEye]);
		//v = XMLoadFloat4x4(&viewOVR[activeEye]);
		//v = XMMatrixIdentity();
		//return XMMatrixTranspose(p);
		Vector3f eye;
		eye.x = this->pos.x;
		eye.y = this->pos.y;
		eye.z = this->pos.z;
		Vector3f at;
		at.x = eye.x + look.x;
		at.y = eye.y + look.y;
		at.z = eye.z + look.z;
		//Vector3f up = Vector3f(0, 1, 0);
		Vector3f upf;
		upf.x = up.x;
		upf.y = up.y;
		upf.z = up.z;
		Matrix4f view = Matrix4f::LookAtLH(eye, at, upf);
		XMFLOAT4X4 vxm;
		Matrix4fToXM(vxm, view);
		v = XMMatrixTranspose( XMLoadFloat4x4(&vxm));
		v = XMLoadFloat4x4(&viewOVR[activeEye]);
	}
#endif
	XMMATRIX wvp = v*p;
	//return wvp;
	//XMFLOAT4 t(0.0f, 0.0f, 0.0f, 1.0f);
	//XMVECTOR tv = XMLoadFloat4(&t);
	//tv = XMVector4Transform(tv, wvp);
	//XMStoreFloat4(&t, tv);
	//t.x /= t.w;
	//t.y /= t.w;
	//t.z /= t.w;
	//t.w = 1.0f;
	//Log("transformed " << t.x << " " << t.y << " " << t.z << " " << t.w << "\n");
	return XMMatrixTranspose(wvp);
	//return XMMatrixTranspose(v);
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

void Camera::walk(float dt) {
	// new position = position + dt*look
	dt *= speed;
	XMVECTOR s = XMVectorReplicate(dt);
	XMVECTOR l = XMLoadFloat4(&look);
	XMVECTOR p = XMLoadFloat4(&pos);
	XMStoreFloat4(&pos, XMVectorMultiplyAdd(s, l, p));
}

void Camera::strafe(float dt) {
	// new position = position + dt*right
	dt *= speed;
	XMVECTOR s = XMVectorReplicate(dt);
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

