#include "stdafx.h"

void Effect::createBufferUploadResources(BufferResource* res, void *anything,
	ComPtr<ID3D12CommandAllocator>& commandAllocator,
	ComPtr<ID3D12GraphicsCommandList>& commandList
	)
{
	Log("this thread: " << GetCurrentThreadId() << endl);
	ID3D12PipelineState* pipelineState = (ID3D12PipelineState*)anything;

	DXGlobal::initSyncPoint(&res->fenceData, dxGlobal->device);
	UINT vertexBufferSize = (UINT)res->bufferSize;
	ThrowIfFailed(dxGlobal->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&res->vertexBuffer)));
	wstring vbName = wstring(L"vertexBuffer_") + res->baseName;
	res->vertexBuffer.Get()->SetName(vbName.c_str());

	ThrowIfFailed(dxGlobal->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&res->vertexBufferUpload)));
	wstring vbNameUpload = wstring(L"vertexBufferUpload_") + res->baseName;
	res->vertexBufferUpload.Get()->SetName(vbNameUpload.c_str());
	//Log(vertexBufferUpload->GetGPUVirtualAddress());
	// Copy data to the intermediate upload heap and then schedule a copy 
	// from the upload heap to the vertex buffer.
	D3D12_SUBRESOURCE_DATA vertexData = {};
	//vertexData.pData = reinterpret_cast<UINT8*>(&(all.at(0)));
	vertexData.pData = reinterpret_cast<UINT8*>(res->data);
	vertexData.RowPitch = vertexBufferSize;
	vertexData.SlicePitch = vertexData.RowPitch;

//	// Create an empty root signature.
//	{
//		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
//		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
//
//		ComPtr<ID3DBlob> signature;
//		ComPtr<ID3DBlob> error;
//		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
//		ThrowIfFailed(dxGlobal->device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&res->rootSignature)));
//	}
//	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
//	{
//		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
//	};
//	// Describe and create the graphics pipeline state object (PSO).
//	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
//	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
//	psoDesc.pRootSignature = res->rootSignature.Get();
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
//	//psoDesc.VS = { nullptr, 0 };
//	ThrowIfFailed(dxGlobal->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&res->pipelineState)));
//	Log(" create PSO BufferResource &fres->pipelineState: " << std::hex << pipelineState << endl);
//	NAME_D3D12_OBJECT_SUFF(res->pipelineState, res->generation);
	//ThrowIfFailed(dxGlobal->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&res->commandAllocator)));
	//NAME_D3D12_OBJECT_SUFF(res->commandAllocator, res->generation);
	//ThrowIfFailed(dxGlobal->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, res->commandAllocator.Get(), pipelineState, IID_PPV_ARGS(&res->commandList)));
	//NAME_D3D12_OBJECT_SUFF(res->commandList, res->generation);
	//res->commandList->Close();

	//PIXBeginEvent(commandLists[frameIndex].Get(), 0, L"lines: update vertex buffer");
	Log(" upload PSO: " << std::hex << pipelineState << endl);
	//res->commandList.Get()->Reset(res->commandAllocator.Get(), pipelineState);
	//UpdateSubresources<1>(res->commandList.Get(), res->vertexBuffer.Get(), res->vertexBufferUpload.Get(), 0, 0, 1, &vertexData); // TODO
	//res->commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(res->vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	commandList.Get()->Reset(commandAllocator.Get(), pipelineState);
	UpdateSubresources<1>(commandList.Get(), res->vertexBuffer.Get(), res->vertexBufferUpload.Get(), 0, 0, 1, &vertexData); // TODO
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(res->vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	// produce before state error: commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	//PIXEndEvent(commandList.Get());

	// Initialize the vertex buffer view.
	res->vertexBufferView.BufferLocation = res->vertexBuffer->GetGPUVirtualAddress();
	res->vertexBufferView.StrideInBytes = (UINT)res->vertexSize;
	res->vertexBufferView.SizeInBytes = vertexBufferSize;

	// Close the command list and execute it to begin the vertex buffer copy into
	// the default heap.
	//ThrowIfFailed(res->commandList->Close());
	//ID3D12CommandList* ppCommandListsUpload[] = { res->commandList.Get() };
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* ppCommandListsUpload[] = { commandList.Get() };
	dxGlobal->commandQueue->ExecuteCommandLists(_countof(ppCommandListsUpload), ppCommandListsUpload);
	dxGlobal->createSyncPoint(&res->fenceData, dxGlobal->commandQueue);
	dxGlobal->waitForSyncPoint(&res->fenceData);
}

