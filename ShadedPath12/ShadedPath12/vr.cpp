#include "stdafx.h"

VR::VR(XApp *xapp) {
	this->xapp = xapp;
}


VR::~VR() {
}

void VR::init()
{
	enabled = xapp->ovrRendering;
}

void VR::initFrame()
{
}

void VR::adaptViews(D3D12_VIEWPORT &viewport, D3D12_RECT &scissorRect)
{
	if (true) {
		viewports[EyeLeft] = viewport;
		scissorRects[EyeLeft] = scissorRect;
		viewports[EyeRight] = viewport;
		scissorRects[EyeRight] = scissorRect;
	}
}
