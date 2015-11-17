#include "stdafx.h"

VR::VR(XApp *xapp) {
	this->xapp = xapp;
}


VR::~VR() {
#if defined(_OVR_)
	ovr_Destroy(session);
	ovr_Shutdown();
#endif
}

void VR::init()
{
#if defined(_OVR_)
	ovrResult result = ovr_Initialize(nullptr);
	if (OVR_FAILURE(result)) {
		Error(L"ERROR: LibOVR failed to initialize.");
		return;
	}
	Log("LibOVR initialized ok!" << endl);
	result = ovr_Create(&session, &luid);
	if (OVR_FAILURE(result))
	{
		ovr_Shutdown();
		Error(L"No Oculus Rift detected. Cannot run in OVR mode without Oculus Rift device.");
		return;
	}

	desc = ovr_GetHmdDesc(session);
	resolution = desc.Resolution;
	// Start the sensor which provides the Rift’s pose and motion.
	result = ovr_ConfigureTracking(session, ovrTrackingCap_Orientation |
		ovrTrackingCap_MagYawCorrection |
		ovrTrackingCap_Position, 0);
	if (OVR_FAILURE(result)) Error(L"Could not enable Oculus Rift Tracking. Cannot run in OVR mode without Oculus Rift tracking.");

	// Setup VR components, filling out description
	eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, desc.DefaultEyeFov[0]);
	eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, desc.DefaultEyeFov[1]);

	//nextTracking();
/*	ovrHmd_AttachToWindow(hmd, wininfo().getHWND(), NULL, NULL);
	*/
#endif
}

void VR::initFrame()
{
}

void VR::startFrame()
{
	curEye = EyeLeft;
}

void VR::endFrame()
{
}

void VR::prepareViews(D3D12_VIEWPORT &viewport, D3D12_RECT &scissorRect)
{
	if (!enabled) {
		viewports[EyeLeft] = viewport;
		scissorRects[EyeLeft] = scissorRect;
		viewports[EyeRight] = viewport;
		scissorRects[EyeRight] = scissorRect;
	} else {
		//orig_viewport = viewport;
		//orig_scissorRect = scissorRect;
		viewports[EyeLeft] = viewport;
		scissorRects[EyeLeft] = scissorRect;
		viewports[EyeRight] = viewport;
		scissorRects[EyeRight] = scissorRect;
		viewports[EyeLeft].Width /= 2;
		scissorRects[EyeLeft].right /= 2;
		//scissorRects[EyeLeft].right--;
		viewports[EyeRight].Width /= 2;
		viewports[EyeRight].TopLeftX = viewports[EyeLeft].Width;
		scissorRects[EyeRight].left = scissorRects[EyeLeft].right;
	}
}

void VR::prepareDraw()
{
	cam_pos = xapp->camera.pos;
	cam_look = xapp->camera.look;
	cam_up = xapp->camera.up;
	firstEye = true;
}

void VR::endDraw() {
	xapp->camera.pos = cam_pos;
	xapp->camera.look = cam_look;
	xapp->camera.up = cam_up;
	xapp->camera.worldViewProjection();
}

void VR::adjustEyeMatrix(XMMATRIX &m) {
	Camera c2 = xapp->camera;
	if (curEye == EyeLeft) {
		c2.pos.x = cam_pos.x - 0.5f;
		c2.pos.y = cam_pos.y + 0.3f;
	}
	else {
		c2.pos.x = cam_pos.x + 0.5f;
		c2.pos.y = cam_pos.y + 0.3f;
	}
	m = c2.worldViewProjection();
	//c2.apply_pitch_yaw();
	//if (curEye == EyeLeft) {
	//	xapp->camera.pos.x = cam_pos.x - 0.5f;
	//	xapp->camera.pos.y = cam_pos.x + 0.3f;
	//} else {
	//	xapp->camera.pos.x = cam_pos.x + 0.5f;
	//	xapp->camera.pos.y = cam_pos.x + 0.3f;
	//}
	//m = xapp->camera.worldViewProjection();
	//xapp->camera.apply_pitch_yaw();
}

void VR::nextEye() {
	if (curEye == EyeLeft) curEye = EyeRight;
	else curEye = EyeLeft;
	firstEye = false;
}

bool VR::isFirstEye() {
	return firstEye;
}

