#include "stdafx.h"


void DXGlobal::init()
{
#ifdef _DEBUG
	// Enable the D3D12 debug layer.
	{
		ComPtr<ID3D12Debug> debugController;
		ComPtr<ID3D12Debug1> debugController1;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			if (!config.disableDX12Debug) debugController->EnableDebugLayer();
			debugController->QueryInterface(IID_PPV_ARGS(&debugController1));
			if (debugController1) {
				debugController1->SetEnableGPUBasedValidation(true);
			}
			else {
				Log("WARNING: Could not enable GPU validation - ID3D12Debug1 controller not available" << endl);
			}
			HRESULT getAnalysis = DXGIGetDebugInterface1(0, __uuidof(pGraphicsAnalysis), reinterpret_cast<void**>(&pGraphicsAnalysis));
		}
		else {
			Log("WARNING: Could not get D3D12 debug interface - ID3D12Debug controller not available" << endl);
		}
	}
#endif

	UINT debugFlags = 0;
	if (!config.disableDX12Debug) {
		debugFlags = 0;
#ifdef _DEBUG
		debugFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	}

	ThrowIfFailed(CreateDXGIFactory2(debugFlags, IID_PPV_ARGS(&factory)));

	if (config.warp)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		// if this fails in debug run: enable win 10 dev mode
		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device)
		));
	}
	else {
		// if this fails in debug run: enable win 10 dev mode and/or disable d3d12 debug layer via command line parameter -disableDX12Debug 
		ThrowIfFailed(D3D12CreateDevice(
			nullptr,
			D3D_FEATURE_LEVEL_12_1,
			IID_PPV_ARGS(&device)
		));
	}

	// disable auto alt-enter fullscreen switch (does leave an unresponsive window during debug sessions)
	//ThrowIfFailed(factory->MakeWindowAssociation(getHWND(), DXGI_MWA_NO_ALT_ENTER));

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));
	commandQueue->SetName(L"commandQueue_dxGlobal");

}

void DXGlobal::initFrameBufferResources(FrameDataGeneral *fd, FrameDataD2D* fd_d2d) {
	UINT d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;
	D2D1_FACTORY_OPTIONS d2dFactoryOptions = {};
	d2dFactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
	// Create an 11 device wrapped around the 12 device and share 12's command queue.
	D3D_FEATURE_LEVEL fl[] = { D3D_FEATURE_LEVEL_12_1 };
	D3D_FEATURE_LEVEL retLevel;
	ThrowIfFailed(D3D11On12CreateDevice(
		device.Get(),
		d3d11DeviceFlags,
		fl,
		1,
		reinterpret_cast<IUnknown * *>(commandQueue.GetAddressOf()),
		1,
		0,
		&fd->device11,
		&fd->deviceContext11,
		&retLevel
	));
	// Query the 11On12 device from the 11 device.
	ThrowIfFailed(fd->device11.As(&fd->device11On12));

	// Create D2D/DWrite components.
	{
		D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
		ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, &fd_d2d->d2dFactory));
		ComPtr<IDXGIDevice> dxgiDevice;
		ThrowIfFailed(fd->device11On12.As(&dxgiDevice));
		ThrowIfFailed(fd_d2d->d2dFactory->CreateDevice(dxgiDevice.Get(), &fd_d2d->d2dDevice));
		ThrowIfFailed(fd_d2d->d2dDevice->CreateDeviceContext(deviceOptions, &fd_d2d->d2dDeviceContext));
		ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &fd_d2d->dWriteFactory));
	}
}

void DXGlobal::initSwapChain(Pipeline* pipeline, HWND hwnd)
{
}