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

void DXGlobal::initFrameBufferResources(FrameDataGeneral *fd, FrameDataD2D* fd_d2d, int i, Pipeline* pipeline) {
	auto config = pipeline->getPipelineConfig();
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
	// d3d resources, first the render texture (unrelated to swap chain or window):
	// create depth/stencil buffer and render texture
	// Describe and create a depth stencil view (DSV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&fd->dsvHeapRenderTexture)));

	// depth stencil
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	// create the depth/stencil texture
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, config.backbufferWidth, config.backbufferHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0),
		IID_PPV_ARGS(&fd->depthStencilRenderTexture)
	);
	device->CreateDepthStencilView(fd->depthStencilRenderTexture.Get(), &depthStencilDesc, fd->dsvHeapRenderTexture->GetCPUDescriptorHandleForHeapStart());
	NAME_D3D12_OBJECT_SUFF(fd->depthStencilRenderTexture, i);

	// Describe and create a render target view (RTV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 1;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&fd->rtvHeapRenderTexture)));
	NAME_D3D12_OBJECT_SUFF(fd->rtvHeapRenderTexture, i);
	fd->rtvDescriptorSizeRenderTexture = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// create the render target texture
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, config.backbufferWidth, config.backbufferHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET /*| D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS /*| D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL*/),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, clearColor),
		IID_PPV_ARGS(&fd->renderTargetRenderTexture)
	);
	NAME_D3D12_OBJECT_SUFF(fd->renderTargetRenderTexture, i);

	resourceStateHelper->add(fd->renderTargetRenderTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	// create the render target view from the heap desc and render texture:
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(fd->rtvHeapRenderTexture->GetCPUDescriptorHandleForHeapStart());
	device->CreateRenderTargetView(fd->renderTargetRenderTexture.Get(), nullptr, rtvHandle);
	rtvHandle.Offset(1, fd->rtvDescriptorSizeRenderTexture);
	//dxmanager->createPSO(*effectFrameResource, frameIndex);
	//effectFrameResource->frameIndex = frameIndex;
	//xapp->workerThreadStates[frameIndex] = WorkerThreadState::InitFrame;
	// Create an empty root signature.
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&fd->rootSignatureRenderTexture)));
	}
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = fd->rootSignatureRenderTexture.Get();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
#include "CompiledShaders/PostVS.h"
	psoDesc.VS = { binShader_PostVS, sizeof(binShader_PostVS) };
	//psoDesc.VS = { nullptr, 0 };
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&fd->pipelineStateRenderTexture)));
	NAME_D3D12_OBJECT_SUFF(fd->pipelineStateRenderTexture, i);
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&fd->commandAllocatorRenderTexture)));
	NAME_D3D12_OBJECT_SUFF(fd->commandAllocatorRenderTexture, i);
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, fd->commandAllocatorRenderTexture.Get(), fd->pipelineStateRenderTexture.Get(), IID_PPV_ARGS(&fd->commandListRenderTexture)));
	NAME_D3D12_OBJECT_SUFF(fd->commandListRenderTexture, i);
	fd->commandListRenderTexture->Close();
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fd->fenceRenderTexture)));
	NAME_D3D12_OBJECT_SUFF(fd->fenceRenderTexture, i);
	fd->fenceValueRenderTexture = 0;
	fd->fenceEventRenderTexture = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (fd->fenceEventRenderTexture == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	// d3d resources, now the rendertarget associated with the swap chain and window:
	if (swapChain == nullptr) {
		Log("no swap chain - cannot initialize D3D12 resources")
			return;
	}
	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 1;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&fd->rtvHeap)));
		NAME_D3D12_OBJECT_SUFF(fd->rtvHeap, i);
		fd->rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(fd->rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&fd->renderTarget)));
		device->CreateRenderTargetView(fd->renderTarget.Get(), nullptr, rtvHandle);
		NAME_D3D12_OBJECT_SUFF(fd->renderTarget, i);
		rtvHandle.Offset(1, fd->rtvDescriptorSize);
		//ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[n])));
		resourceStateHelper->add(fd->renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT);


		// Describe and create a depth stencil view (DSV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&fd->dsvHeap)));
		// Create the depth stencil view for each frame
		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, config.backbufferWidth, config.backbufferHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&fd->depthStencil)
		));

		device->CreateDepthStencilView(fd->depthStencil.Get(), &depthStencilDesc, fd->dsvHeap->GetCPUDescriptorHandleForHeapStart());
		NAME_D3D12_OBJECT_SUFF(fd->depthStencil, i);
	}
	// Create an empty root signature.
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&fd->rootSignature)));
	}
	//D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	//{
	//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	//	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	//};
	// Describe and create the graphics pipeline state object (PSO).
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
//	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
//	psoDesc.pRootSignature = fd->rootSignature.Get();
//	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
//	psoDesc.DepthStencilState.DepthEnable = FALSE;
//	psoDesc.DepthStencilState.StencilEnable = FALSE;
//	psoDesc.SampleMask = UINT_MAX;
//	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//	psoDesc.NumRenderTargets = 1;
//	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
//	psoDesc.SampleDesc.Count = 1;
//#include "CompiledShaders/PostVS.h"
//	psoDesc.VS = { binShader_PostVS, sizeof(binShader_PostVS) };
	//psoDesc.VS = { nullptr, 0 };
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&fd->pipelineState)));
	NAME_D3D12_OBJECT_SUFF(fd->pipelineState, i);
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&fd->commandAllocator)));
	NAME_D3D12_OBJECT_SUFF(fd->commandAllocator, i);
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, fd->commandAllocator.Get(), fd->pipelineState.Get(), IID_PPV_ARGS(&fd->commandList)));
	NAME_D3D12_OBJECT_SUFF(fd->commandList, i);
	fd->commandList->Close();
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fd->fence)));
	NAME_D3D12_OBJECT_SUFF(fd->fence, i);
	fd->fenceValue = 0;
	fd->fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (fd->fenceEvent == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}

