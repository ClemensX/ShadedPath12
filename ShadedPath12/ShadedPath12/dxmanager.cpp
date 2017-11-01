#include "stdafx.h"

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
	if (xapp().ovrRendering) totalSize *= 2; // TODO: really needed?
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
	D3D12_GPU_VIRTUAL_ADDRESS dest = cbvSetResources[setSize * setNum + currentFrame]->GetGPUVirtualAddress();
	UINT64 plus = 0; // getOffsetInConstantBuffer(objectIndex, eyeNum);
	return dest + plus;
}

void DXManager::uploadConstantBufferSet(UINT setNum, size_t singleObjectSize, void *mem_source)
{
	// frame related base address
	UINT8 * dest = cbvSetGPUDest[setSize * setNum + currentFrame];
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
	if (xapp().ovrRendering) totalSize *= 2; // TODO: really needed?
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
	// Describe and create a shader resource view (SRV) and unordered
	// access view (UAV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC srvUavHeapDesc = {};
	srvUavHeapDesc.NumDescriptors = FrameCount;
	srvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&srvUavHeapDesc, IID_PPV_ARGS(&srvUavHeap)));
	NAME_D3D12_OBJECT(srvUavHeap);
	for (int n = 0; n < this->frameCount; n++) {
		// Create compute resources.
		D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE };
		ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueues[n])));
		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[n])));
		ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[n].Get(), ps, IID_PPV_ARGS(&commandLists[n])));
		// Command lists are created in the recording state, but there is nothing
		// to record yet. The main loop expects it to be closed, so close it now.
		ThrowIfFailed(commandLists[n]->Close());
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = maxObjects;
		srvDesc.Buffer.StructureByteStride = slotSize;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle0(srvUavHeap->GetCPUDescriptorHandleForHeapStart(), n, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		device->CreateShaderResourceView(singleCBVResources[n].Get(), &srvDesc, srvHandle0);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = maxObjects;
		uavDesc.Buffer.StructureByteStride = slotSize;
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle0(srvUavHeap->GetCPUDescriptorHandleForHeapStart(), n, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		device->CreateUnorderedAccessView(singleCBVResources[n].Get(), nullptr, &uavDesc, uavHandle0);

		// cbv heaps: (because unlimited array does not work for root CBVs we have to use descriptor tables
		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
		cbvHeapDesc.NumDescriptors = 12;
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap[n])));
		NAME_D3D12_OBJECT(cbvHeap[n]);
		// now set cbv and srv in this heap:
		UINT increment = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		// SRV is first entry in descriptor heap: (wrong entry - must be overwritten before draw()
		int heapIndex = 0;
		auto handle1 = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap[n]->GetCPUDescriptorHandleForHeapStart());
		handle1.Offset(heapIndex, increment);
		device->CreateShaderResourceView(singleCBVResources[n].Get(), &srvDesc, handle1);
		//// CBV is second entry
		heapIndex = 1;
		auto handle2 = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap[n]->GetCPUDescriptorHandleForHeapStart());
		handle2.Offset(heapIndex, increment);
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = singleCBVResources[n]->GetGPUVirtualAddress();
		//cbvDesc.BufferLocation += 512;
		cbvDesc.SizeInBytes = 256;
		device->CreateConstantBufferView(&cbvDesc, handle2);
		// 2nd cbv
		heapIndex = 2;
		auto handle3 = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap[n]->GetCPUDescriptorHandleForHeapStart());
		handle3.Offset(heapIndex, increment);
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc2;
		cbvDesc2.BufferLocation = singleCBVResources[n]->GetGPUVirtualAddress();
		cbvDesc2.BufferLocation += 256;
		cbvDesc2.SizeInBytes = 256;
		device->CreateConstantBufferView(&cbvDesc2, handle3);
	}
}

UINT64 DXManager::getOffsetInConstantBuffer(UINT objectIndex, int eyeNum)
{
	UINT64 plus = slotSize * objectIndex;
	if (xapp().ovrRendering) {
		plus *= 2; // adjust for two eyes
	}
	if (xapp().ovrRendering && eyeNum == 1) {
		plus += slotSize;
	}
	return  plus;
}

void DXManager::upload(UINT objectIndex, int eyeNum, void * mem_source)
{
	// frame related base address
	UINT8 * dest = constantBufferUploadCPU + currentFrame * totalSize;
	// add individual object/eye
	dest += getOffsetInConstantBuffer(objectIndex, eyeNum);
	memcpy(dest, mem_source, slotSize);
	objectStateLists[currentFrame].setObjectValidGPU(objectIndex, true);
}

D3D12_GPU_VIRTUAL_ADDRESS DXManager::getCBVVirtualAddress(UINT objectIndex, int eyeNum)
{
	// frame related base address
	D3D12_GPU_VIRTUAL_ADDRESS dest = singleCBVResources[currentFrame]->GetGPUVirtualAddress();
	UINT64 plus = getOffsetInConstantBuffer(objectIndex, eyeNum);
	//plus = 0;
	return dest + plus;
	//return 0;
}

void DXManager::copyToComputeBuffer(FrameResource & f)
{
	ThrowIfFailed(commandAllocators[currentFrame]->Reset());
	ThrowIfFailed(commandLists[currentFrame]->Reset(commandAllocators[currentFrame].Get(), graphics_ps));
	// Set necessary state.
	//commandLists[currentFrame]->SetGraphicsRootSignature(rootSignature.Get());
	UINT64 source_offset = currentFrame * totalSize;
	resourceStateHelper->toState(singleCBVResources[currentFrame].Get(), D3D12_RESOURCE_STATE_COPY_DEST, commandLists[currentFrame].Get());
	commandLists[currentFrame]->CopyBufferRegion(singleCBVResources[currentFrame].Get(), 0L, constantBufferUpload.Get(), source_offset, totalSize);
	resourceStateHelper->toState(singleCBVResources[currentFrame].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, commandLists[currentFrame].Get());
	//resourceStateHelper->toState(singleCBVResources[currentFrame].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, commandLists[currentFrame].Get());
	//commandLists[currentFrame]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(singleCBVResources[currentFrame].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	ThrowIfFailed(commandLists[currentFrame]->Close());
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { commandLists[currentFrame].Get() };
	commandQueues[currentFrame]->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	EffectBase::createSyncPoint(f, commandQueues[currentFrame]);
	EffectBase::waitForSyncPoint(f);
	// signal updated const buffer: only objects already updated in GPU will be ok in compute buffer
	if (!xapp().ovrRendering)
		assert(maxObjects == totalSize / slotSize);
	else 
		assert(maxObjects == (totalSize/2) / slotSize);
	for (unsigned int i = 0; i < maxObjects; i++) {
		objectStateLists[currentFrame].setObjectValidCompute(i, true);
	}
}