#include "stdafx.h"

#include "CompiledShaders/LinetextVS.h"
#include "CompiledShaders/LinetextGS.h"
#include "CompiledShaders/DotcrossPS.h"

void Linetext::init()
{
	// try to do all expensive operations like shader loading and PSO creation here
	// Create the pipeline state, which includes compiling and loading shaders.
	{
		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		psoDesc.VS = { binShader_LinetextVS, sizeof(binShader_LinetextVS) };
		psoDesc.GS = { binShader_LinetextGS, sizeof(binShader_LinetextGS) };
		psoDesc.PS = { binShader_DotcrossPS, sizeof(binShader_DotcrossPS) };
		//Sleep(4000);
		xapp().device->CreateRootSignature(0, binShader_LinetextGS, sizeof(binShader_LinetextGS), IID_PPV_ARGS(&rootSignature));
		//ThrowIfFailed(xapp().device->CreateRootSignature(0, binShader_LinetextGS, sizeof(binShader_LinetextGS), IID_PPV_ARGS(&rootSignature)));
		//rootSignature.Get()->SetName(L"Linetext_root_signature");
		psoDesc.pRootSignature = rootSignature.Get();
		ThrowIfFailed(xapp().device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
		pipelineState.Get()->SetName(L"state_Linetext_init");

		createConstantBuffer((UINT) sizeof(cbv), L"Linetext_cbv_resource");
		// set cbv data:
		XMMATRIX ident = XMMatrixIdentity();
		XMStoreFloat4x4(&cbv.wvp, ident);
		cbv.dx = 0.01f;
		cbv.dy = 0.01f;
		memcpy(cbvGPUDest, &cbv, sizeof(cbv));
	}
	// Create command allocators and command lists for each frame.
	static LPCWSTR fence_names[XApp::FrameCount] = {
		L"fence_Linetext_0", L"fence_Linetext_1", L"fence_Linetext_2"
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
}

void Linetext::setSize(float charHeight) {
	cbv.dx = charHeight / 5.0f;
	cbv.dy = cbv.dx;
	memcpy(cbvGPUDest, &cbv, sizeof(cbv));
}

void Linetext::update()
{
	Linetext *l = this;
	//auto fut = async([l] { return l->updateTask(); });
	//return l->updateTask();
}

void Linetext::updateTask()
{
	vector<Vertex> all;
	mutex_Linetext.lock();
	//for (XMFLOAT3 pt : points) {
	//	Vertex v1;
	//	v1.pos.x = pt.x;
	//	v1.pos.y = pt.y;
	//	v1.pos.z = pt.z;
	//	v1.color = Colors::Green;
	//	all.push_back(v1);
	//}
	size_t vertexBufferSize = sizeof(Vertex) * all.size();
	mutex_Linetext.unlock();
	createAndUploadVertexBuffer(vertexBufferSize, sizeof(Vertex), &(all.at(0)), pipelineState.Get(), L"Linetext");
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();

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

void Linetext::destroy()
{
}

void Linetext::preDraw()
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

void Linetext::draw()
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

void Linetext::drawInternal()
{
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
	int i = 0;
	for (auto line : lines) {
		size_t vertexBufferSize = sizeof(TextElement)* line.letters.size();
		createAndUploadVertexBuffer(vertexBufferSize, sizeof(TextElement), &(lines.at(i++).letters.at(0)), pipelineState.Get(), L"Linetext");
		UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();

		// Close the command list and execute it to begin the vertex buffer copy into
		// the default heap.
		ThrowIfFailed(commandLists[frameIndex]->Close());
		ID3D12CommandList* ppCommandLists[] = { commandLists[frameIndex].Get() };
		xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Wait for the gpu to complete the update.
		auto &f = frameData[frameIndex];
		createSyncPoint(f, xapp().commandQueue);
		waitForSyncPoint(f);

		preDraw();
		commandLists[frameIndex]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		// update buffers for this text line:
		//XMStoreFloat4x4(&cbv.wvp, wvp);
		cbv.rot = line.rot;
		memcpy(cbvGPUDest, &cbv, sizeof(cbv));
		commandLists[frameIndex]->IASetVertexBuffers(0, 1, &vertexBufferView);
		commandLists[frameIndex]->DrawInstanced((UINT)line.letters.size(), 1, 0, 0);
		postDraw();
	}
}

void Linetext::postDraw()
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

int Linetext::getLetterValue(char c) {
	//return 0; // TODO
	c = tolower(c);
	//return (c % 25);
	//return (c % 9) + 9 + 7;
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'z')
		return 10 + c - 'a';
	if (c == '-')
		return 10 + 26;
	if (c == '>')
		return 10 + 26 + 1;
	if (c == '.')
		return 10 + 26 + 2;
	return -1;
}

void Linetext::createTextLine(XMFLOAT4 pos, string text, Line& line) {
	TextElement a;
	a.pos = pos;
	for (unsigned int i = 0; i < text.length(); i++) {
		int letter = getLetterValue(text.at(i));
		if (letter != -1) {
			//assert(0 <= i && i <= 25);
			a.pos.w = *reinterpret_cast<float*>(&letter);
			a.info.w = *reinterpret_cast<float*>(&i);
			line.letters.push_back(a);
		}
	}
}

int Linetext::addTextLine(XMFLOAT4 pos, string text, XMFLOAT4X4 rotm) {
	Line line;
	line.rot = rotm;
	createTextLine(pos, text, line);
	if (line.letters.size() > 0) {
		// at least one char
		lines.push_back(line);
		return lines.size() - 1;
	}
	return -1;
}

int Linetext::addTextLine(XMFLOAT4 pos, string text, Plane plane) {
	XMFLOAT4X4 rot;
	switch (plane) {
	case XY:
		XMStoreFloat4x4(&rot, XMMatrixIdentity());
		break;
	case ZY:
		XMStoreFloat4x4(&rot, XMMatrixRotationRollPitchYaw(0, XM_PIDIV2, 0));
		break;
	case YX:
		XMStoreFloat4x4(&rot, XMMatrixRotationRollPitchYaw(0, 0, -XM_PIDIV2));
		break;
	}
	return addTextLine(pos, text, rot);
}

void Linetext::changeTextLine(int linenum, string text) {
	assert(linenum >= 0 && linenum < (int)lines.size());
	Line oldline = lines.at(linenum);
	Line newline;
	newline.rot = oldline.rot;
	createTextLine(oldline.letters.at(0).pos, text, newline);
	lines.at(linenum) = newline;
}

