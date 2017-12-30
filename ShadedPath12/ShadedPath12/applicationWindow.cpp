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

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(xapp->device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
		rtvHeap->SetName(L"rtvHeap_xapp");

		rtvDescriptorSize = xapp->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
			xapp->device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
			wstringstream s;
			s << L"renderTarget_xapp[" << n << "]";
			renderTargets[n]->SetName(s.str().c_str());
			rtvHandle.Offset(1, rtvDescriptorSize);
			//ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[n])));

			// Describe and create a depth stencil view (DSV) descriptor heap.
			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
			dsvHeapDesc.NumDescriptors = 1;
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(xapp->device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeaps[n])));
		}
		// Create the depth stencil view for each frame
		for (UINT n = 0; n < FrameCount; n++)
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
			depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
			depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

			D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
			depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
			depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
			depthOptimizedClearValue.DepthStencil.Stencil = 0;

			ThrowIfFailed(xapp->device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, xapp->backbufferWidth, xapp->backbufferHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&depthOptimizedClearValue,
				IID_PPV_ARGS(&depthStencils[n])
			));

			//NAME_D3D12_OBJECT(m_depthStencil);

			xapp->device->CreateDepthStencilView(depthStencils[n].Get(), &depthStencilDesc, dsvHeaps[n]->GetCPUDescriptorHandleForHeapStart());
			wstringstream s;
			s << L"depthStencil_xapp[" << n << "]";
			depthStencils[n]->SetName(s.str().c_str());
		}
	}
	// Create an empty root signature.
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(xapp->device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
	}
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	//ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
#include "CompiledShaders/PostVS.h"
	//#include "CompiledShaders/PostPS.h"
	// test shade library functions
	//{
	//D3DLoadModule() uses ID3D11Module
	//ComPtr<ID3DBlob> vShader;
	//ThrowIfFailed(D3DReadFileToBlob(L"", &vShader));
	psoDesc.VS = { binShader_PostVS, sizeof(binShader_PostVS) };
	ThrowIfFailed(xapp->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
	ThrowIfFailed(xapp->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
	ThrowIfFailed(xapp->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&commandList)));
	ThrowIfFailed(xapp->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&updateFrameData.fence)));
	updateFrameData.fence->SetName(L"fence_texture_update");
	updateFrameData.fenceValue = 0;
	updateFrameData.fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (updateFrameData.fenceEvent == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

}

void ApplicationWindow::present() {
	assert(xapp);
	assert(xapp->device.Get());
	if (xapp->isShutdownMode()) return;
	UINT frameNum = GetCurrentBackBufferIndex();
	//Log("app window present()" << endl);
	//Log("xapp device " << xapp << " " << xapp->device.Get() << endl);
	//Log("xapp device" << xapp << endl);
	//Log("frame: " << xapp->getFramenum() << endl);

	// get source resource from texture:
	static bool done = false;
	static TextureInfo *HouseTex = nullptr;
	if (!done) {
		done = true;
		//xapp->textureStore.loadTexture(L"vac_09.dds", "markings"); wrong format
		xapp->textureStore.loadTexture(L"dirt6_markings.dds", "markings");
		HouseTex = xapp->textureStore.getTexture("markings");
	}

	CD3DX12_TEXTURE_COPY_LOCATION src(HouseTex->texSRV.Get(), 0);
	CD3DX12_TEXTURE_COPY_LOCATION dest(renderTargets[frameNum].Get(), 0);
	CD3DX12_BOX box(0, 0, 512, 512);

	ID3D12GraphicsCommandList *commandList = this->commandList.Get();
	//commandList->CopyResource(renderTargets[frameNum].Get(), HouseTex->texSRV.Get());
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameNum, rtvDescriptorSize);
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameNum].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET));
	commandList->ClearRenderTargetView(rtvHandle, xapp->clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHeaps[frameNum]->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(HouseTex->texSRV.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameNum].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST));
	commandList->CopyTextureRegion(&dest, 0, 0, 0, &src, &box);
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(HouseTex->texSRV.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameNum].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON));
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { this->commandList.Get() };

	xapp->appWindow.commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	//Sleep(30);
	EffectBase::createSyncPoint(updateFrameData, xapp->appWindow.commandQueue);
	EffectBase::waitForSyncPoint(updateFrameData);
	ThrowIfFailed(commandAllocator->Reset());
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pipelineState.Get()));

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