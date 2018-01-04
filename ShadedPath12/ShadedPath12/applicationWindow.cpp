#include "stdafx.h"

void ApplicationWindow::init(XApp *xapp, ComPtr<IDXGIFactory4> &factory) {
	assert(this->xapp == nullptr); // make sure we are only called once
	this->xapp = xapp;
	this->dxmanager = &xapp->dxmanager;
	this->resourceStateHelper = ResourceStateHelper::getResourceStateHelper();
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
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ComPtr<IDXGISwapChain> swapChain0; // we cannot use create IDXGISwapChain3 directly - create IDXGISwapChain, then call As() to map to IDXGISwapChain3
	ThrowIfFailed(factory->CreateSwapChain(
		commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		&swapChainDesc,
		&swapChain0
	));

	ThrowIfFailed(swapChain0.As(&swapChain));
	dxmanager->createFrameResources(frameResources, FrameCount, swapChain);
	// run worker threads:
	//WorkerCommand c;
	//xapp->workerThreads.add_t(&Command::task, c, xapp);
	//xapp->workerThreads.join_all();
	WorkerCopyTextureCommand c;
	//xapp->workerCommands.push_back(c);
	//WorkerCommand &ref_c = xapp->workerCommands.at(0);
	xapp->workerThreads.add_t(&Command::task, c, xapp);
	xapp->workerThreads.add_t(&Command::task, c, xapp);
	xapp->workerThreads.add_t(&Command::task, c, xapp);
	//xapp->workerThreads.join_all();
}


void ApplicationWindow::present() {
	assert(xapp);
	assert(xapp->device.Get());
	UINT frameNum = GetCurrentBackBufferIndex();
	//Log("app window present()" << endl);
	//Log("xapp device " << xapp << " " << xapp->device.Get() << endl);
	//Log("xapp device" << xapp << endl);
	//Log("frame: " << xapp->getFramenum() << endl);
	long long framenum = xapp->getFramenum();
	if ((framenum % 300) == 0) {
		Log("fps " << xapp->fps << " frame " << framenum << endl);
	}
	AppWindowFrameResource &res = frameResources.at(frameNum);
	// work with effect:

	//
	//WorkerCopyTextureCommand wc;
	//wc.type = CommandType::WorkerCopyTexture;
	//wc.textureName = string("markings");
	//xapp->workerQueue.push(wc);
	// get source resource from texture:
	static bool done = false;
	static TextureInfo *HouseTex = nullptr;
	if (!done) {
		done = true;
		//xapp->textureStore.loadTexture(L"vac_09.dds", "markings"); wrong format
		xapp->textureStore.loadTexture(L"dirt6_markings.dds", "markings");
		HouseTex = xapp->textureStore.getTexture("markings");
	}
	resourceStateHelper->addOrKeep(res.renderTarget.Get(), D3D12_RESOURCE_STATE_COMMON);
	resourceStateHelper->addOrKeep(HouseTex->texSRV.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// wait until GPU has finished with previous commandList
	//Sleep(30);
	dxmanager->waitGPU(res, commandQueue);
	ID3D12GraphicsCommandList *commandList = res.commandList.Get();
	ThrowIfFailed(res.commandAllocator->Reset());
	ThrowIfFailed(commandList->Reset(res.commandAllocator.Get(), res.pipelineState.Get()));

	CD3DX12_TEXTURE_COPY_LOCATION src(HouseTex->texSRV.Get(), 0);
	CD3DX12_TEXTURE_COPY_LOCATION dest(res.renderTarget.Get(), 0);
	CD3DX12_BOX box(0, 0, 512, 512);

	//commandList->CopyResource(renderTargets[frameNum].Get(), HouseTex->texSRV.Get());
	resourceStateHelper->toState(res.renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, commandList);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(res.rtvHeap->GetCPUDescriptorHandleForHeapStart(), 0, res.rtvDescriptorSize);
	commandList->ClearRenderTargetView(rtvHandle, xapp->clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(res.dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	resourceStateHelper->toState(HouseTex->texSRV.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, commandList);
	resourceStateHelper->toState(res.renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, commandList);
	commandList->CopyTextureRegion(&dest, 0, 0, 0, &src, &box);
	resourceStateHelper->toState(HouseTex->texSRV.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, commandList);
	resourceStateHelper->toState(res.renderTarget.Get(), D3D12_RESOURCE_STATE_COMMON, commandList);
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { commandList };

	xapp->appWindow.commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	ThrowIfFailedWithDevice(swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING), xapp->device.Get());
}

UINT ApplicationWindow::GetCurrentBackBufferIndex() {
	return swapChain->GetCurrentBackBufferIndex();
}

void ApplicationWindow::destroy() {
	// wait until all frames have finished GPU usage
	dxmanager->destroy(frameResources, commandQueue);
}