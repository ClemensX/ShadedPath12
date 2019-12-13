#include "stdafx.h"

#include "CompiledShaders/LineVS.h"
#include "CompiledShaders/LinePS.h"

void LinesEffect::init(DXGlobal* a, FrameDataLine* fdl, FrameDataGeneral* fd_general_, Pipeline* pipeline)
{
	dxGlobal = a;
	disabled = !pipeline->isEnabledLineEffectUtil();
	if (disabled) return;
	if (!initialized) {
		initialized = true;
		updateQueue.init();
		effectDataUpdate.init(&appDataSets[0]);
#if !defined(DISABLE_UPDATE_THREADS)
		// update thread
		void* native_handle = pipeline->getThreadGroup()->add_t(runUpdate, pipeline, (Effect*)this);
		wstring mod_name = wstring(L"update_line");//.append(L"_").append(to_wstring(i));
		SetThreadDescription((HANDLE)native_handle, mod_name.c_str());
#endif
		DXGlobal::initSyncPoint(&updateFenceData, dxGlobal->device);
		// Create the pipeline state, which includes compiling and loading shaders.
		{
			// Define the vertex input layout.
			D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			// Describe and create the graphics pipeline state object (PSO).
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			psoDesc.SampleDesc.Count = 1;

			psoDesc.VS = { binShader_LineVS, sizeof(binShader_LineVS) };
			psoDesc.PS = { binShader_LinePS, sizeof(binShader_LinePS) };
			ThrowIfFailed(dxGlobal->device->CreateRootSignature(0, binShader_LinePS, sizeof(binShader_LinePS), IID_PPV_ARGS(&rootSignature)));
			rootSignature.Get()->SetName(L"lines_root_signature");
			psoDesc.pRootSignature = rootSignature.Get();
			ThrowIfFailed(dxGlobal->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
			pipelineState.Get()->SetName(L"state_lines_init");

			// init resources for update thread:
			ThrowIfFailed(dxGlobal->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&updateCommandAllocator)));
			ThrowIfFailed(dxGlobal->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, updateCommandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&updateCommandList)));
			// Command lists are created in the recording state, but there is nothing
			// to record yet. The main loop expects it to be closed, so close it now.
			ThrowIfFailed(updateCommandList->Close());
			// init fences: --> use from FrameDataGeneral
		}
	}

	createConstantBuffer((UINT) sizeof(cbv), L"lines_cbv_resource", fdl);
	// set cbv data:
	XMMATRIX ident = XMMatrixIdentity();
	XMStoreFloat4x4(&cbv.wvp, ident);
	//cbv.wvp._11 += 2.0f;
	memcpy(fdl->cbvGPUDest, &cbv, sizeof(cbv));
}

void LinesEffect::activateAppDataSet(unsigned long user)
{
	//unique_lock<mutex> lock(dataSetMutex);
	auto lea = (LineEffectAppData*)getInactiveAppDataSet(user);
	if (!dxGlobal->pipeline->isShutdown()) {
		if (true /*dirty*/) {
			//if (xapp().pGraphicsAnalysis != nullptr) xapp().pGraphicsAnalysis->BeginCapture();
			// recreate vertex input buffer
			dirty = false;
			vector<Vertex> all;
			// handle fixed lines:
			for (LineDef& line : lea->lines) {
				Vertex v1, v2;
				v1.color = line.color;
				v1.pos = line.start;
				v2.color = line.color;
				v2.pos = line.end;
				all.push_back(v1);
				all.push_back(v2);
			}
			// just add the one-time lines at this point (may be extra function later?
			for (LineDef& line : lea->oneTimeLines) {
				Vertex v1, v2;
				v1.color = line.color;
				v1.pos = line.start;
				v2.color = line.color;
				v2.pos = line.end;
				all.push_back(v1);
				all.push_back(v2);
			}
			lea->oneTimeLines.clear();
			// one-time lines end
			lea->numVericesToDraw = (UINT)all.size();
			size_t vertexBufferSize = sizeof(Vertex) * all.size();//lines.size() * 2;

			BufferResource* res = ResourceStore::getInstance()->getSlot();
			lea->bufferResource = res;
			createAndUploadVertexBuffer(vertexBufferSize, sizeof(Vertex), &(all.at(0)), pipelineState.Get(),
				L"lines", res->vertexBuffer, res->vertexBufferUpload, updateCommandAllocator, updateCommandList, res->vertexBufferView);

			// Close the command list and execute it to begin the vertex buffer copy into
			// the default heap.
			ThrowIfFailed(updateCommandList->Close());
			ID3D12CommandList* ppCommandListsUpload[] = { updateCommandList.Get() };
			dxGlobal->commandQueue->ExecuteCommandLists(_countof(ppCommandListsUpload), ppCommandListsUpload);
			dxGlobal->createSyncPoint(&updateFenceData, dxGlobal->commandQueue);
			dxGlobal->waitForSyncPoint(&updateFenceData);
			ResourceStore::getInstance()->freeUnusedSlots(res->generation - 1);
		}
	}
	// switch inactive and active data sets:
	currentActiveAppDataSet = (currentActiveAppDataSet + 1) % 2;
	currentInactiveAppDataSet = (currentInactiveAppDataSet + 1) % 2;
}

