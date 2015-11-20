#include "stdafx.h"

void EffectBase::createSyncPoint(FrameResource &f, ComPtr<ID3D12CommandQueue> queue)
{
	UINT64 threadFenceValue = InterlockedIncrement(&f.fenceValue);
	ThrowIfFailed(queue->Signal(f.fence.Get(), threadFenceValue));
	ThrowIfFailed(f.fence->SetEventOnCompletion(threadFenceValue, f.fenceEvent));
}

void EffectBase::waitForSyncPoint(FrameResource & f)
{
	UINT64 completed = f.fence->GetCompletedValue();
	if (completed < f.fenceValue)
	{
		WaitForSingleObject(f.fenceEvent, INFINITE);
	}
}

void EffectBase::createConstantBuffer(size_t s, wchar_t * name)
{
	UINT cbvSize = calcConstantBufferSize((UINT)s);
	ThrowIfFailed(xapp().device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE, // do not set - dx12 does this automatically depending on resource type
		&CD3DX12_RESOURCE_DESC::Buffer(cbvSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&cbvResource)));
	cbvResource.Get()->SetName(name);
	//Log("GPU virtual: " <<  cbvResource->GetGPUVirtualAddress(); << endl);
	ThrowIfFailed(cbvResource->Map(0, nullptr, reinterpret_cast<void**>(&cbvGPUDest)));
}

void EffectBase::createAndUploadVertexBuffer(size_t bufferSize, size_t vertexSize, void *data, ID3D12PipelineState *pipelineState)
{
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
	UINT vertexBufferSize = (UINT)bufferSize;
	ThrowIfFailed(xapp().device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer)));
	vertexBuffer.Get()->SetName(L"vertexBuffer_lines");

	ThrowIfFailed(xapp().device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexBufferUpload)));
	vertexBufferUpload.Get()->SetName(L"vertexBufferUpload_lines");
	//Log(vertexBufferUpload->GetGPUVirtualAddress());
	// Copy data to the intermediate upload heap and then schedule a copy 
	// from the upload heap to the vertex buffer.
	D3D12_SUBRESOURCE_DATA vertexData = {};
	//vertexData.pData = reinterpret_cast<UINT8*>(&(all.at(0)));
	vertexData.pData = reinterpret_cast<UINT8*>(data);
	vertexData.RowPitch = vertexBufferSize;
	vertexData.SlicePitch = vertexData.RowPitch;

	//PIXBeginEvent(commandLists[frameIndex].Get(), 0, L"lines: update vertex buffer");
	commandLists[frameIndex].Get()->Reset(commandAllocators[frameIndex].Get(), pipelineState);
	UpdateSubresources<1>(commandLists[frameIndex].Get(), vertexBuffer.Get(), vertexBufferUpload.Get(), 0, 0, 1, &vertexData);
	commandLists[frameIndex]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	PIXEndEvent(commandLists[frameIndex].Get());

	// Initialize the vertex buffer view.
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = (UINT)vertexSize;
	vertexBufferView.SizeInBytes = vertexBufferSize;
}
