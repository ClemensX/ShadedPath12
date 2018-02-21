#include "stdafx.h"

// create manager for number of frames
void DXManager::init(XApp *a, int frameIndexes) {
	assert(FrameCount == frameIndexes);
	this->frameCount = frameIndexes;
	xapp = a;
	device = xapp->device.Get();
};

void DXManager::createConstantBuffer(UINT maxThreads, UINT maxObjects, size_t singleObjectSize, wchar_t * name) {
	assert(maxObjects > 0);
	this->maxObjects = maxObjects;
	// check that constant buffer data fills 16 byte slots:
	assert(singleObjectSize % 16 == 0);
	//slotSize = (UINT)singleObjectSize;
	slotSize = calcConstantBufferSize((UINT)singleObjectSize); // we need to align because we reset constant buffer start with each call
	// allocate const buffer for all frames and possibly OVR:
	totalSize = slotSize * maxObjects;
	totalSize = calcConstantBufferSize((UINT)totalSize);
	assert(totalSize == calcConstantBufferSize((UINT)totalSize));
	if (xapp->ovrRendering) totalSize *= 2; // TODO: really needed?
	for (unsigned int i = 0; i < this->frameCount * maxThreads; i++) {
		ComPtr<ID3D12Resource> t;
		singleCBVResources.push_back(move(t));
		//Log("GPU virtual: " <<  cbvResource->GetGPUVirtualAddress(); << endl);
		//ThrowIfFailed(singleCBVResources[i]->Map(0, nullptr, reinterpret_cast<void**>(&singleCBV_GPUDests[i])));
		//void *mem = new BYTE[totalSize];
		//singleMemResources.push_back(mem);
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE, // do not set - dx12 does this automatically depending on resource type
			&CD3DX12_RESOURCE_DESC::Buffer(totalSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			D3D12_RESOURCE_STATE_COPY_DEST,
			//D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
			nullptr,
			IID_PPV_ARGS(&singleCBVResources[i])));
		wstring mod_name = wstring(name).append(L"_").append(to_wstring(i));
		singleCBVResources[i].Get()->SetName(mod_name.c_str());
		ID3D12Resource * resource = singleCBVResources[i].Get();
		resourceStateHelper->add(resource, D3D12_RESOURCE_STATE_COPY_DEST);
		//wstring gpuRwName = wstring(name).append(L"GPU_RW");
		//singleCBVResourcesGPU_RW[i].Get()->SetName(gpuRwName.c_str());
		//Log("GPU virtual: " <<  cbvResource->GetGPUVirtualAddress(); << endl);
		//ThrowIfFailed(singleCBVResources[i]->Map(0, nullptr, reinterpret_cast<void**>(&singleCBV_GPUDests[i])));
	}
	Log("slot size: " << slotSize << endl);
	Log("max objects: " << maxObjects << endl);
	Log("total size (per frame): " << totalSize << endl);
	Log("object size: " << singleObjectSize << endl);
	assert(slotSize == singleObjectSize);
};

D3D12_GPU_VIRTUAL_ADDRESS DXManager::getConstantBufferSetVirtualAddress(UINT setNum, int eyeNum)
{
	// frame related base address
	D3D12_GPU_VIRTUAL_ADDRESS dest = cbvSetResources[setSize * setNum + currentFrameIndex]->GetGPUVirtualAddress();
	UINT64 plus = 0; // getOffsetInConstantBuffer(objectIndex, eyeNum);
	return dest + plus;
}

void DXManager::uploadConstantBufferSet(UINT setNum, size_t singleObjectSize, void *mem_source)
{
	// frame related base address
	UINT8 * dest = cbvSetGPUDest[setSize * setNum + currentFrameIndex];
	memcpy(dest, mem_source, singleObjectSize);
}

