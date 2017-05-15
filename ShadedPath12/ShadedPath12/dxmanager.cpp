#include "stdafx.h"

void DXManager::createConstantBuffer(UINT maxThreads, UINT maxObjects, size_t singleObjectSize, wchar_t * name) {
	assert(maxObjects > 0);
	this->maxObjects = maxObjects;
	slotSize = calcConstantBufferSize((UINT)singleObjectSize);
	// allocate const buffer for all frames and possibly OVR:
	totalSize = slotSize * maxObjects;
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
			&CD3DX12_RESOURCE_DESC::Buffer(totalSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			//D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
			nullptr,
			IID_PPV_ARGS(&singleCBVResources[i])));
		singleCBVResources[i].Get()->SetName(name);
		ID3D12Resource * resource = singleCBVResources[i].Get();
		resourceStateHelper->add(resource, D3D12_RESOURCE_STATE_COPY_DEST);
		//wstring gpuRwName = wstring(name).append(L"GPU_RW");
		//singleCBVResourcesGPU_RW[i].Get()->SetName(gpuRwName.c_str());
		//Log("GPU virtual: " <<  cbvResource->GetGPUVirtualAddress(); << endl);
		//ThrowIfFailed(singleCBVResources[i]->Map(0, nullptr, reinterpret_cast<void**>(&singleCBV_GPUDests[i])));
	}

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
	for (int n = 0; n < this->frameCount; n++) {
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
	if (xapp().ovrRendering) {
		plus *= 2; // adjust for two eyes
	}
	if (xapp().ovrRendering && eyeNum == 1) {
		plus += slotSize;
	}
	return  plus;
}
