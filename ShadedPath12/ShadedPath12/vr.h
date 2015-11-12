#pragma once

enum EyePos { EyeLeft, EyeRight };

class XApp;

class VR {
public:
	VR(XApp *xapp);
	~VR();
	void init();
	void initFrame();
	EyePos getCurrentEye() { return curEye; };
	void adaptViews(D3D12_VIEWPORT &viewport, D3D12_RECT &scissorRect);
	D3D12_VIEWPORT *getViewport() { return &viewports[curEye]; };
	D3D12_RECT *getScissorRect() { return &scissorRects[curEye]; };

	bool enabled = false;  // default: VR is off, switch on by command line option -vr
protected:
	EyePos curEye = EyeLeft;
private:
	D3D12_VIEWPORT viewports[2];
	D3D12_RECT scissorRects[2];
	XApp* xapp;
};
