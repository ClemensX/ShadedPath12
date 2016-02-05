#include "stdafx.h"

#include "CompiledShaders/LineVS.h"
#include "CompiledShaders/LinePS.h"

void LinesEffect::init()
{
	// try to do all expensive operations like shader loading and PSO creation here
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
		psoDesc.SampleDesc.Count = 1;

		psoDesc.VS = { binShader_LineVS, sizeof(binShader_LineVS) };
		psoDesc.PS = { binShader_LinePS, sizeof(binShader_LinePS) };
		ThrowIfFailed(xapp().device->CreateRootSignature(0, binShader_LinePS, sizeof(binShader_LinePS), IID_PPV_ARGS(&rootSignature)));
		rootSignature.Get()->SetName(L"lines_root_signature");
		psoDesc.pRootSignature = rootSignature.Get();
		ThrowIfFailed(xapp().device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
		pipelineState.Get()->SetName(L"state_lines_init");

		createConstantBuffer((UINT) sizeof(cbv), L"lines_cbv_resource");
		// set cbv data:
		XMMATRIX ident = XMMatrixIdentity();
		XMStoreFloat4x4(&cbv.wvp, ident);
		//cbv.wvp._11 += 2.0f;
		memcpy(cbvGPUDest, &cbv, sizeof(cbv));
	}
	// Create command allocators and command lists for each frame.
	static LPCWSTR fence_names[XApp::FrameCount] = {
		L"fence_lines_0", L"fence_lines_1", L"fence_lines_2"
	};
	for (UINT n = 0; n < XApp::FrameCount; n++)
	{
		ThrowIfFailed(xapp().device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[n])));
		ThrowIfFailed(xapp().device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[n].Get(), pipelineState.Get(), IID_PPV_ARGS(&commandLists[n])));
		// Command lists are created in the recording state, but there is nothing
		// to record yet. The main loop expects it to be closed, so close it now.
		ThrowIfFailed(commandLists[n]->Close());
		// init fences:
		ThrowIfFailed(xapp().device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(frameData[n].fence.GetAddressOf())));
		frameData[n].fence->SetName(fence_names[n]);
		frameData[n].fenceValue = 0;
		frameData[n].fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (frameData[n].fenceEvent == nullptr) {
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
	}
}

void LinesEffect::addOneTime(vector<LineDef> &linesToAdd) {
	if (linesToAdd.size() == 0 && lines.size() == 0)
		return;
	addLines.insert(addLines.end(), linesToAdd.begin(), linesToAdd.end());
}

void LinesEffect::add(vector<LineDef> &linesToAdd) {
	if (linesToAdd.size() == 0 && lines.size() == 0)
		return;
	lines.insert(lines.end(), linesToAdd.begin(), linesToAdd.end());
	dirty = true;
}

void LinesEffect::update() {
	//dirty = true; // uncomment to force CBV update every frame
	LinesEffect *l = this;
	auto fut = async([l]{ return l->updateTask(); });
	//return l->updateTask();
}

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
		for (LineDef line : lines) {
			Vertex v1, v2;
			v1.color = line.color;
			v1.pos = line.start;
			v2.color = line.color;
			v2.pos = line.end;
			all.push_back(v1);
			all.push_back(v2);
		}
		size_t vertexBufferSize = sizeof(Vertex) * lines.size() * 2;
		mutex_lines.unlock();
		UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
		//createAndUploadVertexBuffer(vertexBufferSize, sizeof(Vertex), &(all.at(0)), pipelineState.Get(), L"lines");
		createAndUploadVertexBuffer(vertexBufferSize, sizeof(Vertex), &(all.at(0)), pipelineState.Get(), L"lines", vertexBuffer, vertexBufferUpload, commandAllocators[frameIndex], commandLists[frameIndex], vertexBufferView);

		// Close the command list and execute it to begin the vertex buffer copy into
		// the default heap.
		ThrowIfFailed(commandLists[frameIndex]->Close());
		ID3D12CommandList* ppCommandLists[] = { commandLists[frameIndex].Get() };
		xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Wait for the gpu to complete the update.
		auto &f = frameData[frameIndex];
		createSyncPoint(f, xapp().commandQueue);
		waitForSyncPoint(f);
		//WaitForSingleObject(f.fenceEvent, INFINITE);
		//Sleep(10);
	}
}

void LinesEffect::updateCBV(CBV newCBV)
{
	updatedCBV = newCBV;
	signalUpdateCBV = true;
	//cbv.wvp._11 += 2.0f;
}

void LinesEffect::destroy()
{
	//WaitForGpu();
	//CloseHandle(fenceEvent);
}

void LinesEffect::preDraw() {
	// last frame must have been finished before we run here!!!
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
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

	// Set CBV
	commandLists[frameIndex]->SetGraphicsRootConstantBufferView(0, cbvResource->GetGPUVirtualAddress());

	// Indicate that the back buffer will be used as a render target.
	//	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(xapp().rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, xapp().rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(xapp().dsvHeaps[frameIndex]->GetCPUDescriptorHandleForHeapStart());
	//m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	commandLists[frameIndex]->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	xapp().handleRTVClearing(commandLists[frameIndex].Get(), rtvHandle, dsvHandle);
}

void LinesEffect::draw()
{
	if (!xapp().ovrRendering) {
		XMStoreFloat4x4(&cbv.wvp, xapp().camera.worldViewProjection());
		memcpy(cbvGPUDest, &cbv, sizeof(cbv));
		return drawInternal();
	}
	// draw VR, iterate over both eyes
	xapp().vr.prepareDraw();
	for (int eyeNum = 0; eyeNum < 2; eyeNum++) {
		// adjust PVW matrix
		XMMATRIX adjustedEyeMatrix;
		xapp().vr.adjustEyeMatrix(adjustedEyeMatrix);
		XMStoreFloat4x4(&cbv.wvp, adjustedEyeMatrix);
		memcpy(cbvGPUDest, &cbv, sizeof(cbv));
		drawInternal();
		xapp().vr.nextEye();
	}
	xapp().vr.endDraw();
}

void LinesEffect::drawInternal()
{
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
	preDraw();
	commandLists[frameIndex]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandLists[frameIndex]->IASetVertexBuffers(0, 1, &vertexBufferView);
	auto numVertices = lines.size() * 2;
	commandLists[frameIndex]->DrawInstanced((UINT)numVertices, 1, 0, 0);
	postDraw();
}

void LinesEffect::postDraw() {
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
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
	auto &f = frameData[frameIndex];
	// Wait for the gpu to complete the draw.
	createSyncPoint(f, xapp().commandQueue);
	waitForSyncPoint(f); // ok, but not optimal
	//Sleep(100);
	//Log("frame " << frameIndex << " fence val = " << f.fenceValue << endl);
}