void Effect::releaseBufferUploadResources(BufferResource* res)
{
}

void Effect::createConstantBuffer(size_t s, LPCWSTR name, FrameDataBase* frameData)
{
	UINT cbvSize = calcConstantBufferSize((UINT)s);
	ThrowIfFailed(dxGlobal->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE, // do not set - dx12 does this automatically depending on resource type
		&CD3DX12_RESOURCE_DESC::Buffer(cbvSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&frameData->cbvResource)));
	frameData->cbvResource.Get()->SetName(name);
	Log("GPU virtual: " << frameData->cbvResource->GetGPUVirtualAddress() << endl);
	ThrowIfFailed(frameData->cbvResource->Map(0, nullptr, reinterpret_cast<void**>(&frameData->cbvGPUDest)));
	ThrowIfFailed(dxGlobal->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE, // do not set - dx12 does this automatically depending on resource type
		&CD3DX12_RESOURCE_DESC::Buffer(cbvSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&frameData->cbvResource2)));
	frameData->cbvResource.Get()->SetName(name);
	Log("GPU virtual: " << frameData->cbvResource2->GetGPUVirtualAddress() << endl);
	ThrowIfFailed(frameData->cbvResource2->Map(0, nullptr, reinterpret_cast<void**>(&frameData->cbvGPUDest2)));
}

void Effect::createAndUploadVertexBuffer(size_t bufferSize, size_t vertexSize, void* data, ID3D12PipelineState* pipelineState, LPCWSTR baseName,
	ComPtr<ID3D12Resource>& vertexBuffer,
	ComPtr<ID3D12Resource>& vertexBufferUpload,
	ComPtr<ID3D12CommandAllocator>& commandAllocator,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	D3D12_VERTEX_BUFFER_VIEW& vertexBufferView
)
{
	UINT vertexBufferSize = (UINT)bufferSize;
	ThrowIfFailed(dxGlobal->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer)));
	wstring vbName = wstring(L"vertexBuffer_") + baseName;
	vertexBuffer.Get()->SetName(vbName.c_str());

	ThrowIfFailed(dxGlobal->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexBufferUpload)));
	wstring vbNameUpload = wstring(L"vertexBufferUpload_") + baseName;
	vertexBufferUpload.Get()->SetName(vbNameUpload.c_str());
	//Log(vertexBufferUpload->GetGPUVirtualAddress());
	// Copy data to the intermediate upload heap and then schedule a copy 
	// from the upload heap to the vertex buffer.
	D3D12_SUBRESOURCE_DATA vertexData = {};
	//vertexData.pData = reinterpret_cast<UINT8*>(&(all.at(0)));
	vertexData.pData = reinterpret_cast<UINT8*>(data);
	vertexData.RowPitch = vertexBufferSize;
	vertexData.SlicePitch = vertexData.RowPitch;

	//PIXBeginEvent(commandLists[frameIndex].Get(), 0, L"lines: update vertex buffer");
	Log(" upload PSO: " << std::hex << pipelineState << endl);
	commandList.Get()->Reset(commandAllocator.Get(), pipelineState);
	UpdateSubresources<1>(commandList.Get(), vertexBuffer.Get(), vertexBufferUpload.Get(), 0, 0, 1, &vertexData); // TODO
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	// produce before state error: commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	//PIXEndEvent(commandList.Get());

	// Initialize the vertex buffer view.
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = (UINT)vertexSize;
	vertexBufferView.SizeInBytes = vertexBufferSize;
}