void DXManager::createConstantBufferSet(UINT setNum, UINT maxThreads, UINT maxObjects, size_t singleObjectSize, wchar_t * name) {
	assert(maxObjects == 1); // we only handle constant buffer for one element currently
	// check that constant buffer data fills 16 byte slots:
	assert(singleObjectSize % 16 == 0);
	UINT slotSize = calcConstantBufferSize((UINT)singleObjectSize);	// we need to align because we reset constant buffer start with each call
		        													// allocate const buffer for all frames and possibly OVR:
	UINT totalSize = slotSize * maxObjects;
	totalSize = calcConstantBufferSize((UINT)totalSize);
	assert(totalSize == calcConstantBufferSize((UINT)totalSize));
	if (xapp->ovrRendering) totalSize *= 2; // TODO: really needed?
	setSize = this->frameCount * maxThreads;
	for (unsigned int i = setSize * setNum + 0; i < setSize * (setNum+1); i++) {
		ComPtr<ID3D12Resource> t;
		cbvSetResources.push_back(move(t));
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE, // do not set - dx12 does this automatically depending on resource type
			&CD3DX12_RESOURCE_DESC::Buffer(totalSize),
			//D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&cbvSetResources[i])));
		wstring mod_name = wstring(name).append(L"_").append(to_wstring(i));
		cbvSetResources[i].Get()->SetName(mod_name.c_str());
		ID3D12Resource * resource = cbvSetResources[i].Get();
		//resourceStateHelper->add(resource, D3D12_RESOURCE_STATE_COPY_DEST);
		resourceStateHelper->add(resource, D3D12_RESOURCE_STATE_GENERIC_READ);
		UINT8* gpuDest;
		ThrowIfFailed(resource->Map(0, nullptr, reinterpret_cast<void**>(&gpuDest)));
		cbvSetGPUDest.push_back(gpuDest);
		//wstring gpuRwName = wstring(name).append(L"GPU_RW");
		//singleCBVResourcesGPU_RW[i].Get()->SetName(gpuRwName.c_str());
		//Log("GPU virtual: " <<  cbvResource->GetGPUVirtualAddress(); << endl);
		//ThrowIfFailed(singleCBVResources[i]->Map(0, nullptr, reinterpret_cast<void**>(&singleCBV_GPUDests[i])));
	}
	//Log("slot size: " << slotSize << endl);
	//Log("max objects: " << maxObjects << endl);
	//Log("total size (per frame): " << totalSize << endl);
	//Log("object size: " << singleObjectSize << endl);
	//assert(slotSize == singleObjectSize);
};


void DXManager::createUploadBuffers()
{
	const UINT constantBufferSize = totalSize * FrameCount;
	assert(calcConstantBufferSize(constantBufferSize) == constantBufferSize);

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constantBufferUpload)
	));

	NAME_D3D12_OBJECT(constantBufferUpload);

	CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
	ThrowIfFailed(constantBufferUpload->Map(0, &readRange, reinterpret_cast<void**>(&constantBufferUploadCPU)));
	ZeroMemory(constantBufferUploadCPU, constantBufferSize);
}

void DXManager::createGraphicsExecutionEnv(ID3D12PipelineState *ps)
{
	assert(device != nullptr);
	graphics_ps = ps;
	for (int n = 0; n < this->frameCount; n++) {
		// Create compute resources.
		D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE };
		ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueues[n])));
		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[n])));
		ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[n].Get(), ps, IID_PPV_ARGS(&commandLists[n])));
		// Command lists are created in the recording state, but there is nothing
		// to record yet. The main loop expects it to be closed, so close it now.
		ThrowIfFailed(commandLists[n]->Close());
	}
}

UINT64 DXManager::getOffsetInConstantBuffer(UINT objectIndex, int eyeNum)
{
	UINT64 plus = slotSize * objectIndex;
	if (xapp->ovrRendering) {
		plus *= 2; // adjust for two eyes
	}
	if (xapp->ovrRendering && eyeNum == 1) {
		plus += slotSize;
	}
	return  plus;
}

void DXManager::upload(UINT objectIndex, int eyeNum, void * mem_source)
{
	// frame related base address
	UINT8 * dest = constantBufferUploadCPU + currentFrameIndex * totalSize;
	// add individual object/eye
	dest += getOffsetInConstantBuffer(objectIndex, eyeNum);
	memcpy(dest, mem_source, slotSize);
	objectStateLists[currentFrameIndex].setObjectValidGPU(objectIndex, true);
}

