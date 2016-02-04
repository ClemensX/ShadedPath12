#include "stdafx.h"

#include "CompiledShaders/BillboardVS.h"
#include "CompiledShaders/BillboardPS.h"

void Billboard::init() {
	// try to do all expensive operations like shader loading and PSO creation here
	// Create the pipeline state, which includes compiling and loading shaders.
	{
		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			//{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			//{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			//{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64 + 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		psoDesc.VS = { binShader_BillboardVS, sizeof(binShader_BillboardVS) };
		//psoDesc.GS = { binShader_LinetextGS, sizeof(binShader_LinetextGS) };
		psoDesc.PS = { binShader_BillboardPS, sizeof(binShader_BillboardPS) };
		//xapp().device->CreateRootSignature(0, binShader_LinetextGS, sizeof(binShader_LinetextGS), IID_PPV_ARGS(&rootSignature));
		ThrowIfFailed(xapp().device->CreateRootSignature(0, binShader_BillboardVS, sizeof(binShader_BillboardVS), IID_PPV_ARGS(&rootSignature)));
		//rootSignature.Get()->SetName(L"Linetext_root_signature");
		psoDesc.pRootSignature = rootSignature.Get();
		ThrowIfFailed(xapp().device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
		pipelineState.Get()->SetName(L"state_billboard_init");

		createConstantBuffer((UINT) sizeof(cbv), L"Billboard_cbv_resource");
		// set cbv data:
		XMMATRIX ident = XMMatrixIdentity();
		XMStoreFloat4x4(&cbv.wvp, ident);
		memcpy(cbvGPUDest, &cbv, sizeof(cbv));
	}

	// Create command allocators and command lists for each frame.
	static LPCWSTR fence_names[XApp::FrameCount] = {
		L"fence_Billboard_0", L"fence_Billboard_1", L"fence_Billboard_2"
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
	updateFrameData.fence->SetName(L"fence_Billboard_update");
	updateFrameData.fenceValue = 0;
	updateFrameData.fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (updateFrameData.fenceEvent == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}

void Billboard::update()
{
/*	mutex_Billboard.lock();
	if (updateRunning) {
		// no need to start another update task if the old one is not ready
		//Log("lintext update still running." << endl);
		return;
	}
	updateRunning = true;
	mutex_Billboard.unlock();
	Billboard *l = this;
	auto fut = async(launch::async, [l] { return l->updateTask(); });
*/
	static boolean done = false;
	if (done) return;
	updateTask();
	done = true;
	//Log("update ready" << endl);
}

void Billboard::updateTask()
{
	mutex_Billboard.lock();
	//updateRunning = true; already done in update()
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
	// first run: determine size for all text
	vector<Vertex>& vertexBuffer = recreateVertexBufferContent();
	size_t vertexBufferSize = sizeof(Vertex) * vertexBuffer.size();
	createAndUploadVertexBuffer(vertexBufferSize, sizeof(Vertex), &(vertexBuffer.at(0)), pipelineState.Get(), L"Billboard2", vertexBufferX, vertexBufferUploadX, commandAllocators[frameIndex], updateCommandList, vertexBufferViewX);
	mutex_Billboard.unlock();

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
	// all is ready - now activate the updated vertexBufferView
	mutex_Billboard.lock();
	this->vertexBufferView = vertexBufferViewX;
	mutex_Billboard.unlock();
	updateRunning = false;
	//Log("updateTask ready" << endl);
}

void Billboard::preDraw()
{
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
	commandLists[frameIndex]->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	xapp().handleRTVClearing(commandLists[frameIndex].Get(), rtvHandle);
}

void Billboard::draw()
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

void Billboard::drawInternal()
{
	mutex_Billboard.lock();
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
	preDraw();
	commandLists[frameIndex]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// update buffers for this text line:
	//XMStoreFloat4x4(&cbv.wvp, wvp);
	//cbv.rot = lines[0].rot;
	memcpy(cbvGPUDest, &cbv, sizeof(cbv));
	commandLists[frameIndex]->IASetVertexBuffers(0, 1, &vertexBufferView);
	// Set SRV
	TextureInfo *tex = xapp().textureStore.getTexture("default");
	ID3D12DescriptorHeap* ppHeaps[] = { tex->m_srvHeap.Get() };
	commandLists[frameIndex]->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	commandLists[frameIndex]->SetGraphicsRootDescriptorTable(1, tex->m_srvHeap->GetGPUDescriptorHandleForHeapStart());
	commandLists[frameIndex]->DrawInstanced(6, 1, 0, 0);
	postDraw();
	mutex_Billboard.unlock();
	//Sleep(50);
}

void Billboard::postDraw()
{
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();

	ThrowIfFailed(commandLists[frameIndex]->Close());
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { commandLists[frameIndex].Get() };
	xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	// tests
	auto &f = frameData[frameIndex];
	// Wait for the gpu to complete the draw.
	createSyncPoint(f, xapp().commandQueue);
	waitForSyncPoint(f); // ok, but not optimal
						 //Sleep(1);
}


vector<Billboard::Vertex>& Billboard::recreateVertexBufferContent()
{
	static vector<Vertex> vertices;
	if (vertices.size() > 0) return vertices;
	Vertex all[] = {
		// 1st triangle
		{ XMFLOAT4(-1, -1, 0, 1), XMFLOAT4(), XMFLOAT2(0, 1) },
		{ XMFLOAT4(-1, 1, 0, 1),  XMFLOAT4(), XMFLOAT2(0, 0) },
		{ XMFLOAT4(1, -1, 0, 1),  XMFLOAT4(), XMFLOAT2(1, 1) },
		// 2nd triangle
		{ XMFLOAT4(1, -1, 0, 1),  XMFLOAT4(), XMFLOAT2(1, 1) },
		{ XMFLOAT4(-1, 1, 0, 1),  XMFLOAT4(), XMFLOAT2(0, 0) },
		{ XMFLOAT4(1,  1, 0, 1),  XMFLOAT4(), XMFLOAT2(1, 0) }
	};
	for (int i = 0; i < _countof(all); i++) {
		vertices.push_back(all[i]);
	}
	return vertices;
}

BillboardElement & Billboard::get(string texture_id, int order_num) {
	auto &v = billboards[texture_id];
	return v.at(order_num);
}

size_t Billboard::add(string texture_id, BillboardElement billboardEl) {
	auto &v = billboards[texture_id];
	v.push_back(billboardEl);
	return v.size() - 1;
}