void VR::nextTracking()
{
	// Query the HMD for the current tracking state.
	ovrTrackingState ts = ovr_GetTrackingState(session, ovr_GetTimeInSeconds(), false);

	if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked))
	{
		ovrPoseStatef pose = ts.HeadPose;// .ThePose;

										 // fake ovr
										 //pose.ThePose.Orientation.x = 0.1f;
										 //pose.ThePose.Orientation.y = 0.1f;
										 //pose.ThePose.Orientation.z = 0.1f;

										 // try new method:
		XMFLOAT4 af = XMFLOAT4(1.0f, 2.0f, 0.0f, 0.0f);
		XMFLOAT4 bf = XMFLOAT4(4.5f, 1.5f, 0.0f, 0.0f);
		XMVECTOR a = XMLoadFloat4(&af);
		XMVECTOR b = XMLoadFloat4(&bf);
		//XMVECTOR proj = XMVector3ProjectOnVector(a, b);
		XMVECTOR proj = XMVector3ReflecttOnVector(a, b);
		XMFLOAT4 p;
		XMStoreFloat4(&p, proj);
		//Log(" vec " << p.x << " " << p.y << endl);

		//Log(" head pose" << pose.ThePose.Orientation.x << " " << pose.ThePose.Orientation.y << " " << pose.ThePose.Orientation.z << endl);
		//float yaw, eyePitch, eyeRoll;
		//pose.ThePose.Orientation.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &eyePitch, &eyeRoll);
		XMFLOAT4 lookQ = XMFLOAT4(pose.ThePose.Orientation.x, pose.ThePose.Orientation.y, pose.ThePose.Orientation.z, pose.ThePose.Orientation.w);
		XMVECTOR lookQV = XMLoadFloat4(&lookQ);
		//lookQV = XMQuaternionNormalize(lookQV);  //TODO do we need to normalize here?
		XMMATRIX lookM = XMMatrixRotationQuaternion(lookQV);
		// fix orientation of reversed coords:
		////lookM = XMMatrixScaling(1.0f, -1.0f, 1.0f) * lookM;
		lookM = (XMMatrixScaling(-1.0f, -1.0f, -1.0f) * lookM) * XMMatrixScaling(-1.0f, -1.0f, -1.0f);
		//lookM = XMMatrixTranspose(lookM);

		//const XMFLOAT3* camPos = &world.path.getPos(PATHID_CAM_INTRO, xapp->gametime.getRealTime(), 0);
		//camera.pos
		XMFLOAT4 pos(xapp->camera.pos.x, xapp->camera.pos.y, xapp->camera.pos.z, 0.0f);
		XMFLOAT4 target(0.0f, 0.0f, 100.0f, 1.0f);
		//XMFLOAT4 target(camera.pos.x, camera.pos.y, camera.pos.z + 100.0f, 1.0f);
		XMVECTOR targetV = XMLoadFloat4(&target);
		XMVECTOR axis = targetV;
		targetV = XMVector3Transform(targetV, lookM);
		targetV = XMVector3ReflecttOnVector(targetV, axis);
		XMFLOAT4 up(0.0f, 1.0f, 0.0f, 0.0f);
		XMFLOAT4 xmpos, xmtarget, xmup;
		xmpos = XMFLOAT4(pos);
		XMStoreFloat4(&xmtarget, targetV);
		// adjust lookat:
		xmtarget.x += xapp->camera.pos.x;
		xmtarget.y += xapp->camera.pos.y;
		xmtarget.z += xapp->camera.pos.z;

		//xmtarget.y *= -1.0f;
		//xmtarget.x *= -1.0f;
		// adjust up vector:
		XMVECTOR upPoseV = XMLoadFloat4(&lookQ);
		XMMATRIX upM = XMMatrixRotationQuaternion(lookQV);
		XMVECTOR upV = XMLoadFloat4(&up);
		axis = upV;
		upV = XMVector3Transform(upV, upM);
		upV = XMVector3ReflecttOnVector(upV, axis);
		XMStoreFloat4(&xmup, upV);
		//xmup.y *= -1.0f;

		xmup = XMFLOAT4(up);
		xapp->camera.lookAt(xmpos, xmtarget, xmup);
		xapp->camera.viewTransform();
		// now do it the oculus way:
		// Get both eye poses simultaneously, with IPD offset already included. 
		ovrVector3f useHmdToEyeViewOffset[2] = { eyeRenderDesc[0].HmdToEyeViewOffset, eyeRenderDesc[1].HmdToEyeViewOffset };
		ovrPosef temp_EyeRenderPose[2];
		ovr_CalcEyePoses(ts.HeadPose.ThePose, useHmdToEyeViewOffset, temp_EyeRenderPose);
		//ovrHmd_GetEyePoses(hmd, 0, useHmdToEyeViewOffset, temp_EyeRenderPose, &ts);

		ovrPosef    * useEyePose = &EyeRenderPose[0];
		float       * useYaw = &YawAtRender[0];
		float Yaw = XM_PI;
		*useEyePose = temp_EyeRenderPose[0];
		*useYaw = Yaw;

		// Get view and projection matrices (note near Z to reduce eye strain)
		Matrix4f rollPitchYaw = Matrix4f::RotationY(Yaw);
		Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(useEyePose->Orientation);
		Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
		Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
		Vector3f Pos;
		Pos.x = xapp->camera.pos.x;
		Pos.y = xapp->camera.pos.y;
		Pos.z = xapp->camera.pos.z;
		Vector3f shiftedEyePos = Pos + rollPitchYaw.Transform(useEyePose->Position);

		Matrix4f view = Matrix4f::LookAtLH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
		Matrix4f projO = ovrMatrix4f_Projection(eyeRenderDesc[0].Fov, 0.2f, 1000.0f, true);
		float f = projO.M[0][0];
		XMFLOAT4X4 xmProjO;
		xmProjO._11 = projO.M[0][0];
		xmProjO._12 = projO.M[0][1];
		xmProjO._13 = projO.M[0][2];
		xmProjO._14 = projO.M[0][3];
		xmProjO._21 = projO.M[1][0];
		xmProjO._22 = projO.M[1][1];
		xmProjO._23 = projO.M[1][2];
		xmProjO._24 = projO.M[1][3];
		xmProjO._31 = projO.M[2][0];
		xmProjO._32 = projO.M[2][1];
		xmProjO._33 = projO.M[2][2];
		xmProjO._34 = projO.M[2][3];
		xmProjO._41 = projO.M[3][0];
		xmProjO._42 = projO.M[3][1];
		xmProjO._43 = projO.M[3][2];
		xmProjO._44 = projO.M[3][3];
		XMMATRIX xmProjOM = XMLoadFloat4x4(&xmProjO);
		xapp->camera.projection = xmProjO;
	}
}