D3D12_GPU_VIRTUAL_ADDRESS DXManager::getCBVVirtualAddress(UINT objectIndex, int eyeNum)
{
	// frame related base address
	D3D12_GPU_VIRTUAL_ADDRESS dest = singleCBVResources[currentFrameIndex]->GetGPUVirtualAddress();
	UINT64 plus = getOffsetInConstantBuffer(objectIndex, eyeNum);
	//plus = 0;
	return dest + plus;
	//return 0;
}

void DXManager::copyToComputeBuffer(FrameResourceSimple & f)
{
	ThrowIfFailed(commandAllocators[currentFrameIndex]->Reset());
	ThrowIfFailed(commandLists[currentFrameIndex]->Reset(commandAllocators[currentFrameIndex].Get(), graphics_ps));
	// Set necessary state.
	//commandLists[currentFrame]->SetGraphicsRootSignature(rootSignature.Get());
	UINT64 source_offset = currentFrameIndex * totalSize;
	resourceStateHelper->toState(singleCBVResources[currentFrameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST, commandLists[currentFrameIndex].Get());
	commandLists[currentFrameIndex]->CopyBufferRegion(singleCBVResources[currentFrameIndex].Get(), 0L, constantBufferUpload.Get(), source_offset, totalSize);
	resourceStateHelper->toState(singleCBVResources[currentFrameIndex].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, commandLists[currentFrameIndex].Get());
	//resourceStateHelper->toState(singleCBVResources[currentFrame].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, commandLists[currentFrame].Get());
	//commandLists[currentFrame]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(singleCBVResources[currentFrame].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	ThrowIfFailed(commandLists[currentFrameIndex]->Close());
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { commandLists[currentFrameIndex].Get() };
	commandQueues[currentFrameIndex]->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	EffectBase::createSyncPoint(f, commandQueues[currentFrameIndex]);
	EffectBase::waitForSyncPoint(f);
	// signal updated const buffer: only objects already updated in GPU will be ok in compute buffer
	if (!xapp->ovrRendering)
		assert(maxObjects == totalSize / slotSize);
	else 
		assert(maxObjects == (totalSize/2) / slotSize);
	for (unsigned int i = 0; i < maxObjects; i++) {
		objectStateLists[currentFrameIndex].setObjectValidCompute(i, true);
	}
}
void DXManager::createPSO(EffectFrameResource &res, int frameIndex)
{
	// Create an empty root signature.
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(xapp->device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&res.rootSignature)));
	}
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = res.rootSignature.Get();
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
	ThrowIfFailed(xapp->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&res.pipelineState)));
	NAME_D3D12_OBJECT_SUFF(res.pipelineState, frameIndex);
	ThrowIfFailed(xapp->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&res.commandAllocator)));
	NAME_D3D12_OBJECT_SUFF(res.commandAllocator, frameIndex);
	ThrowIfFailed(xapp->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, res.commandAllocator.Get(), res.pipelineState.Get(), IID_PPV_ARGS(&res.commandList)));
	NAME_D3D12_OBJECT_SUFF(res.commandList, frameIndex);
	res.commandList->Close();
	ThrowIfFailed(xapp->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&res.fence)));
	NAME_D3D12_OBJECT_SUFF(res.fence, frameIndex);
	res.fenceValue = 0;
	res.fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (res.fenceEvent == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

}

