#include "stdafx.h"

void ApplicationWindow::init(XApp *xapp, ComPtr<IDXGIFactory4> &factory) {
	assert(this->xapp == nullptr); // make sure we are only called once
	this->xapp = xapp;

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(xapp->device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));
	commandQueue->SetName(L"commandQueue_appWindow");

	// Describe the swap chain.
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = xapp->FrameCount;
	swapChainDesc.BufferDesc.Width = xapp->backbufferWidth;
	swapChainDesc.BufferDesc.Height = xapp->backbufferHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = xapp->getHWND();
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;

	ComPtr<IDXGISwapChain> swapChain0; // we cannot use create IDXGISwapChain3 directly - create IDXGISwapChain, then call As() to map to IDXGISwapChain3
	ThrowIfFailed(factory->CreateSwapChain(
		commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		&swapChainDesc,
		&swapChain0
	));

	ThrowIfFailed(swapChain0.As(&swapChain));
	//swapChain = nullptr;
}

void ApplicationWindow::present() {
	assert(xapp);
	assert(xapp->device.Get());
	if (xapp->isShutdownMode()) return;
	//Log("app window present()" << endl);
	//Log("xapp device " << xapp << " " << xapp->device.Get() << endl);
	//Log("xapp device" << xapp << endl);
	ThrowIfFailedWithDevice(swapChain->Present(0, 0), xapp->device.Get());
}

UINT ApplicationWindow::GetCurrentBackBufferIndex() {
	return swapChain->GetCurrentBackBufferIndex();
}

void ApplicationWindow::destroy() {
	//commandQueue.ReleaseAndGetAddressOf();
	//commandQueue.~ComPtr();
	//swapChain.~ComPtr();
}