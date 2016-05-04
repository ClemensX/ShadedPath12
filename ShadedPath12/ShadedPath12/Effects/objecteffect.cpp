#include "stdafx.h"

#include "CompiledShaders/ObjectVS.h"
#include "CompiledShaders/ObjectPS.h"

void WorldObjectEffect::init(WorldObjectStore *oStore, UINT maxNumObjects) {
	objectStore = oStore;
	oStore->setWorldObjectEffect(this);
	// try to do all expensive operations like shader loading and PSO creation here
	// Create the pipeline state, which includes compiling and loading shaders.
	{
		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		//{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;//D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = blendDesc;
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		psoDesc.VS = { binShader_ObjectVS, sizeof(binShader_ObjectVS) };
		psoDesc.PS = { binShader_ObjectPS, sizeof(binShader_ObjectPS) };
		ThrowIfFailed(xapp().device->CreateRootSignature(0, binShader_ObjectVS, sizeof(binShader_ObjectVS), IID_PPV_ARGS(&rootSignature)));
		rootSignature.Get()->SetName(L"Object_root_signature");
		psoDesc.pRootSignature = rootSignature.Get();
		ThrowIfFailed(xapp().device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
		pipelineState.Get()->SetName(L"state_objecteffect_init");
		cbvAlignedSize = calcConstantBufferSize((UINT)sizeof(cbv));

		//createConstantBuffer((UINT)2 * cbvAlignedSize, L"objecteffect_cbv_resource");
		setSingleCBVMode(maxNumObjects, sizeof(cbv), L"objecteffect_cbvsingle_resource");
		// set cbv data:
		XMMATRIX ident = XMMatrixIdentity();
		XMStoreFloat4x4(&cbv.wvp, ident);
		cbv.world = cbv.wvp;
		//memcpy(cbvGPUDest+cbvAlignedSize, &cbv, sizeof(cbv));
	}

	// Create command allocators and command lists for each frame.
	static LPCWSTR fence_names[XApp::FrameCount] = {
		L"fence_objecteffect_0", L"fence_objecteffect_1", L"fence_objecteffect_2"
	};
	for (UINT n = 0; n < XApp::FrameCount; n++)
	{
		ThrowIfFailed(xapp().device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[n])));
		ThrowIfFailed(xapp().device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[n].Get(), pipelineState.Get(), IID_PPV_ARGS(&commandLists[n])));
		// Command lists are created in the recording state, but there is nothing
		// to record yet. The main loop expects it to be closed, so close it now.
		ThrowIfFailed(commandLists[n]->Close());
		// init fences:
		//ThrowIfFailed(xapp().device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(frameData[n].fence.GetAddressOf())));
		ThrowIfFailed(xapp().device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameData[n].fence)));
		frameData[n].fence->SetName(fence_names[n]);
		frameData[n].fenceValue = 0;
		frameData[n].fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (frameData[n].fenceEvent == nullptr) {
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
	}
	// init resources for update thread:
	ThrowIfFailed(xapp().device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&updateCommandAllocator)));
	ThrowIfFailed(xapp().device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, updateCommandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&updateCommandList)));
	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	ThrowIfFailed(updateCommandList->Close());
	// init fences:
	//ThrowIfFailed(xapp().device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(frameData[n].fence.GetAddressOf())));
	ThrowIfFailed(xapp().device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&updateFrameData.fence)));
	updateFrameData.fence->SetName(L"fence_objecteffect_update");
	updateFrameData.fenceValue = 0;
	updateFrameData.fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (updateFrameData.fenceEvent == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}

