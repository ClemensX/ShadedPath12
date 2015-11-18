#pragma once

#if defined(_OVR_)
#include "../../../OculusSDK/LibOVR/Include/OVR_CAPI.h"
#include "../../../OculusSDK/LibOVR/Include/OVR_CAPI_D3D.h"
#include "../../../OculusSDK/LibOVR/Include/Extras/OVR_Math.h"
using namespace OVR;
#pragma comment(lib, "../../../OculusSDK/LibOVR/Lib/Windows/x64/Release/VS2015/LibOVR.lib")
#endif

enum EyePos { EyeLeft, EyeRight };

class XApp;

class VR {
public:
	VR(XApp *xapp);
	~VR();
	// basic OVR initialization, called at start of xapp.init()
	void init();
	// init d3d resources, needs d3d11 devices ready, called within xapp.init()
	void initD3D();
	void initFrame();
	void startFrame();
	void endFrame();
	EyePos getCurrentEye() { return curEye; };

	void prepareViews(D3D12_VIEWPORT &viewport, D3D12_RECT &scissorRect);
	D3D12_VIEWPORT *getViewport() { return &viewports[curEye]; };
	D3D12_RECT *getScissorRect() { return &scissorRects[curEye]; };
	// getHeight and getWidth should only be called after init()
	int getHeight() { return buffersize_height; };
	int getWidth() { return buffersize_width; };
	// prepare VR draw: save current camera pos/look/up, viewport and scissor rect
	void prepareDraw();
	// undo the chages to the camera made by prepareDraw
	void endDraw();
	// adjust the MVP matrix according to current eye position
	void adjustEyeMatrix(XMMATRIX &m);
	// move to 2nd eye
	void nextEye();
	// return if this run is for the first eye - some initializations are not needed for 2nd eye
	bool isFirstEye();
	// read HMD position and generate view parameters for both eyes
	void nextTracking();
	void submitFrame();
	// get view matrix for current eye
	XMFLOAT4X4 getOVRViewMatrix() { return viewOVR[curEye]; };
	// get projection matrix for current eye
	XMFLOAT4X4 getOVRProjectionMatrix() { return projOVR[curEye]; };

	bool enabled = false;  // default: VR is off, switch on by command line option -vr
protected:
	EyePos curEye = EyeLeft;
private:
	D3D12_VIEWPORT viewports[2];
	D3D12_RECT scissorRects[2];
	XApp* xapp;
	XMFLOAT4 cam_look, cam_up, cam_pos;
	bool firstEye = false;
	int buffersize_width = 0;
	int buffersize_height = 0;

#if defined(_OVR_)
	ovrHmdDesc desc;
	ovrSizei resolution;
	ovrSession session;
	ovrGraphicsLuid luid;
	ovrEyeRenderDesc eyeRenderDesc[2];
	ovrPosef         EyeRenderPose[2];     // Useful to remember where the rendered eye originated
	float            YawAtRender[2];       // Useful to remember where the rendered eye originated
	XMFLOAT4X4 viewOVR[2], projOVR[2];
	ovrSwapTextureSet *      pTextureSet = 0;
	ID3D11RenderTargetView * pTexRtv[3];
	ovrLayerEyeFov layer;

#endif
};
