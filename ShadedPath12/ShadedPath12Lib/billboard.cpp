#include "stdafx.h"

BillboardElement& Billboard::get(string texture_id, int order_num) {
	auto& billboards = getInactiveAppDataSet()->billboards;
	auto& v = billboards[texture_id];
	return v.at(order_num);
}

size_t Billboard::add(string texture_id, BillboardElement billboardEl) {
	auto& billboards = getInactiveAppDataSet()->billboards;
	auto& v = billboards[texture_id];
	v.push_back(billboardEl);
	return v.size() - 1;
}

#include "CompiledShaders/BillboardVS.h"
#include "CompiledShaders/BillboardPS.h"

void Billboard::init(DXGlobal* a, FrameDataBillboard* fdb, FrameDataGeneral* fd_general_) {
	initialized = true;
	dxGlobal = a;
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
		//psoDesc.DepthStencilState.DepthEnable = TRUE;
		//psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		//psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		//psoDesc.DepthStencilState.StencilEnable = FALSE;
		//const D3D12_DEPTH_STENCIL_DESC&(ds);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		psoDesc.VS = { binShader_BillboardVS, sizeof(binShader_BillboardVS) };
		//psoDesc.GS = { binShader_LinetextGS, sizeof(binShader_LinetextGS) };
		psoDesc.PS = { binShader_BillboardPS, sizeof(binShader_BillboardPS) };
		//xapp().device->CreateRootSignature(0, binShader_LinetextGS, sizeof(binShader_LinetextGS), IID_PPV_ARGS(&rootSignature));
		ThrowIfFailed(a->device->CreateRootSignature(0, binShader_BillboardVS, sizeof(binShader_BillboardVS), IID_PPV_ARGS(&fdb->rootSignature)));
		//rootSignature.Get()->SetName(L"Linetext_root_signature");
		psoDesc.pRootSignature = fdb->rootSignature.Get();
		ThrowIfFailed(a->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&fdb->pipelineState)));
		fdb->pipelineState.Get()->SetName(L"state_billboard_init");

		createConstantBuffer((UINT) sizeof(cbv), L"Billboard_cbv_resource", fdb);
		// set cbv data:
		XMMATRIX ident = XMMatrixIdentity();
		XMStoreFloat4x4(&cbv.wvp, ident);
		memcpy(fdb->cbvGPUDest, &cbv, sizeof(cbv));
	}

	// Create command allocators and command lists for each frame.
	static LPCWSTR fence_names[XApp::FrameCount] = {
		L"fence_Billboard_0", L"fence_Billboard_1", L"fence_Billboard_2"
	};
/***	for (UINT n = 0; n < XApp::FrameCount; n++)
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
***/
	// init resources for update thread:
	ThrowIfFailed(a->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&fdb->updateCommandAllocator)));
	ThrowIfFailed(a->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, fdb->updateCommandAllocator.Get(), fdb->pipelineState.Get(), IID_PPV_ARGS(&fdb->updateCommandList)));
	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	ThrowIfFailed(fdb->updateCommandList->Close());
	// init fences: --> use from FrameDataGeneral
	//ThrowIfFailed(a->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&updateFrameData.fence)));
	//updateFrameData.fence->SetName(L"fence_Billboard_update");
	//updateFrameData.fenceValue = 0;
	//updateFrameData.fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	//if (updateFrameData.fenceEvent == nullptr) {
	//	ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	//}
}