void LinesEffect::draw(Frame* frame, FrameDataGeneral* fdg, FrameDataLine* fdl, Pipeline* pipeline)
{
	auto config = pipeline->getPipelineConfig();
	dxGlobal->waitGPU(fdg, dxGlobal->commandQueue);
	{
		// prepare drawing:
		ID3D12GraphicsCommandList* commandList = fdg->commandListRenderTexture.Get();
		ThrowIfFailed(fdg->commandAllocatorRenderTexture->Reset());
		ThrowIfFailed(commandList->Reset(fdg->commandAllocatorRenderTexture.Get(), pipelineState.Get()));
		resourceStateHelper->toState(fdg->renderTargetRenderTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, commandList);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(fdg->rtvHeapRenderTexture->GetCPUDescriptorHandleForHeapStart(), 0, fdg->rtvDescriptorSizeRenderTexture);

		// prepare viewport and scissor rect:
		int width = config.backbufferWidth;
		int height = config.backbufferHeight;
		D3D12_VIEWPORT viewport;
		viewport.MinDepth = 0.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width = static_cast<float>(width);
		viewport.Height = static_cast<float>(height);
		viewport.MaxDepth = 1.0f;

		D3D12_RECT scissorRect;
		scissorRect.left = 0;
		scissorRect.top = 0;
		scissorRect.right = static_cast<LONG>(width);
		scissorRect.bottom = static_cast<LONG>(height);

		// Set necessary state.
		commandList->SetGraphicsRootSignature(rootSignature.Get());
		commandList->RSSetViewports(1, &fdg->eyes.viewports[0]);
		commandList->RSSetScissorRects(1, &fdg->eyes.scissorRects[0]);

		// Set CBV
		commandList->SetGraphicsRootConstantBufferView(0, fdl->cbvResource->GetGPUVirtualAddress());

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(fdg->dsvHeapRenderTexture->GetCPUDescriptorHandleForHeapStart());
		fdg->commandListRenderTexture->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		ID3D12Resource* resource = fdg->renderTarget.Get();

		// prepare cbv:
		if (pipeline->isHMD()) {
#if defined(_SVR_)
			//pipeline->getVR()->UpdateHMDMatrixPose(); // cam == null means no cam movement with keyboard
			pipeline->getVR()->UpdateHMDMatrixPose(&fdg->leftCam);
			pipeline->getVR()->SetupCameras();
			Matrix4 wvp = pipeline->getVR()->GetCurrentViewProjectionMatrix(vr::Eye_Left);
			frame->wvpTime = pipeline->gametime.getTimeAbs();
			frame->wvpId = pipeline->getNextWVPNumber();
			memcpy(&cbv.wvp, &wvp, sizeof(cbv.wvp));
			memcpy(fdb->cbvGPUDest, &cbv, sizeof(cbv));
#endif
		}
		else {
			XMStoreFloat4x4(&cbv.wvp, fdg->leftCam.worldViewProjection());
			frame->wvpTime = pipeline->gametime.getTimeAbs();
			frame->wvpId = pipeline->getNextWVPNumber();
			memcpy(fdl->cbvGPUDest, &cbv, sizeof(cbv));
		}
		//Log("size my wvp: " << sizeof(cbv.wvp) << endl);
		//Log("size Steam wvp: " << sizeof(Matrix4) << endl);
		//assert(sizeof(cbv.wvp) == sizeof(Matrix4));

		// draw
		auto d = (LineEffectAppData*)getActiveAppDataSet();

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		commandList->IASetVertexBuffers(0, 1, &d->bufferResource->vertexBufferView);
		//auto numVertices = lines.size() * 2;
		// now draw all the lines
		commandList->DrawInstanced(d->numVericesToDraw, 1, 0, 0);
		if (true && pipeline->isVR()) {
			commandList->SetGraphicsRootConstantBufferView(0, fdl->cbvResource2->GetGPUVirtualAddress());
			// draw right eye:
			commandList->RSSetViewports(1, &fdg->eyes.viewports[1]);
			commandList->RSSetScissorRects(1, &fdg->eyes.scissorRects[1]);
			if (pipeline->isHMD()) {
#if defined(_SVR_)
				Matrix4 wvp = pipeline->getVR()->GetCurrentViewProjectionMatrix(vr::Eye_Right);
				memcpy(&cbv.wvp, &wvp, sizeof(cbv.wvp));
				memcpy(fdb->cbvGPUDest2, &cbv, sizeof(cbv));
#endif
			}
			else {
				XMStoreFloat4x4(&cbv.wvp, fdg->rightCam.worldViewProjection());
				memcpy(fdl->cbvGPUDest2, &cbv, sizeof(cbv));
			}
			commandList->DrawInstanced(d->numVericesToDraw, 1, 0, 0);
		}

		// execute commands:
		ThrowIfFailed(commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { commandList };

		dxGlobal->commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		dxGlobal->waitGPU(fdg, dxGlobal->commandQueue);
		commandList = fdg->commandListRenderTexture.Get();
		releaseActiveAppDataSet(d);
	}
}

void LinesEffect::addOneTime(vector<LineDef>& linesToAdd, unsigned long& user) {
	auto& lines = getInactiveAppDataSet(user)->oneTimeLines;
	if (linesToAdd.size() == 0 && lines.size() == 0)
		return;
	lines.insert(lines.end(), linesToAdd.begin(), linesToAdd.end());
	dirty = true;
}

void LinesEffect::add(vector<LineDef>& linesToAdd, unsigned long& user) {
	auto& lines = getInactiveAppDataSet(user)->lines;
	if (linesToAdd.size() == 0 && lines.size() == 0)
		return;
	lines.insert(lines.end(), linesToAdd.begin(), linesToAdd.end());
	dirty = true;
}

void LinesEffect::update() {
	//if (disabled) return;
	////dirty = true; // uncomment to force CBV update every frame
	//LinesEffect* l = this;
	////auto fut = async([l] { return l->updateTask(); });
}
/*
void LinesEffect::updateTask()
{
	if (signalUpdateCBV) {
		signalUpdateCBV = false;
		cbv = updatedCBV;
		memcpy(cbvGPUDest, &cbv, sizeof(cbv));
	}
	// handle fixed lines:
	if (dirty) {
		//if (xapp().pGraphicsAnalysis != nullptr) xapp().pGraphicsAnalysis->BeginCapture();
		// recreate vertex input buffer
		dirty = false;
		vector<Vertex> all;
		mutex_lines.lock();
		for (LineDef& line : lines) {
			Vertex v1, v2;
			v1.color = line.color;
			v1.pos = line.start;
			v2.color = line.color;
			v2.pos = line.end;
			all.push_back(v1);
			all.push_back(v2);
		}
		// just add the one-time lines at this point (may be extra function later?
		for (LineDef& line : addLines) {
			Vertex v1, v2;
			v1.color = line.color;
			v1.pos = line.start;
			v2.color = line.color;
			v2.pos = line.end;
			all.push_back(v1);
			all.push_back(v2);
		}
		addLines.clear();
		// one-time lines end
		numVericesToDraw = (UINT)all.size();
		size_t vertexBufferSize = sizeof(Vertex) * all.size();//lines.size() * 2;
		mutex_lines.unlock();
		int frameIndex = xapp().getCurrentBackBufferIndex();
		//createAndUploadVertexBuffer(vertexBufferSize, sizeof(Vertex), &(all.at(0)), pipelineState.Get(), L"lines");
		createAndUploadVertexBuffer(vertexBufferSize, sizeof(Vertex), &(all.at(0)), pipelineState.Get(), L"lines", vertexBuffer, vertexBufferUpload, commandAllocators[frameIndex], commandLists[frameIndex], vertexBufferView);

		// Close the command list and execute it to begin the vertex buffer copy into
		// the default heap.
		ThrowIfFailed(commandLists[frameIndex]->Close());
		ID3D12CommandList* ppCommandLists[] = { commandLists[frameIndex].Get() };
		xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Wait for the gpu to complete the update.
		auto& f = frameData[frameIndex];
		createSyncPoint(f, xapp().commandQueue);
		waitForSyncPoint(f);
		//WaitForSingleObject(f.fenceEvent, INFINITE);
		//Sleep(10);
	}
}
*/
void LinesEffect::updateCBV(CBV newCBV)
{
	//updatedCBV = newCBV;
	//signalUpdateCBV = true;
	////cbv.wvp._11 += 2.0f;
}

void LinesEffect::destroy()
{
	//WaitForGpu();
	//CloseHandle(fenceEvent);
}

/*
void LinesEffect::preDraw(int eyeNum) {
	// last frame must have been finished before we run here!!!
	int frameIndex = xapp().getCurrentBackBufferIndex();
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
	commandLists[frameIndex]->RSSetViewports(1, &vr_eyes.viewports[eyeNum]);
	commandLists[frameIndex]->RSSetScissorRects(1, &vr_eyes.scissorRects[eyeNum]);

	// Set CBV
	commandLists[frameIndex]->SetGraphicsRootConstantBufferView(0, cbvResource->GetGPUVirtualAddress());

	// Indicate that the back buffer will be used as a render target.
	//	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(xapp().rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, xapp().rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = xapp().getRTVHandle(frameIndex);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(xapp().dsvHeaps[frameIndex]->GetCPUDescriptorHandleForHeapStart());
	//m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	commandLists[frameIndex]->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	ID3D12Resource* resource;
	if (!xapp().ovrRendering) resource = xapp().renderTargets[frameIndex].Get();
	else resource = xapp().vr.texResource[frameIndex];
	xapp().handleRTVClearing(commandLists[frameIndex].Get(), rtvHandle, dsvHandle, resource);
}

void LinesEffect::draw()
{
	if (xapp().disableLineShaders) return;
	prepareDraw(&xapp().vr);
	if (!xapp().ovrRendering) {
		XMStoreFloat4x4(&cbv.wvp, xapp().camera.worldViewProjection());
		memcpy(cbvGPUDest, &cbv, sizeof(cbv));
		return drawInternal();
	}
	// draw VR, iterate over both eyes
	for (int eyeNum = 0; eyeNum < 2; eyeNum++) {
		// adjust WVP matrix
		XMMATRIX adjustedEyeMatrix;
		vr_eyes.adjustEyeMatrix(adjustedEyeMatrix, &xapp().camera, eyeNum, &xapp().vr);
		XMStoreFloat4x4(&cbv.wvp, adjustedEyeMatrix);
		memcpy(cbvGPUDest, &cbv, sizeof(cbv));
		drawInternal(eyeNum);
	}
}

void LinesEffect::drawInternal(int eyeNum)
{
	int frameIndex = xapp().getCurrentBackBufferIndex();
	preDraw(eyeNum);
	commandLists[frameIndex]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandLists[frameIndex]->IASetVertexBuffers(0, 1, &vertexBufferView);
	//auto numVertices = lines.size() * 2;
	commandLists[frameIndex]->DrawInstanced(numVericesToDraw, 1, 0, 0);
	postDraw();
}

void LinesEffect::postDraw() {
	int frameIndex = xapp().getCurrentBackBufferIndex();
	// Note: do not transition the render target to present here.
	// the transition will occur when the wrapped 11On12 render
	// target resource is released.

	// Indicate that the back buffer will now be used to present.
	//commandLists[frameIndex]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(xapp().renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(commandLists[frameIndex]->Close());
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { commandLists[frameIndex].Get() };
	xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	// tests
	auto& f = frameData[frameIndex];
	// Wait for the gpu to complete the draw.
	createSyncPoint(f, xapp().commandQueue);
	waitForSyncPoint(f); // ok, but not optimal
	//Sleep(100);
	//Log("frame " << frameIndex << " fence val = " << f.fenceValue << endl);
}

*/