void Effect::createAndUploadIndexBuffer(size_t bufferSize, void* data, ID3D12PipelineState* pipelineState, LPCWSTR baseName,
	ComPtr<ID3D12Resource>& indexBuffer,
	ComPtr<ID3D12Resource>& indexBufferUpload,
	ComPtr<ID3D12CommandAllocator>& commandAllocator,
	ComPtr<ID3D12GraphicsCommandList>& commandList,
	D3D12_INDEX_BUFFER_VIEW& indexBufferView
)
{
	//int frameIndex = xapp().getCurrentBackBufferIndex();
	UINT indexBufferSize = (UINT)bufferSize;
	ThrowIfFailed(dxGlobal->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&indexBuffer)));
	wstring ibName = wstring(L"indexBuffer_") + baseName;
	indexBuffer.Get()->SetName(ibName.c_str());

	ThrowIfFailed(dxGlobal->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBufferUpload)));
	wstring ibNameUpload = wstring(L"indexBufferUpload_") + baseName;
	indexBufferUpload.Get()->SetName(ibNameUpload.c_str());
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
	//PIXEndEvent(commandList.Get());

	// Initialize the vertex buffer view.
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = indexBufferSize;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
}

void Effect::update(vector<Effect*> effectList, Pipeline* pipeline, unsigned long& user)
{
	// initiating phase: trigger all effect update threads
	for (Effect* eff : effectList) {
		EffectAppData* inactiveDataSet = eff->getInactiveAppDataSet(user);
		if (!inactiveDataSet->noUpdate) {
			eff->updateQueue.push(inactiveDataSet, pipeline);
			eff->updateQueue.releaseLockedInactiveDataSet(user);
		}
	}
	// synchronization phase: wait until all effect update threads have finished
	for (Effect* eff : effectList) {
		EffectAppData* inactiveDataSet = eff->getInactiveAppDataSet(user);
		if (!inactiveDataSet->noUpdate) {
			eff->updateQueue.waitForEffectUpdateFinish();
		}
		//Log("GOTSCHA" << endl);
	}
}

void Effect::runUpdate(Pipeline* pipeline, Effect *effectInstance) {
	Log("start effect update thread" << endl);
	static long long calls = 0L;
	while (!pipeline->isShutdown()) {
		calls++;
		PIXBeginEvent(PIX_COLOR_INDEX(6), "effect update %d", calls);
		//Log(" Effect::runUpdate loop start " << endl);
		EffectAppData* ead = effectInstance->updateQueue.pop(pipeline);
		//Log(" Effect::runUpdate EffectAppData " << ead << " calls == " << calls << endl);
		unsigned long user = 0;
		effectInstance->updateQueue.getLockedInactiveDataSet(user);
		//Log(" Effect::runUpdate getlocked data set" << endl);
		effectInstance->activateAppDataSet(user);
		//Log(" Effect::runUpdate activate" << endl);
		effectInstance->updateQueue.releaseLockedInactiveDataSet(user);
		//Log(" Effect::runUpdate release lock" << endl);
		effectInstance->updateQueue.triggerEffectUpdateFinished();
		//Log(" Effect::runUpdate trigget finished" << endl);
		PIXEndEvent();
	}
	Log("end effect update thread" << endl);
	Log("   calls " << calls << endl);
}

// wait until next data set is available
// will only be called by update thread

inline EffectAppData* UpdateQueue::pop(Pipeline* pipeline) {
	unique_lock<mutex> lock(monitorMutex);
	while (myqueue.empty()) {
		cond.wait_for(lock, chrono::milliseconds(3000));
		if (myqueue.empty()) {
			LogF("UpdateQueue wait suspended\n");
		}
		if (pipeline->isShutdown()) {
			LogF("UpdateQueue shutdown in pop\n");
			return nullptr;
		}
	}
	assert(myqueue.empty() == false);
	EffectAppData* ad = myqueue.front();
	myqueue.pop();
	cond.notify_one();
	return ad;
}

// push finished data set

inline void UpdateQueue::push(EffectAppData* ed, Pipeline* pipeline) {
	unique_lock<mutex> lock(monitorMutex);
	if (pipeline->isShutdown()) {
		Log("UpdateQueue shutdown in UpdateQueue push" << endl);
		return;
	}
	// remove old entries - they are obsolete when new data set arrives
	while (myqueue.size() > 0) {
		myqueue.pop();
		Log("UpdateQueue removed obsolete entry " << ed << endl);
	}
	myqueue.push(ed);
	//Log("UpdateQueue length " << myqueue.size() << endl);
	cond.notify_one();
}
