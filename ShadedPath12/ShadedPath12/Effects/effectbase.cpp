#include "stdafx.h"
//#include "effectbase.h"

void EffectBase::createSyncPoint(FrameResourceSimple &f, ComPtr<ID3D12CommandQueue> queue)
{
	UINT64 threadFenceValue = InterlockedIncrement(&f.fenceValue);
	ThrowIfFailed(queue->Signal(f.fence.Get(), threadFenceValue));
	ThrowIfFailed(f.fence->SetEventOnCompletion(threadFenceValue, f.fenceEvent));
}

void EffectBase::waitForSyncPoint(FrameResourceSimple & f)
{
	//	int frameIndex = xapp->getCurrentBackBufferIndex();
	UINT64 completed = f.fence->GetCompletedValue();
	//Log("ev start " << frameIndex << " " << completed << " " << f.fenceValue << endl);
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

void EffectBase::createConstantBuffer(size_t s, wchar_t * name)
{
	UINT cbvSize = calcConstantBufferSize((UINT)s);
	//ThrowIfFailed(xapp->device->CreateCommittedResource(
	//	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
	//	D3D12_HEAP_FLAG_NONE, // do not set - dx12 does this automatically depending on resource type
	//	&CD3DX12_RESOURCE_DESC::Buffer(cbvSize),
	//	D3D12_RESOURCE_STATE_GENERIC_READ,
	//	nullptr,
	//	IID_PPV_ARGS(&cbvResource)));
	//cbvResource.Get()->SetName(name);
	////Log("GPU virtual: " <<  cbvResource->GetGPUVirtualAddress(); << endl);
	//ThrowIfFailed(cbvResource->Map(0, nullptr, reinterpret_cast<void**>(&cbvGPUDest)));
}


D3D12_GPU_VIRTUAL_ADDRESS EffectBase::getCBVVirtualAddress(int frame, int thread, UINT objectIndex, int eyeNum)
{
	// TODO correction for OVR mode
	//assert(XApp::FrameCount*thread + frame <= singleCBVResources);
	UINT64 plus = slotSize * objectIndex;
	if (xapp->ovrRendering) {
		plus *= 2; // adjust for two eyes
	}
	if (xapp->ovrRendering && eyeNum == 1) {
		plus += slotSize;
	}
	//UINT64 va = singleCBVResources[XApp::FrameCount*thread + frame]->GetGPUVirtualAddress() + plus;
	//Log("va " << va << endl);
	return singleCBVResources[XApp::FrameCount*thread + frame]->GetGPUVirtualAddress() + plus;
}

UINT8* EffectBase::getCBVUploadAddress(int frame, int thread, UINT objectIndex, int eyeNum)
{
	// TODO: slotsize has to be object specific?
	//assert(XApp::FrameCount*thread + frame <= singleCBVResources.size());
	UINT8* mem = singleCBV_GPUDests[XApp::FrameCount*thread + frame];
	if (mem == nullptr) {
		return this->cbvGPUDest;
	}
	UINT64 plus = slotSize * objectIndex;
	if (xapp->ovrRendering) {
		plus *= 2; // adjust for two eyes
	}
	if (xapp->ovrRendering && eyeNum == 1) {
		plus += slotSize;
	}
	//Log("vup " << (mem + plus) << endl);
	return  mem + plus;
}

UINT8* EffectBase::getMemUploadAddress(int frame, int thread, UINT objectIndex, int eyeNum)
{
	// TODO: slotsize has to be object specific?
	//assert(XApp::FrameCount*thread + frame <= singleCBVResources.size());
	UINT8* mem = (UINT8 *)singleMemResources[XApp::FrameCount*thread + frame];
	//if (mem == nullptr) {
	//	return this->cbvGPUDest;
	//}
	UINT64 plus = slotSize * objectIndex;
	if (xapp->ovrRendering) {
		plus *= 2; // adjust for two eyes
	}
	if (xapp->ovrRendering && eyeNum == 1) {
		plus += slotSize;
	}
	//Log("vup " << (mem + plus) << endl);
	return  mem + plus;
}

void EffectBase::createAndUploadVertexBuffer(size_t bufferSize, size_t vertexSize, void *data, ID3D12PipelineState *pipelineState, LPCWSTR baseName,
	ComPtr<ID3D12Resource> &vertexBuffer,
	ComPtr<ID3D12Resource> &vertexBufferUpload,
	ComPtr<ID3D12CommandAllocator> &commandAllocator,
	ComPtr<ID3D12GraphicsCommandList>  &commandList,
	D3D12_VERTEX_BUFFER_VIEW &vertexBufferView
	)
{
	XApp *xapp = XApp::getInstance();
	int frameIndex = xapp->getCurrentBackBufferIndex();
	UINT vertexBufferSize = (UINT)bufferSize;
	//ThrowIfFailed(xapp->device->CreateCommittedResource(
	//	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
	//	D3D12_HEAP_FLAG_NONE,
	//	&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
	//	D3D12_RESOURCE_STATE_COPY_DEST,
	//	nullptr,
	//	IID_PPV_ARGS(&vertexBuffer)));
	//wstring vbName = wstring(L"vertexBuffer_") + baseName;
	//vertexBuffer.Get()->SetName(vbName.c_str());

	//ThrowIfFailed(xapp->device->CreateCommittedResource(
	//	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
	//	D3D12_HEAP_FLAG_NONE,
	//	&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
	//	D3D12_RESOURCE_STATE_GENERIC_READ,
	//	nullptr,
	//	IID_PPV_ARGS(&vertexBufferUpload)));
	//wstring vbNameUpload = wstring(L"vertexBufferUpload_") + baseName;
	//vertexBufferUpload.Get()->SetName(vbNameUpload.c_str());
	//Log(vertexBufferUpload->GetGPUVirtualAddress());
	// Copy data to the intermediate upload heap and then schedule a copy 
	// from the upload heap to the vertex buffer.
	D3D12_SUBRESOURCE_DATA vertexData = {};
	//vertexData.pData = reinterpret_cast<UINT8*>(&(all.at(0)));
	vertexData.pData = reinterpret_cast<UINT8*>(data);
	vertexData.RowPitch = vertexBufferSize;
	vertexData.SlicePitch = vertexData.RowPitch;

	//PIXBeginEvent(commandLists[frameIndex].Get(), 0, L"lines: update vertex buffer");
	commandList.Get()->Reset(commandAllocator.Get(), pipelineState);
	UpdateSubresources<1>(commandList.Get(), vertexBuffer.Get(), vertexBufferUpload.Get(), 0, 0, 1, &vertexData); // TODO
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	PIXEndEvent(commandList.Get());

	// Initialize the vertex buffer view.
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = (UINT)vertexSize;
	vertexBufferView.SizeInBytes = vertexBufferSize;
}

void EffectBase::createAndUploadIndexBuffer(size_t bufferSize, void *data, ID3D12PipelineState *pipelineState, LPCWSTR baseName,
	ComPtr<ID3D12Resource> &indexBuffer,
	ComPtr<ID3D12Resource> &indexBufferUpload,
	ComPtr<ID3D12CommandAllocator> &commandAllocator,
	ComPtr<ID3D12GraphicsCommandList>  &commandList,
	D3D12_INDEX_BUFFER_VIEW &indexBufferView
	)
{
	XApp *xapp = XApp::getInstance();
	int frameIndex = xapp->getCurrentBackBufferIndex();
	UINT indexBufferSize = (UINT)bufferSize;
	//ThrowIfFailed(xapp->device->CreateCommittedResource(
	//	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
	//	D3D12_HEAP_FLAG_NONE,
	//	&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
	//	D3D12_RESOURCE_STATE_COPY_DEST,
	//	nullptr,
	//	IID_PPV_ARGS(&indexBuffer)));
	//wstring ibName = wstring(L"indexBuffer_") + baseName;
	//indexBuffer.Get()->SetName(ibName.c_str());

	//ThrowIfFailed(xapp->device->CreateCommittedResource(
	//	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
	//	D3D12_HEAP_FLAG_NONE,
	//	&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
	//	D3D12_RESOURCE_STATE_GENERIC_READ,
	//	nullptr,
	//	IID_PPV_ARGS(&indexBufferUpload)));
	//wstring ibNameUpload = wstring(L"indexBufferUpload_") + baseName;
	//indexBufferUpload.Get()->SetName(ibNameUpload.c_str());
	//Log(vertexBufferUpload->GetGPUVirtualAddress());
	// Copy data to the intermediate upload heap and then schedule a copy 
	// from the upload heap to the vertex buffer.
	D3D12_SUBRESOURCE_DATA indexData = {};
	//vertexData.pData = reinterpret_cast<UINT8*>(&(all.at(0)));
	indexData.pData = reinterpret_cast<UINT8*>(data);
	indexData.RowPitch = indexBufferSize;
	indexData.SlicePitch = indexData.RowPitch;

	//PIXBeginEvent(commandLists[frameIndex].Get(), 0, L"lines: update vertex buffer");
	// commanList is still open from call to createAndUploadVertexBuffer - do not reset it here
	//commandList.Get()->Reset(commandAllocator.Get(), pipelineState);
	UpdateSubresources<1>(commandList.Get(), indexBuffer.Get(), indexBufferUpload.Get(), 0, 0, 1, &indexData);
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));
	PIXEndEvent(commandList.Get());

	// Initialize the vertex buffer view.
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = indexBufferSize;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
}

void EffectBase::waitForWorkerThreads()
{
	if (workerThreads.size() > 0) {
		// we still have threads running
		for (auto& t : workerThreads) {
			if (t.joinable()) {
				t.join();
			}
		}
		// all threads finished - remove from list
		workerThreads.clear();
	}
}

//void EffectBase::prepareDraw(VR *vr)
//{
//	vr_eyes.viewports[0] = *vr->getViewportByIndex(0);
//	vr_eyes.viewports[1] = *vr->getViewportByIndex(1);
//	vr_eyes.scissorRects[0] = *vr->getScissorRectByIndex(0);
//	vr_eyes.scissorRects[1] = *vr->getScissorRectByIndex(1);
//}

EffectBase::~EffectBase()
{
	waitForWorkerThreads();
	for (auto m : singleMemResources) {
		delete[] m;
	}
}

