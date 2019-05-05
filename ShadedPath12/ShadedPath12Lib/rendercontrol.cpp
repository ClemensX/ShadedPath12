#include "stdafx.h"

void RenderControl::init(XApp * xapp)
{
	resourceStateHelper = ResourceStateHelper::getResourceStateHelper();
}

ID3D12Resource * RenderControl::getNextFrame()
{
	return nullptr;
}
