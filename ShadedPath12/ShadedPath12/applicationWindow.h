#pragma once

/*
* Application window is responsible for drawing to window frame
* swap chain is hold here, rest of engine is independent of app window
*/
class ApplicationWindow
{
public:
	void init(XApp *xapp, ComPtr<IDXGIFactory4> &factory);
	void present();
	UINT GetCurrentBackBufferIndex();
	AppWindowFrameResource * getCurrentFrameResource() { return &frameResources.at(GetCurrentBackBufferIndex()); }
	void destroy();
	ComPtr<ID3D12CommandQueue> commandQueue;
	static void task() {};
	ResourceStateHelper *resourceStateHelper = nullptr;
	ComPtr<IDXGISwapChain3> swapChain;
private:
	XApp * xapp = nullptr;
	DXManager *dxmanager = nullptr;
	static const UINT FrameCount = 3;
	vector<AppWindowFrameResource> frameResources;
};