void Billboard::draw(FrameDataGeneral* fdg, FrameDataBillboard* fdb, Pipeline* pipeline)
{
	//Log("draw " << endl);
	auto config = pipeline->getPipelineConfig();
	//{
	//	// test draw with chnaging background color:
	//	// wait for last frame with this index to be finished:
	//	dxGlobal->waitGPU(fd, dxGlobal->commandQueue);
	//	// d3d12 present:
	//	ID3D12GraphicsCommandList* commandList = fd->commandListRenderTexture.Get();
	//	ThrowIfFailed(fd->commandAllocatorRenderTexture->Reset());
	//	ThrowIfFailed(commandList->Reset(fd->commandAllocatorRenderTexture.Get(), fd->pipelineStateRenderTexture.Get()));
	//	resourceStateHelper->toState(fd->renderTargetRenderTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, commandList);
	//	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(fd->rtvHeapRenderTexture->GetCPUDescriptorHandleForHeapStart(), 0, fd->rtvDescriptorSizeRenderTexture);
	//	float clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; //will correctly produce warnings CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE
	//	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	//	commandList->ClearDepthStencilView(fd->dsvHeapRenderTexture->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	//	resourceStateHelper->toState(fd->renderTargetRenderTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, commandList);
	//	ThrowIfFailed(commandList->Close());
	//	ID3D12CommandList* ppCommandLists[] = { commandList };

	//	dxGlobal->commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	//}
	dxGlobal->waitGPU(fdg, dxGlobal->commandQueue);
	{
		//if (pipeline->vr) {
		//	assert(fdg->rightCam. != nullptr);
		//}
		// TODO workaround for mem leak: only call this once:
		if (fdb->vertexBuffer == nullptr) {

			// prepare vertices:
			vector<Vertex> vertices;
			vector<Vertex>& vertexBuffer = recreateVertexBufferContent(vertices);
			size_t vertexBufferSize = sizeof(Vertex) * vertexBuffer.size();
			createAndUploadVertexBuffer(vertexBufferSize, sizeof(Vertex), &(vertexBuffer.at(0)), fdb->pipelineState.Get(),
				L"Billboard2", fdb->vertexBuffer, fdb->vertexBufferUpload, fdb->updateCommandAllocator, fdb->updateCommandList, fdb->vertexBufferView);

			// Close the command list and execute it to begin the vertex buffer copy into
			// the default heap.
			ThrowIfFailed(fdb->updateCommandList->Close());
			ID3D12CommandList* ppCommandListsUpload[] = { fdb->updateCommandList.Get() };
			dxGlobal->commandQueue->ExecuteCommandLists(_countof(ppCommandListsUpload), ppCommandListsUpload);
			dxGlobal->waitGPU(fdg, dxGlobal->commandQueue);
		}
		// prepare drawing:
		ID3D12GraphicsCommandList* commandList = fdg->commandListRenderTexture.Get();
		ThrowIfFailed(fdg->commandAllocatorRenderTexture->Reset());
		ThrowIfFailed(commandList->Reset(fdg->commandAllocatorRenderTexture.Get(), fdb->pipelineState.Get()));
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
		commandList->SetGraphicsRootSignature(fdb->rootSignature.Get());
		commandList->RSSetViewports(1, &fdg->eyes.viewports[0]);
		commandList->RSSetScissorRects(1, &fdg->eyes.scissorRects[0]);

		// Set CBV
		commandList->SetGraphicsRootConstantBufferView(0, fdb->cbvResource->GetGPUVirtualAddress());

		// Indicate that the back buffer will be used as a render target.
		//	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		//CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(xapp().rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, xapp().rtvDescriptorSize);
		//CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(fd->rtvHeapRenderTexture->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(fdg->dsvHeapRenderTexture->GetCPUDescriptorHandleForHeapStart());
		fdg->commandListRenderTexture->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		ID3D12Resource* resource = fdg->renderTarget.Get();
		//if (!xapp().ovrRendering) resource = xapp().renderTargets[frameIndex].Get();
		//else resource = xapp().vr.texResource[frameIndex];

		// prepare cbv:
		XMStoreFloat4x4(&cbv.wvp, fdg->leftCam.worldViewProjection());
		memcpy(fdb->cbvGPUDest, &cbv, sizeof(cbv));

		// draw
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &fdb->vertexBufferView);
		// now draw all the billboards, one draw call per texture type 
		// iterate over billboard types
		UINT cur_vertex_index = 0;
		auto d = (BillboardEffectAppData*)getActiveAppDataSet();
		for (auto& elvec : d->billboards) {
			//Log(elvec.first.c_str() << endl);
			auto tex = dxGlobal->getTextureStore()->getTexture(elvec.first);
			// Set SRV
			ID3D12DescriptorHeap* ppHeaps[] = { tex->m_srvHeap.Get() };
			fdg->commandListRenderTexture->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
			fdg->commandListRenderTexture->SetGraphicsRootDescriptorTable(1, tex->m_srvHeap->GetGPUDescriptorHandleForHeapStart());
			UINT count = (UINT)elvec.second.size() * 6;
			if (count > 0) fdg->commandListRenderTexture->DrawInstanced(count, 1, cur_vertex_index, 0);
			cur_vertex_index += count;
		}
		//Sleep(50);
		if (true && pipeline->vr) {
			// draw right eye:
			commandList->RSSetViewports(1, &fdg->eyes.viewports[1]);
			commandList->RSSetScissorRects(1, &fdg->eyes.scissorRects[1]);
			XMStoreFloat4x4(&cbv.wvp, fdg->rightCam.worldViewProjection());
			memcpy(fdb->cbvGPUDest, &cbv, sizeof(cbv)); // TODO use 2 buffers!!!!!
			// now draw all the billboards, one draw call per texture type 
			// iterate over billboard types
			UINT cur_vertex_index = 0;
			auto d = (BillboardEffectAppData*)getActiveAppDataSet();
			for (auto& elvec : d->billboards) {
				//Log(elvec.first.c_str() << endl);
				auto tex = dxGlobal->getTextureStore()->getTexture(elvec.first);
				// Set SRV
				ID3D12DescriptorHeap* ppHeaps[] = { tex->m_srvHeap.Get() };
				fdg->commandListRenderTexture->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
				fdg->commandListRenderTexture->SetGraphicsRootDescriptorTable(1, tex->m_srvHeap->GetGPUDescriptorHandleForHeapStart());
				UINT count = (UINT)elvec.second.size() * 6;
				if (count > 0) fdg->commandListRenderTexture->DrawInstanced(count, 1, cur_vertex_index, 0);
				cur_vertex_index += count;
			}
		}

		// execute commands:
		ThrowIfFailed(commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { commandList };

		dxGlobal->commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		dxGlobal->waitGPU(fdg, dxGlobal->commandQueue);
		commandList = fdg->commandListRenderTexture.Get();
		//ThrowIfFailed(fdg->commandAllocatorRenderTexture->Reset());
		//ThrowIfFailed(commandList->Reset(fdg->commandAllocatorRenderTexture.Get(), fdg->pipelineStateRenderTexture.Get()));

	}
}

