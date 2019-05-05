#include "stdafx.h"

#include "CompiledShaders/DotcrossVS.h"
#include "CompiledShaders/DotcrossGS.h"
#include "CompiledShaders/DotcrossPS.h"

void Dotcross::init()
{
	if (xapp().disableLineShaders) return;
	initialized = true;
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
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		psoDesc.VS = { binShader_DotcrossVS, sizeof(binShader_DotcrossVS) };
		psoDesc.GS = { binShader_DotcrossGS, sizeof(binShader_DotcrossGS) };
		psoDesc.PS = { binShader_DotcrossPS, sizeof(binShader_DotcrossPS) };
		//Sleep(4000);
		xapp().device->CreateRootSignature(0, binShader_DotcrossGS, sizeof(binShader_DotcrossGS), IID_PPV_ARGS(&rootSignature));
		//ThrowIfFailed(xapp().device->CreateRootSignature(0, binShader_DotcrossGS, sizeof(binShader_DotcrossGS), IID_PPV_ARGS(&rootSignature)));
		//rootSignature.Get()->SetName(L"dotcross_root_signature");
		psoDesc.pRootSignature = rootSignature.Get();
		ThrowIfFailed(xapp().device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
		pipelineState.Get()->SetName(L"state_dotcross_init");

		createConstantBuffer((UINT) sizeof(cbv), L"dotcross_cbv_resource");
		// set cbv data:
		XMMATRIX ident = XMMatrixIdentity();
		XMStoreFloat4x4(&cbv.wvp, ident);
		cbv.linelen = this->currentLinerLenth;
		//cbv.wvp._11 += 2.0f;
		memcpy(cbvGPUDest, &cbv, sizeof(cbv));
	}
	// Create command allocators and command lists for each frame.
	static LPCWSTR fence_names[XApp::FrameCount] = {
		L"fence_dotcross_0", L"fence_dotcross_1", L"fence_dotcross_2"
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

void Dotcross::update(vector<XMFLOAT3>& pts)
{
	if (pts.size() == 0 && points.size() == 0)
		return;
	points.insert(points.end(), pts.begin(), pts.end());
}

void Dotcross::update()
{
	if (xapp().disableLineShaders) return;
	Dotcross *l = this;
	auto fut = async([l] { return l->updateTask(); });
	//return l->updateTask();
}

void Dotcross::updateTask()
{
	vector<Vertex> all;
	mutex_dotcross.lock();
	for (XMFLOAT3 pt : points) {
		Vertex v1;
		v1.pos.x = pt.x;
		v1.pos.y = pt.y;
		v1.pos.z = pt.z;
		v1.color = Colors::Green;
		all.push_back(v1);
	}
	size_t vertexBufferSize = sizeof(Vertex) * all.size();
	mutex_dotcross.unlock();
	int frameIndex = xapp().getCurrentBackBufferIndex();
	//createAndUploadVertexBuffer(vertexBufferSize, sizeof(Vertex), &(all.at(0)), pipelineState.Get(), L"dotcross");
	createAndUploadVertexBuffer(vertexBufferSize, sizeof(Vertex), &(all.at(0)), pipelineState.Get(), L"dotcross", vertexBuffer, vertexBufferUpload, commandAllocators[frameIndex], commandLists[frameIndex], vertexBufferView);

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

void Dotcross::setLineLength(float lineLength)
{
	cbv.linelen = lineLength;
}

void Dotcross::destroy()
{
}

void Dotcross::preDraw(int eyeNum)
{
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
	ID3D12Resource *resource;
	if (!xapp().ovrRendering) resource = xapp().renderTargets[frameIndex].Get();
	else resource = xapp().vr.texResource[frameIndex];
	xapp().handleRTVClearing(commandLists[frameIndex].Get(), rtvHandle, dsvHandle, resource);
}

void Dotcross::draw()
{
	if (xapp().disableLineShaders) return;
	if (points.size() == 0)
		return;
	prepareDraw(&xapp().vr);
	if (!xapp().ovrRendering) {
		XMStoreFloat4x4(&cbv.wvp, xapp().camera.worldViewProjection());
		memcpy(cbvGPUDest, &cbv, sizeof(cbv));
		return drawInternal();
	}
	// draw VR, iterate over both eyes
	for (int eyeNum = 0; eyeNum < 2; eyeNum++) {
		// adjust PVW matrix
		XMMATRIX adjustedEyeMatrix;
		vr_eyes.adjustEyeMatrix(adjustedEyeMatrix, &xapp().camera, eyeNum, &xapp().vr);
		XMStoreFloat4x4(&cbv.wvp, adjustedEyeMatrix);
		memcpy(cbvGPUDest, &cbv, sizeof(cbv));
		drawInternal(eyeNum);
	}
}

void Dotcross::drawInternal(int eyeNum)
{
	int frameIndex = xapp().getCurrentBackBufferIndex();
	preDraw(eyeNum);
	commandLists[frameIndex]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	commandLists[frameIndex]->IASetVertexBuffers(0, 1, &vertexBufferView);
	auto numVertices = points.size();
	commandLists[frameIndex]->DrawInstanced((UINT)numVertices, 1, 0, 0);
	postDraw();
}

void Dotcross::postDraw()
{
	int frameIndex = xapp().getCurrentBackBufferIndex();

	ThrowIfFailed(commandLists[frameIndex]->Close());
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { commandLists[frameIndex].Get() };
	xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	// tests
	auto &f = frameData[frameIndex];
	// Wait for the gpu to complete the draw.
	createSyncPoint(f, xapp().commandQueue);
	waitForSyncPoint(f); // ok, but not optimal
}

