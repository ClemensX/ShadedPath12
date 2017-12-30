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
	void destroy();
	ComPtr<ID3D12CommandQueue> commandQueue;
private:
	XApp * xapp = nullptr;
	DXManager *dxmanager = nullptr;
	static const UINT FrameCount = 3;
	ComPtr<IDXGISwapChain3> swapChain;
	vector<AppWindowFrameResource> frameResources;
};