void WorldObjectEffect::createAndUploadVertexBuffer(Mesh * mesh) {
	int frameIndex = xapp().getCurrentBackBufferIndex();
	size_t vertexSize = sizeof(WorldObjectVertex::VertexTextured);
	size_t bufferSize = vertexSize * mesh->numVertices;
	void *data = &mesh->vertices[0];
	EffectBase::createAndUploadVertexBuffer(bufferSize, vertexSize, data,
		pipelineState.Get(), L"MeshVB",
		mesh->vertexBuffer,
		mesh->vertexBufferUpload,
		commandAllocators[frameIndex],
		updateCommandList,
		mesh->vertexBufferView
		);
	// upload index
	bufferSize = sizeof(mesh->indexes[0]) * mesh->indexes.size();
	data = &mesh->indexes[0];
	EffectBase::createAndUploadIndexBuffer(bufferSize, data,
		pipelineState.Get(), L"MeshIB",
		mesh->indexBuffer,
		mesh->indexBufferUpload,
		commandAllocators[frameIndex],
		updateCommandList,
		mesh->indexBufferView
		);
	// Close the command list and execute it to begin the vertex buffer copy into
	// the default heap.
	ThrowIfFailed(updateCommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { updateCommandList.Get() };
	xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	//Sleep(100);
	// Wait for the gpu to complete the update.
	auto &f = updateFrameData;
	//
	// Close the command list and execute it to begin the vertex buffer copy into
	// the default heap.
	//ThrowIfFailed(commandLists[frameIndex]->Close());
	//ID3D12CommandList* ppCommandLists[] = { commandLists[frameIndex].Get() };
	//xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	////Sleep(100);
	//// Wait for the gpu to complete the update.
	//auto &f = frameData[frameIndex];
	//
	createSyncPoint(f, xapp().commandQueue);
	waitForSyncPoint(f);
	/*	// prepare vertex buffer:
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(WorldObjectEffect::VertexTextured) * numVertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];	// very important to set address to first element instead of collection: &mesh->vertices is wrong!!
	ThrowIfFailed(WorldObject::xapp->device->CreateBuffer(&vbd, &vinitData, &vertexBuffer));

	// prepare index buffer:
	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(indexes[0]) * numIndexes;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indexes[0];	// very important to set address to first element instead of collection: &mesh->vertices is wrong!!
	ThrowIfFailed(WorldObject::xapp->device->CreateBuffer(&ibd, &iinitData, &indexBuffer));
	*/
}

void WorldObjectEffect::draw(Mesh * mesh, ComPtr<ID3D12Resource> &vertexBuffer, ComPtr<ID3D12Resource> &indexBuffer, XMFLOAT4X4 world, long numIndexes, TextureID tex, Material &material, UINT objNum, float alpha) {
	DrawInfo di(vertexBuffer, indexBuffer);
	di.world = world;
	di.numIndexes = numIndexes;
	di.tex = tex;
	di.alpha = alpha;
	di.mesh = mesh;
	di.material = &material;
	di.objectNum = objNum;
	draw(di);
}

void WorldObjectEffect::preDraw(DrawInfo &di)
{
	// last frame must have been finished before we run here!!!
	int frameIndex = xapp().getCurrentBackBufferIndex();
	if (!inBulkOperation) {
		//auto &f = frameData[xapp().lastPresentedFrame];
		//waitForSyncPoint(f);
		//auto &f = frameData[frameIndex];
		//waitForSyncPoint(f);
		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		ThrowIfFailed(commandAllocators[frameIndex]->Reset());

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		ThrowIfFailed(commandLists[frameIndex]->Reset(commandAllocators[frameIndex].Get(), pipelineState.Get()));
		// Set necessary state.
		commandLists[frameIndex]->SetGraphicsRootSignature(rootSignature.Get());
		commandLists[frameIndex]->RSSetViewports(1, xapp().vr.getViewport());
		commandLists[frameIndex]->RSSetScissorRects(1, xapp().vr.getScissorRect());

		// Set CBVs
		//commandLists[frameIndex]->SetGraphicsRootConstantBufferView(0, cbvResource->GetGPUVirtualAddress() + cbvAlignedSize);
		commandLists[frameIndex]->SetGraphicsRootConstantBufferView(1, xapp().lights.cbvResource->GetGPUVirtualAddress());

		// Indicate that the back buffer will be used as a render target.
		//	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		//CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(xapp().rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, xapp().rtvDescriptorSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = xapp().getRTVHandle(frameIndex);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(xapp().dsvHeaps[frameIndex]->GetCPUDescriptorHandleForHeapStart());
		//m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		commandLists[frameIndex]->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		xapp().handleRTVClearing(commandLists[frameIndex].Get(), rtvHandle, dsvHandle);
	}
	commandLists[frameIndex]->SetGraphicsRootConstantBufferView(0, getCBVVirtualAddress(frameIndex, di.objectNum));
}

/*
 * Calculate WVP matrix from projection and world matrix
 */
XMMATRIX calcWVP(XMMATRIX &toWorld, XMMATRIX &vp) {
	// optimization of commented code via transpose rule: (A*B)T = BT * AT
	//vp = XMMatrixTranspose(vp);
	//toWorld = XMMatrixTranspose(toWorld);
	//XMMATRIX wvp = toWorld * vp;
	//wvp = XMMatrixTranspose(wvp);
	//return wvp;
	return vp * toWorld;
}

void WorldObjectEffect::draw(DrawInfo &di) {
	if (singleCbvBufferMode) assert(di.objectNum < maxObjects);
	int frameIndex = xapp().getCurrentBackBufferIndex();
	xapp().lights.lights.material = *di.material;
	xapp().lights.update();
	if (!xapp().ovrRendering) {
		XMMATRIX vp = xapp().camera.worldViewProjection();
		XMMATRIX toWorld = XMLoadFloat4x4(&di.world);
		XMMATRIX wvp = calcWVP(toWorld, vp);
		XMStoreFloat4x4(&cbv.wvp, wvp);
		cbv.world = di.world;
		cbv.cameraPos.x = xapp().camera.pos.x;
		cbv.cameraPos.y = xapp().camera.pos.y;
		cbv.cameraPos.z = xapp().camera.pos.z;
		cbv.alpha = di.alpha;
		//memcpy(cbvGPUDest + cbvAlignedSize, &cbv, sizeof(cbv));
		memcpy(getCBVUploadAddress(frameIndex, di.objectNum), &cbv, sizeof(cbv));
		drawInternal(di);
		return;
	}
	// draw VR, iterate over both eyes
	xapp().vr.prepareDraw();
	for (int eyeNum = 0; eyeNum < 2; eyeNum++) {
		// adjust PVW matrix
		XMMATRIX adjustedEyeMatrix;
		xapp().vr.adjustEyeMatrix(adjustedEyeMatrix);
		XMMATRIX toWorld = XMLoadFloat4x4(&di.world);
		XMMATRIX wvp = calcWVP(toWorld, adjustedEyeMatrix);
		XMStoreFloat4x4(&cbv.wvp, wvp);
		cbv.world = di.world;
		cbv.cameraPos.x = xapp().camera.pos.x;
		cbv.cameraPos.y = xapp().camera.pos.y;
		cbv.cameraPos.z = xapp().camera.pos.z;
		cbv.alpha = di.alpha;
		memcpy(cbvGPUDest, &cbv, sizeof(cbv));
		drawInternal(di);
		xapp().vr.nextEye();
	}
	xapp().vr.endDraw();
}

void WorldObjectEffect::beginBulkUpdate()
{
	if (inThreadOperation) return;
	int frameIndex = xapp().getCurrentBackBufferIndex();
	inBulkOperation = true;
	ThrowIfFailed(commandAllocators[frameIndex]->Reset());
	ThrowIfFailed(commandLists[frameIndex]->Reset(commandAllocators[frameIndex].Get(), pipelineState.Get()));
	// Set necessary state.
	commandLists[frameIndex]->SetGraphicsRootSignature(rootSignature.Get());
	commandLists[frameIndex]->RSSetViewports(1, xapp().vr.getViewport());
	commandLists[frameIndex]->RSSetScissorRects(1, xapp().vr.getScissorRect());

	// Set CBVs
	//commandLists[frameIndex]->SetGraphicsRootConstantBufferView(0, cbvResource->GetGPUVirtualAddress() + cbvAlignedSize);
	commandLists[frameIndex]->SetGraphicsRootConstantBufferView(1, xapp().lights.cbvResource->GetGPUVirtualAddress());

	// Indicate that the back buffer will be used as a render target.
	//	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(xapp().rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, xapp().rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = xapp().getRTVHandle(frameIndex);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(xapp().dsvHeaps[frameIndex]->GetCPUDescriptorHandleForHeapStart());
	//m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	commandLists[frameIndex]->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	xapp().handleRTVClearing(commandLists[frameIndex].Get(), rtvHandle, dsvHandle);
}

void WorldObjectEffect::endBulkUpdate()
{
	if (inThreadOperation) return;
	inBulkOperation = false;
	postDraw();
}

void WorldObjectEffect::drawInternal(DrawInfo &di)
{
	int frameIndex = xapp().getCurrentBackBufferIndex();
	if (inThreadOperation) {
		commandList->SetGraphicsRootConstantBufferView(0, getCBVVirtualAddress(frameIndex, di.objectNum));
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		// update buffers for this text line:
		//XMStoreFloat4x4(&cbv.wvp, wvp);
		//cbv.rot = lines[0].rot;
		//memcpy(cbvGPUDest, &cbv, sizeof(cbv));
		commandList->IASetVertexBuffers(0, 1, &di.mesh->vertexBufferView);
		commandList->IASetIndexBuffer(&di.mesh->indexBufferView);
		//auto *tex = xapp().textureStore.getTexture(elvec.first);
		// Set SRV
		ID3D12DescriptorHeap* ppHeaps[] = { di.tex->m_srvHeap.Get() };
		commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		commandList->SetGraphicsRootDescriptorTable(2, di.tex->m_srvHeap->GetGPUDescriptorHandleForHeapStart());
		commandList->DrawIndexedInstanced(di.numIndexes, 1, 0, 0, 0);
		return;
	}
	mutex_Object.lock();
	preDraw(di);
	commandLists[frameIndex]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// update buffers for this text line:
	//XMStoreFloat4x4(&cbv.wvp, wvp);
	//cbv.rot = lines[0].rot;
	//memcpy(cbvGPUDest, &cbv, sizeof(cbv));
	commandLists[frameIndex]->IASetVertexBuffers(0, 1, &di.mesh->vertexBufferView);
	commandLists[frameIndex]->IASetIndexBuffer(&di.mesh->indexBufferView);
	//auto *tex = xapp().textureStore.getTexture(elvec.first);
	// Set SRV
	ID3D12DescriptorHeap* ppHeaps[] = { di.tex->m_srvHeap.Get() };
	commandLists[frameIndex]->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	commandLists[frameIndex]->SetGraphicsRootDescriptorTable(2, di.tex->m_srvHeap->GetGPUDescriptorHandleForHeapStart());
	commandLists[frameIndex]->DrawIndexedInstanced(di.numIndexes, 1, 0, 0, 0);
	postDraw();
	mutex_Object.unlock();
	//Sleep(50);
}

void WorldObjectEffect::updateTask(BulkDivideInfo bi, const vector<unique_ptr<WorldObject>> *grp, WorldObjectEffect *effect)
{
	//Log(" obj bulk update thread " << this_thread::get_id() << endl);
	//Log(" obj bulk update thread " << bi.start << endl);
	//this_thread::sleep_for(2s);
	int frameIndex = xapp().getCurrentBackBufferIndex();
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(xapp().device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
	ThrowIfFailed(xapp().device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), effect->pipelineState.Get(), IID_PPV_ARGS(&commandList)));
	//Log(" obj bulk update thread " << bi.end << " complete" << endl);
	commandList->SetGraphicsRootSignature(effect->rootSignature.Get());
	commandList->RSSetViewports(1, xapp().vr.getViewport());
	commandList->RSSetScissorRects(1, xapp().vr.getScissorRect());

	// Set CBVs
	//commandLists[frameIndex]->SetGraphicsRootConstantBufferView(0, cbvResource->GetGPUVirtualAddress() + cbvAlignedSize);
	commandList->SetGraphicsRootConstantBufferView(1, xapp().lights.cbvResource->GetGPUVirtualAddress());

	// Indicate that the back buffer will be used as a render target.
	//	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(xapp().rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, xapp().rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = xapp().getRTVHandle(frameIndex);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(xapp().dsvHeaps[frameIndex]->GetCPUDescriptorHandleForHeapStart());
	//m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	//commandList->SetGraphicsRootConstantBufferView(0, effect->getCBVVirtualAddress(frameIndex, 0/*di.objectNum*/));
	for (auto i = bi.start; i <= bi.end; i++) {
		WorldObject *w = grp->at(i).get();
		//commandList->SetGraphicsRootConstantBufferView(0, effect->getCBVVirtualAddress(frameIndex, w->objectNum));
		w->draw();
		//Log(" pos" << w->objectNum << endl)
		//auto & w = grp[i];
		//w.->draw();
	}
	ThrowIfFailed(commandList->Close());
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	this_thread::sleep_for(20ms);
}

