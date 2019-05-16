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
			D3D_FEATURE_LEVEL_11_0,
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

	UINT d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT| D3D11_CREATE_DEVICE_DEBUG;
	D2D1_FACTORY_OPTIONS d2dFactoryOptions = {};
	// Create an 11 device wrapped around the 12 device and share 12's command queue.
	ThrowIfFailed(D3D11On12CreateDevice(
		device.Get(),
		d3d11DeviceFlags,
		nullptr,
		0,
		reinterpret_cast<IUnknown * *>(commandQueue.GetAddressOf()),
		1,
		0,
		&device11,
		&deviceContext11,
		nullptr
	));

	// Query the 11On12 device from the 11 device.
	ThrowIfFailed(device11.As(&device11On12));

	// Create D2D/DWrite components.
	{
		D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
		ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, &d2dFactory));
		ComPtr<IDXGIDevice> dxgiDevice;
		ThrowIfFailed(device11On12.As(&dxgiDevice));
		ThrowIfFailed(d2dFactory->CreateDevice(dxgiDevice.Get(), &d2dDevice));
		ThrowIfFailed(d2dDevice->CreateDeviceContext(deviceOptions, &d2dDeviceContext));
		ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &dWriteFactory));
	}
}

void DXGlobal::initSwapChain(Pipeline* pipeline)
{
	auto pc = pipeline->getPipelineConfig();
	// Describe the swap chain.
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = (UINT) pc.getFrameBufferSize();
	swapChainDesc.BufferDesc.Width = pc.backbufferWidth;
	swapChainDesc.BufferDesc.Height = pc.backbufferHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = 0;// xapp->getHWND();
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ComPtr<IDXGISwapChain> swapChain0; // we cannot use create IDXGISwapChain3 directly - create IDXGISwapChain, then call As() to map to IDXGISwapChain3
	ThrowIfFailed(factory->CreateSwapChain(
		commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		&swapChainDesc,
		&swapChain0
	));

	ThrowIfFailed(swapChain0.As(&swapChain));
}