vector<Billboard::Vertex>& Billboard::recreateVertexBufferContent(vector<Vertex> &vertices)
{
	if (vertices.size() > 0) return vertices;
	Vertex cur_billboard[6];
	auto d = (BillboardEffectAppData*)getActiveAppDataSet();
	for (auto& elvec : d->billboards) {
		//Log(elvec.first.c_str() << endl);
		// iterate over billboards of current type
		for (auto& bb : elvec.second) {
			//Log("billboard " << bb.pos.x << endl);
			createBillbordVertexData(cur_billboard, bb);
			for (int i = 0; i < 6; i++) {
				vertices.push_back(cur_billboard[i]);
			}
		}
	}
	return vertices;
}

// create 2 triangles to display a billboard
void Billboard::createBillbordVertexData(Vertex* cur_billboard, BillboardElement& bb) {
	// we know cur_billboard is a Vertex[6]
	// first create billboard at origin in x/y plane with correct size:
	float deltaw = bb.size.x / 2;
	float deltah = bb.size.y / 2;
	Vertex* c = cur_billboard; // use shorter name
	// low left
	c[0].pos.x = 0 - deltaw;
	c[0].pos.y = 0 - deltah;
	c[0].pos.z = 0;
	c[0].pos.w = 1;
	c[0].uv.x = 0;
	c[0].uv.y = 1;
	// top left
	c[1].pos.x = 0 - deltaw;
	c[1].pos.y = 0 + deltah;
	c[1].pos.z = 0;
	c[1].pos.w = 1;
	c[1].uv.x = 0;
	c[1].uv.y = 0;
	// low right
	c[2].pos.x = 0 + deltaw;
	c[2].pos.y = 0 - deltah;
	c[2].pos.z = 0;
	c[2].pos.w = 1;
	c[2].uv.x = 1;
	c[2].uv.y = 1;
	// 2nd triangle: copy low right
	c[3] = c[2];
	// 2nd triangle: copy top left
	c[4] = c[1];
	// 2nd triangle: top right
	c[5].pos.x = 0 + deltaw;
	c[5].pos.y = 0 + deltah;
	c[5].pos.z = 0;
	c[5].pos.w = 1;
	c[5].uv.x = 1;
	c[5].uv.y = 0;
	// now translate to real position:
	for (int i = 0; i < 6; i++) {
		c[i].pos.x += bb.pos.x;
		c[i].pos.y += bb.pos.y;
		c[i].pos.z += bb.pos.z;
	}
	// layout of vertex input: 
	// pos.x		P.x
	// pos.y		P.y
	// pos.z		P.z
	// pos.w
	if (true) {
		// now turn the whole thing into normal
		// default normal is (0,0,-1), we call it n0
		// the billboard should be turned into bb.normal (n1)
		// turn axis is perpendicular vector to n0 and n1: 
		// axis = n0 x n1
		// radians turn degree is cos t = n0 * n1 / (|n0|*|n1|)
		// 
		XMFLOAT4 n0_xm = XMFLOAT4(0.0f, 0.0f, -2.0f, 0.0f);
		XMVECTOR n0 = XMLoadFloat4(&n0_xm);
		XMVECTOR n1 = XMLoadFloat4(&bb.normal);
		XMVECTOR angle = XMVector3AngleBetweenVectors(n0, n1);
		XMVECTOR axis = XMVector3Cross(n0, n1);
		if (XMVector3Equal(axis, XMVectorZero())) {
			n0 = XMVectorSetX(n0, XMVectorGetX(n0) + 0.001f);
			angle = XMVector3AngleBetweenVectors(n0, n1);
			axis = XMVector3Cross(n0, n1);
			assert(XMVector3NotEqual(axis, XMVectorZero()));
		}
		XMMATRIX rot = XMMatrixRotationAxis(axis, XMVectorGetX(angle));
		for (int i = 0; i < 6; i++) {
			XMFLOAT4 v_xm = XMFLOAT4(c[i].pos.x, c[i].pos.y, c[i].pos.z, 0.0f);
			XMVECTOR v = XMLoadFloat4(&v_xm);
			XMVECTOR v2 = XMVector3Transform(v, rot);
			XMFLOAT3 v2_xm;
			XMStoreFloat3(&v2_xm, v2);
			c[i].pos.x = v2_xm.x;
			c[i].pos.y = v2_xm.y;
			c[i].pos.z = v2_xm.z;
		}
	}
}

