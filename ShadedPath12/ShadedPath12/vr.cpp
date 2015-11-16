#include "stdafx.h"

VR::VR(XApp *xapp) {
	this->xapp = xapp;
}


VR::~VR() {
}

void VR::init()
{
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