void DXGlobal::initSwapChain(Pipeline* pipeline, HWND hwnd)
{
	auto conf = pipeline->getPipelineConfig();
	// Describe the swap chain.
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = (UINT)conf.getFrameBufferSize();
	swapChainDesc.BufferDesc.Width = conf.backbufferWidth;
	swapChainDesc.BufferDesc.Height = conf.backbufferHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = hwnd;
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

void DXGlobal::createSyncPoint(FrameDataGeneral* f, ComPtr<ID3D12CommandQueue> queue)
{
	UINT64 threadFenceValue = InterlockedIncrement(&f->fenceValue);
	ThrowIfFailed(queue->Signal(f->fence.Get(), threadFenceValue));
	ThrowIfFailed(f->fence->SetEventOnCompletion(threadFenceValue, f->fenceEvent));
}

void DXGlobal::waitForSyncPoint(FrameDataGeneral* f)
{
	//	int frameIndex = xapp->getCurrentBackBufferIndex();
	UINT64 completed = f->fence->GetCompletedValue();
	//Log("ev start " << f.frameIndex << " " << completed << " " << f.fenceValue << endl);
	if (completed == -1) {
		Error(L"fence.GetCompletedValue breakdown");
	}
	if (completed > 100000) {
		//Log("ev MAX " << completed << " " << f.fenceValue << endl);
	}
	if (completed <= f->fenceValue)
	{
		WaitForSingleObject(f->fenceEvent, INFINITE);
	}
	else {
		//Log("ev " << completed << " " << f.fenceValue << endl);
	}
}

void DXGlobal::waitGPU(FrameDataGeneral* res, ComPtr<ID3D12CommandQueue> queue)
{
	DXGlobal::createSyncPoint(res, queue);
	DXGlobal::waitForSyncPoint(res);
}

void DXGlobal::destroy(Pipeline *pipeline)
{
	auto conf = pipeline->getPipelineConfig();
	for (int i = 0; i < conf.getFrameBufferSize(); i++) {
	AppFrameData* af = (AppFrameData*)pipeline->afManager.getAppDataForSlot(i);
		waitGPU(&af->fd_general, commandQueue);
	}
}

void DXGlobal::copyRenderTexture2Window(FrameDataGeneral* fd_renderTexture, FrameDataGeneral* fd_swapChain)
{
	// hwnd RT in state _COMMON or _PRESENT
	// texture in state _RENDER_TARGET
	// wait for last frame with this index to be finished:
	waitGPU(fd_swapChain, commandQueue);
	// prepare command
	ID3D12GraphicsCommandList* commandList = fd_swapChain->commandList.Get();
	ThrowIfFailed(fd_swapChain->commandAllocator->Reset());
	ThrowIfFailed(commandList->Reset(fd_swapChain->commandAllocator.Get(), fd_swapChain->pipelineState.Get()));

	resourceStateHelper->toState(fd_renderTexture->renderTargetRenderTexture.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, commandList);
	resourceStateHelper->toState(fd_swapChain->renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, commandList);
	commandList->CopyResource(fd_swapChain->renderTarget.Get(), fd_renderTexture->renderTargetRenderTexture.Get());
	resourceStateHelper->toState(fd_renderTexture->renderTargetRenderTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, commandList);
	resourceStateHelper->toState(fd_swapChain->renderTarget.Get(), D3D12_RESOURCE_STATE_COMMON, commandList);

	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { commandList };

	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void DXGlobal::clearRenderTexture(FrameDataGeneral* fd)
{
	// wait for last frame with this index to be finished:
	waitGPU(fd, commandQueue);
	// d3d12 present:
	ID3D12GraphicsCommandList* commandList = fd->commandListRenderTexture.Get();
	ThrowIfFailed(fd->commandAllocatorRenderTexture->Reset());
	ThrowIfFailed(commandList->Reset(fd->commandAllocatorRenderTexture.Get(), fd->pipelineStateRenderTexture.Get()));
	resourceStateHelper->toState(fd->renderTargetRenderTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, commandList);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(fd->rtvHeapRenderTexture->GetCPUDescriptorHandleForHeapStart(), 0, fd->rtvDescriptorSizeRenderTexture);
	//float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; will correctly produce warnings CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(fd->dsvHeapRenderTexture->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	resourceStateHelper->toState(fd->renderTargetRenderTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, commandList);
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { commandList };

	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}