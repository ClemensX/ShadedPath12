#include "stdafx.h"

BillboardElement& Billboard::get(string texture_id, int order_num, unsigned long& user) {
	auto& billboards = getInactiveAppDataSet(user)->billboards;
	auto& v = billboards[texture_id];
	return v.at(order_num);
}

size_t Billboard::add(string texture_id, BillboardElement billboardEl, unsigned long& user) {
	auto& billboards = getInactiveAppDataSet(user)->billboards;
	auto& v = billboards[texture_id];
	v.push_back(billboardEl);
	return v.size() - 1;
}

#include "CompiledShaders/BillboardVS.h"
#include "CompiledShaders/BillboardPS.h"

void Billboard::init(DXGlobal* a, FrameDataBillboard* fdb, FrameDataGeneral* fd_general_, Pipeline* pipeline) {
	if (!initialized) {
		updateQueue.init();
		effectDataUpdate.init(&appDataSets[0]);
#if !defined(DISABLE_UPDATE_THREADS)
		// update thread
		void* native_handle = pipeline->getThreadGroup()->add_t(runUpdate, pipeline, (Effect*)this);
		wstring mod_name = wstring(L"update_billboard");//.append(L"_").append(to_wstring(i));
		SetThreadDescription((HANDLE)native_handle, mod_name.c_str());
#endif
		DXGlobal::initSyncPoint(&updateFenceData, a->device);
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
			ThrowIfFailed(a->device->CreateRootSignature(0, binShader_BillboardVS, sizeof(binShader_BillboardVS), IID_PPV_ARGS(&rootSignature)));
			//rootSignature.Get()->SetName(L"Linetext_root_signature");
			psoDesc.pRootSignature = rootSignature.Get();
			ThrowIfFailed(a->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
			pipelineState.Get()->SetName(L"state_billboard_init");

			// init resources for update thread:
			ThrowIfFailed(a->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&updateCommandAllocator)));
			ThrowIfFailed(a->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, updateCommandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&updateCommandList)));
			// Command lists are created in the recording state, but there is nothing
			// to record yet. The main loop expects it to be closed, so close it now.
			ThrowIfFailed(updateCommandList->Close());
			// init fences: --> use from FrameDataGeneral
		}
	}
	initialized = true;
	dxGlobal = a;

	// Create command allocators and command lists for each frame.
	//static LPCWSTR fence_names[DXManager::FrameCount] = {
	//	L"fence_Billboard_0", L"fence_Billboard_1", L"fence_Billboard_2"
	//};
	createConstantBuffer((UINT) sizeof(cbv), L"Billboard_cbv_resource", fdb);
	// set cbv data:
	XMMATRIX ident = XMMatrixIdentity();
	XMStoreFloat4x4(&cbv.wvp, ident);
	memcpy(fdb->cbvGPUDest, &cbv, sizeof(cbv));

}

// make inactive app data set active and vice versa
// synchronized
void Billboard::activateAppDataSet(unsigned long user)
{
	//unique_lock<mutex> lock(dataSetMutex);
	auto bea = (BillboardEffectAppData*)getInactiveAppDataSet(user);
	if (/*bea->vertexBuffer == nullptr &&*/ !dxGlobal->pipeline->isShutdown()) {
		//Error(L"vertex buffer not initialized in billboard.draw(). Cannot continue.");
		// prepare vertices:
		vector<Vertex> vertices;
		vector<Vertex>& vertexBuffer = recreateVertexBufferContent(vertices, bea);
		size_t vertexBufferSize = sizeof(Vertex) * vertexBuffer.size();
		//Log(" upload billboard vertex buffer, size " << vertexBufferSize << endl);
		// upload changed data
		BufferResource* res = resourceStore.getSlot();
		bea->bufferResource = res;
		createAndUploadVertexBuffer(vertexBufferSize, sizeof(Vertex), &(vertexBuffer.at(0)), pipelineState.Get(),
			L"Billboard2", res->vertexBuffer, res->vertexBufferUpload, updateCommandAllocator, updateCommandList, res->vertexBufferView);

		// Close the command list and execute it to begin the vertex buffer copy into
		// the default heap.
		ThrowIfFailed(updateCommandList->Close());
		ID3D12CommandList* ppCommandListsUpload[] = { updateCommandList.Get() };
		dxGlobal->commandQueue->ExecuteCommandLists(_countof(ppCommandListsUpload), ppCommandListsUpload);
		dxGlobal->createSyncPoint(&updateFenceData, dxGlobal->commandQueue);
		dxGlobal->waitForSyncPoint(&updateFenceData);
		resourceStore.freeUnusedSlots(res->generation-1);
	}
	// switch inactive and active data sets:
	currentActiveAppDataSet = (currentActiveAppDataSet + 1) % 2;
	currentInactiveAppDataSet = (currentInactiveAppDataSet + 1) % 2;
}