// strange version that uses normal parameters for triangle calculation 
// probably left-over from old massive test
/*
void Billboard::createBillbordVertexData(Vertex* cur_billboard, BillboardElement& bb) {
	// we know cur_billboard is a Vertex[6]
	// first create billboard at origin in x/y plane with correct size:
	float deltaw = bb.size.x / 2;
	float deltah = bb.size.y / 2;
	Vertex* cleft = cur_billboard; // use shorter name
	// low left
	cleft[0].pos.x = 0 - deltaw;
	cleft[0].pos.y = 0 - deltah;
	cleft[0].pos.z = 0;
	cleft[0].pos.w = 1;
	cleft[0].uv.x = 0;
	cleft[0].uv.y = 1;
	// top left
	cleft[1].pos.x = 0 - deltaw;
	cleft[1].pos.y = 0 + deltah;
	cleft[1].pos.z = 0;
	cleft[1].pos.w = 1;
	cleft[1].uv.x = 0;
	cleft[1].uv.y = 0;
	// low right
	cleft[2].pos.x = 0 + deltaw;
	cleft[2].pos.y = 0 - deltah;
	cleft[2].pos.z = 0;
	cleft[2].pos.w = 1;
	cleft[2].uv.x = 1;
	cleft[2].uv.y = 1;
	// 2nd triangle: copy low right
	cleft[3] = cleft[2];
	// 2nd triangle: copy top left
	cleft[4] = cleft[1];
	// 2nd triangle: top right
	cleft[5].pos.x = 0 + deltaw;
	cleft[5].pos.y = 0 + deltah;
	cleft[5].pos.z = 0;
	cleft[5].pos.w = 1;
	cleft[5].uv.x = 1;
	cleft[5].uv.y = 0;
	// now translate to real position:
	for (int i = 0; i < 6; i++) {
		cleft[i].pos.x += bb.pos.x;
		cleft[i].pos.y += bb.pos.y;
		cleft[i].pos.z += bb.pos.z;
	}
	// layout of vertex input: 
	// pos.x		P.x
	// pos.y		P.y
	// pos.z		P.z
	// pos.w
	// normal.x		factor.x
	// normal.y		factor.y
	// normal.z		w/2
	// normal.w		h/2
	//XMFLOAT4 cam = xapp().camera.pos;
	for (int i = 0; i < 6; i++) {
		cleft[i].pos.x = bb.pos.x;
		cleft[i].pos.y = bb.pos.y;
		cleft[i].pos.z = bb.pos.z;
		cleft[i].normal.z = deltaw;
		cleft[i].normal.w = deltah;
	}
	cleft[0].normal.x = -1;
	cleft[0].normal.y = -1;
	cleft[1].normal.x = -1;
	cleft[1].normal.y = 1;
	cleft[2].normal.x = 1;
	cleft[2].normal.y = -1;

	cleft[3].normal.x = 1;
	cleft[3].normal.y = -1;
	cleft[4].normal.x = -1;
	cleft[4].normal.y = 1;
	cleft[5].normal.x = 1;
	cleft[5].normal.y = 1;
}
*/