// create resources for an app window
void DXManager::createFrameResources(vector<AppWindowFrameResource>& res, int count, ComPtr<IDXGISwapChain3> &swapChain)
{
	Log("app window frame resources size: " << res.size() << endl);
	for (int i = 0; i < count; i++) {
		AppWindowFrameResource appwinres;
		appwinres.frameIndex = i;
		// Create descriptor heaps.
		{
			// Describe and create a render target view (RTV) descriptor heap.

			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = FrameCount;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(xapp->device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&appwinres.rtvHeap)));
			NAME_D3D12_OBJECT_SUFF(appwinres.rtvHeap, i);
			appwinres.rtvDescriptorSize = xapp->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		// Create frame resources.
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(appwinres.rtvHeap->GetCPUDescriptorHandleForHeapStart());

			// Create a RTV
			ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&appwinres.renderTarget)));
			xapp->device->CreateRenderTargetView(appwinres.renderTarget.Get(), nullptr, rtvHandle);
			NAME_D3D12_OBJECT_SUFF(appwinres.renderTarget, i);
			rtvHandle.Offset(1, appwinres.rtvDescriptorSize);
			//ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[n])));

			// Describe and create a depth stencil view (DSV) descriptor heap.
			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
			dsvHeapDesc.NumDescriptors = 1;
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(xapp->device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&appwinres.dsvHeap)));
			// Create the depth stencil view for each frame
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
				IID_PPV_ARGS(&appwinres.depthStencil)
			));

			//NAME_D3D12_OBJECT(m_depthStencil);

			xapp->device->CreateDepthStencilView(appwinres.depthStencil.Get(), &depthStencilDesc, appwinres.dsvHeap->GetCPUDescriptorHandleForHeapStart());
			NAME_D3D12_OBJECT_SUFF(appwinres.depthStencil, i);
		}
		// Create an empty root signature.
		{
			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;
			ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
			ThrowIfFailed(xapp->device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&appwinres.rootSignature)));
		}
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = appwinres.rootSignature.Get();
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
		ThrowIfFailed(xapp->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&appwinres.pipelineState)));
		NAME_D3D12_OBJECT_SUFF(appwinres.pipelineState, i);
		ThrowIfFailed(xapp->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&appwinres.commandAllocator)));
		NAME_D3D12_OBJECT_SUFF(appwinres.commandAllocator, i);
		ThrowIfFailed(xapp->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, appwinres.commandAllocator.Get(), appwinres.pipelineState.Get(), IID_PPV_ARGS(&appwinres.commandList)));
		NAME_D3D12_OBJECT_SUFF(appwinres.commandList, i);
		appwinres.commandList->Close();
		ThrowIfFailed(xapp->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&appwinres.fence)));
		NAME_D3D12_OBJECT_SUFF(appwinres.fence, i);
		appwinres.fenceValue = 0;
		appwinres.fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (appwinres.fenceEvent == nullptr) {
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
		res.push_back(appwinres);
	}
	Log("app window frame resources size: " << res.size() << endl);
}

void DXManager::createSyncPoint(FrameResource &f, ComPtr<ID3D12CommandQueue> queue)
{
	UINT64 threadFenceValue = InterlockedIncrement(&f.fenceValue);
	ThrowIfFailed(queue->Signal(f.fence.Get(), threadFenceValue));
	ThrowIfFailed(f.fence->SetEventOnCompletion(threadFenceValue, f.fenceEvent));
}

void DXManager::waitForSyncPoint(FrameResource & f)
{
	//	int frameIndex = xapp->getCurrentBackBufferIndex();
	UINT64 completed = f.fence->GetCompletedValue();
	//Log("ev start " << f.frameIndex << " " << completed << " " << f.fenceValue << endl);
	if (completed == -1) {
		Error(L"fence.GetCompletedValue breakdown");
	}
	if (completed > 100000) {
		//Log("ev MAX " << completed << " " << f.fenceValue << endl);
	}
	if (completed <= f.fenceValue)
	{
		WaitForSingleObject(f.fenceEvent, INFINITE);
	}
	else {
		//Log("ev " << completed << " " << f.fenceValue << endl);
	}
}

void DXManager::waitGPU(FrameResource & res, ComPtr<ID3D12CommandQueue> queue)
{
	DXManager::createSyncPoint(res, queue);
	DXManager::waitForSyncPoint(res);
}

void DXManager::destroy(vector<AppWindowFrameResource>& resources, ComPtr<ID3D12CommandQueue>& queue)
{
	for each (AppWindowFrameResource res in resources) {
		waitGPU(res, queue);
	}
}