void Billboard::draw(Frame* frame, FrameDataGeneral* fdg, FrameDataBillboard* fdb, Pipeline* pipeline)
{
	// DON'T DO IT:
	//unique_lock<mutex> lock(dataSetMutex);
	//Log("draw " << endl);
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
		commandList->SetGraphicsRootConstantBufferView(0, fdb->cbvResource->GetGPUVirtualAddress());

		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(fdg->dsvHeapRenderTexture->GetCPUDescriptorHandleForHeapStart());
		fdg->commandListRenderTexture->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		ID3D12Resource* resource = fdg->renderTarget.Get();

		// prepare cbv:
		if (pipeline->isHMD()) {
#if defined(_SVR_)
			//pipeline->getVR()->UpdateHMDMatrixPose(); // cam == null means no cam movement with keyboard
			Matrix4 wvp = pipeline->getVR()->GetCurrentViewProjectionMatrix(vr::Eye_Left);
			memcpy(&cbv.wvp, &wvp, sizeof(cbv.wvp));
			memcpy(fdb->cbvGPUDest, &cbv, sizeof(cbv));
#endif
		} else {
			XMStoreFloat4x4(&cbv.wvp, fdg->leftCam.worldViewProjection());
			memcpy(fdb->cbvGPUDest, &cbv, sizeof(cbv));
		}
		//Log("size my wvp: " << sizeof(cbv.wvp) << endl);
		//Log("size Steam wvp: " << sizeof(Matrix4) << endl);
		//assert(sizeof(cbv.wvp) == sizeof(Matrix4));

		// draw
		auto d = (BillboardEffectAppData*)getActiveAppDataSet();
		// multi thread access problem:
		//if (d->billboards.size() != 13) {
		//	Error(L"mult thread access problem");
		//}
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &d->bufferResource->vertexBufferView);
		// now draw all the billboards, one draw call per texture type 
		// iterate over billboard types
		UINT cur_vertex_index = 0;
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
		if (true && pipeline->isVR()) {
			commandList->SetGraphicsRootConstantBufferView(0, fdb->cbvResource2->GetGPUVirtualAddress());
			// draw right eye:
			commandList->RSSetViewports(1, &fdg->eyes.viewports[1]);
			commandList->RSSetScissorRects(1, &fdg->eyes.scissorRects[1]);
			if (pipeline->isHMD()) {
#if defined(_SVR_)
				Matrix4 wvp = pipeline->getVR()->GetCurrentViewProjectionMatrix(vr::Eye_Right);
				memcpy(&cbv.wvp, &wvp, sizeof(cbv.wvp));
				memcpy(fdb->cbvGPUDest2, &cbv, sizeof(cbv));
#endif
			} else {
				XMStoreFloat4x4(&cbv.wvp, fdg->rightCam.worldViewProjection());
				memcpy(fdb->cbvGPUDest2, &cbv, sizeof(cbv));
			}
			// now draw all the billboards, one draw call per texture type 
			// iterate over billboard types
			UINT cur_vertex_index = 0;
			//auto d = (BillboardEffectAppData*)getActiveAppDataSet();
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
		releaseActiveAppDataSet(d);
	}
}

vector<Billboard::Vertex>& Billboard::recreateVertexBufferContent(vector<Vertex> &vertices, BillboardEffectAppData *bea)
{
	if (vertices.size() > 0) return vertices;
	Vertex cur_billboard[6];
	for (auto& elvec : bea->billboards) {
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