void WorldObjectEffect::postDraw()
{
	int frameIndex = xapp().getCurrentBackBufferIndex();
	if (inBulkOperation) return;

	ThrowIfFailed(commandLists[frameIndex]->Close());
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { commandLists[frameIndex].Get() };
	xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	// tests
	auto &f = frameData[frameIndex];
	// Wait for the gpu to complete the draw.
	//createSyncPoint(f, xapp().commandQueue);
	//waitForSyncPoint(f); // ok, but not optimal
						 //Sleep(1);
}

void WorldObjectEffect::divideBulk(size_t numObjects, size_t numThreads, const vector<unique_ptr<WorldObject>> *grp)
{
	//if (numThreads < 2) return;
	inThreadOperation = true;
	bulkInfos.clear();
	BulkDivideInfo bi;
	UINT totalNum = (UINT) numObjects;
	UINT perThread = totalNum / (UINT)numThreads;
	// adjust for rounding error: better to increase the count per thread by one instead of starting a new thread with very few elements:
	if (perThread * (UINT)numThreads < totalNum)
		perThread++;
	UINT count = 0;
	while (count < totalNum) {
		bi.start = count;
		bi.end = count + perThread - 1;
		if (bi.end > (totalNum - 1))
			bi.end = totalNum - 1;
		count += perThread;
		bulkInfos.push_back(bi);
	}

	int frameIndex = xapp().getCurrentBackBufferIndex();
	// now launch all the threads:
	//for each (BulkDivideInfo bi in bulkInfos)
	//{
	//	WorldObjectEffect *l = this;
	//	auto fut = async(launch::async, [l,bi] { return l->updateTask(bi); });

	//}
	WorldObjectEffect *l = this;
	//auto fut = async(launch::async, [l, bi] { return l->updateTask(bi); });

	waitForWorkerThreads();
	assert(workerThreads.size() == 0);
	//Log("all workers finished " << endl);
	if (workerThreads.size() == 0) {
		//Log("threads start" << endl);
		//auto &fut = async(launch::async, [this, globbi] { this->updateTask(globbi); });
		//auto &fut = async(launch::async, [] { return updateTask(); });
		for each (BulkDivideInfo bi in bulkInfos)
		{
			thread t(updateTask, bi, grp, this);
			workerThreads.push_back(move(t));
		}
		//Log("all threads started" << endl);
	}
	//t.detach();
	//t.join();
	//xapp().mythread = move(t);
}

thread_local ComPtr<ID3D12GraphicsCommandList> WorldObjectEffect::commandList;
