#include "stdafx.h"

void PostEffect::init()
{
	auto frameCount = xapp().FrameCount;
	Log("PostEffect init for " << frameCount << " frames called" << endl);

	// create render textures for post effects:
	DXGI_SWAP_CHAIN_DESC1 scd;
	xapp().swapChain->GetDesc1(&scd);
	Log("" << scd.Format << endl);

	// try to do all expensive operations like shader loading and PSO creation here
	// Create the pipeline state, which includes compiling and loading shaders.
	{
		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = nullptr;//xapp().rootSignature.Get();  // TODO get it from shader compile later
										 //psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
										 //psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		//ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
#include "CompiledShaders/PostVS.h"
#include "CompiledShaders/PostPS.h"
		// test shade library functions
		//{
		//D3DLoadModule() uses ID3D11Module
		//ComPtr<ID3DBlob> vShader;
		//ThrowIfFailed(D3DReadFileToBlob(L"", &vShader));
		psoDesc.VS = { binShader_PostVS, sizeof(binShader_PostVS) };
		psoDesc.PS = { binShader_PostPS, sizeof(binShader_PostPS) };
		ThrowIfFailed(xapp().device->CreateRootSignature(0, binShader_PostPS, sizeof(binShader_PostPS), IID_PPV_ARGS(&rootSignature)));
		rootSignature.Get()->SetName(L"post_root_signature");
		psoDesc.pRootSignature = rootSignature.Get();
		ThrowIfFailed(xapp().device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
		pipelineState.Get()->SetName(L"state_post_init");
		ComPtr<ID3D12RootSignatureDeserializer> de;
		//D3D12CreateRootSignatureDeserializer(binShader_PostPS, sizeof(binShader_PostPS), __uuidof(ID3D12RootSignatureDeserializer), );
		ThrowIfFailed(D3D12CreateRootSignatureDeserializer(binShader_PostPS, sizeof(binShader_PostPS), IID_PPV_ARGS(&de)));
		const D3D12_ROOT_SIGNATURE_DESC *rt = de->GetRootSignatureDesc();
		Log("num samplers: " << rt->NumStaticSamplers << endl);
		// set cbv:
		// flow of control:
		// initial phase:
		// --> device->CreateCommittedResource()  (resource type is constant buffer)
		// --> ID3D12Resource::GetGPUVirtualAddress
		// --> constantBuffer->Map() to get GPUAdress to copy to from C++ code
		// --> initially memcpy the cbv data to GPU
		//
		// update/render phase:
		// -->SetGraphicsRootConstantBufferView(0, D3D12_GPU_VIRTUAL_ADDRESS); // on command list
		// -->memcpy(GPUAdress, changed constant buffer content)

		// CBV:
		//UINT cbvSize = calcConstantBufferSize(sizeof(cbv));
		//ThrowIfFailed(xapp().device->CreateCommittedResource(
		//	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		//	D3D12_HEAP_FLAG_NONE, // do not set - dx12 does this automatically depending on resource type
		//	&CD3DX12_RESOURCE_DESC::Buffer(cbvSize),
		//	D3D12_RESOURCE_STATE_GENERIC_READ,
		//	nullptr,
		//	IID_PPV_ARGS(&cbvResource)));
		//cbvResource.Get()->SetName(L"lines_cbv_resource");
		//Log("GPU virtual: " <<  cbvResource->GetGPUVirtualAddress(); << endl);
		//ThrowIfFailed(m_constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_pCbvDataBegin)));
//		cbvResource->Map(0, nullptr, reinterpret_cast<void**>(&cbvGPUDest));
		// set cbv data:
		XMMATRIX ident = XMMatrixIdentity();
//		XMStoreFloat4x4(&cbv.wvp, ident);
		//cbv.wvp._11 += 2.0f;
//		memcpy(cbvGPUDest, &cbv, sizeof(cbv));
	}
	static LPCWSTR fence_names[XApp::FrameCount] = {
		L"fence_post_0", L"fence_post_1", L"fence_post_2"
	};
	// Create command allocators and command lists for each frame.
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

	// Describe and create a shader resource view (SRV) heap for the texture.
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(xapp().device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));

	ComPtr<ID3D12Resource> textureUploadHeap;

	// Create the texture.
	{
		// Describe and create a Texture2D.
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = xapp().backbufferWidth;
		textureDesc.Height = xapp().backbufferHeight;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		ThrowIfFailed(xapp().device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_texture)));
		m_texture.Get()->SetName(L"post_copy_frame");

		//const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, 1);

		// Describe and create a SRV for the texture.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		xapp().device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	init2();
}

void PostEffect::init2()
{
	// create 2 triangles covering the whole area in device coords:
	// remember to revert y for texture because top coord is 0 !!
	Vertex all[] = {
		// 1st triangle
		{ XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 1) },
		{ XMFLOAT3(-1, 1, 0),  XMFLOAT2(0, 0) },
		{ XMFLOAT3(1, -1, 0),  XMFLOAT2(1, 1) },
		// 2nd triangle
		{ XMFLOAT3(1, -1, 0),  XMFLOAT2(1, 1) },
		{ XMFLOAT3(-1, 1, 0),  XMFLOAT2(0, 0) },
		{ XMFLOAT3(1,  1, 0),  XMFLOAT2(1, 0) }
	};
	UINT vertexBufferSize = (UINT)(sizeof(all));
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
	ThrowIfFailed(xapp().device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer)));
	vertexBuffer.Get()->SetName(L"vertexBuffer_post");

	ThrowIfFailed(xapp().device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexBufferUpload)));
	vertexBufferUpload.Get()->SetName(L"vertexBufferUpload_post");
	//Log(vertexBufferUpload->GetGPUVirtualAddress());
	// Copy data to the intermediate upload heap and then schedule a copy 
	// from the upload heap to the vertex buffer.
	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = reinterpret_cast<UINT8*>(&all);
	vertexData.RowPitch = vertexBufferSize;
	vertexData.SlicePitch = vertexData.RowPitch;

	PIXBeginEvent(commandLists[frameIndex].Get(), 0, L"posteffect: update vertex buffer for postEffect");
	commandLists[frameIndex].Get()->Reset(commandAllocators[frameIndex].Get(), pipelineState.Get());
	UpdateSubresources<1>(commandLists[frameIndex].Get(), vertexBuffer.Get(), vertexBufferUpload.Get(), 0, 0, 1, &vertexData);
	commandLists[frameIndex]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	PIXEndEvent(commandLists[frameIndex].Get());

	// Initialize the vertex buffer view.
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = vertexBufferSize;
	// Close the command list and execute it to begin the vertex buffer copy into
	// the default heap.

	// test textures:
	TextureLoadResult result;
	xapp().textureStore.loadTexture(L"dirt6_markings.dds", "default", commandLists[frameIndex].Get(), result);
	// end test textures

	ThrowIfFailed(commandLists[frameIndex]->Close());
	ID3D12CommandList* ppCommandLists[] = { commandLists[frameIndex].Get() };
	xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	auto &f = frameData[frameIndex];
	createSyncPoint(f, xapp().commandQueue);
	waitForSyncPoint(f);
	result.UploadBuffer->Release();
}

void PostEffect::preDraw() {
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	ThrowIfFailed(commandAllocators[frameIndex]->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(commandLists[frameIndex]->Reset(commandAllocators[frameIndex].Get(), pipelineState.Get()));

	// Indicate that the back buffer will now be used as pixel shader input.
	ID3D12Resource *resource = xapp().renderTargets[frameIndex].Get();
	D3D12_RESOURCE_DESC rDesc = resource->GetDesc();
	//commandLists[frameIndex]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
	//	D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE));
	commandLists[frameIndex]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE));

	ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
	TextureInfo *tex = xapp().textureStore.getTexture("default");
	if (tex->available) {
		ppHeaps[0] = tex->m_srvHeap.Get();
	}
	// Set necessary state.
	commandLists[frameIndex]->SetGraphicsRootSignature(rootSignature.Get());
	commandLists[frameIndex]->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	commandLists[frameIndex]->RSSetViewports(1, &xapp().viewport);
	commandLists[frameIndex]->RSSetScissorRects(1, &xapp().scissorRect);

	// Set SRV
	commandLists[frameIndex]->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());

	// Indicate that the back buffer will be used as a render target.
	//	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(xapp().rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, xapp().rtvDescriptorSize);
	commandLists[frameIndex]->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
}

void PostEffect::draw()
{
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
	preDraw();
	commandLists[frameIndex]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D12Resource *frame = xapp().renderTargets[frameIndex].Get();
	CD3DX12_TEXTURE_COPY_LOCATION Dst(m_texture.Get(), 0);
	CD3DX12_TEXTURE_COPY_LOCATION Src(frame, 0);
	commandLists[frameIndex]->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
	//	commandLists[frameIndex]->IASetVertexBuffers(0, 1, &vertexBufferView);
	//	auto numVertices = lines.size() * 2;
	//	commandLists[frameIndex]->DrawInstanced((UINT)numVertices, 1, 0, 0);
	commandLists[frameIndex]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(frame, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE));
	commandLists[frameIndex]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE));
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(xapp().rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, xapp().rtvDescriptorSize);
	const float clearColor[] = { 0.0f, 1.0f, 0.0f, 1.0f };
	commandLists[frameIndex]->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	// we now have the current frame in the texture - make draw calls to refill our rtv
	commandLists[frameIndex]->IASetVertexBuffers(0, 1, &vertexBufferView);
	UINT numVertices = 6;
	commandLists[frameIndex]->DrawInstanced(numVertices, 1, 0, 0);

	if (xapp().ovrRendering) ovrDraw();
	postDraw();
}

void PostEffect::ovrDraw() {
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
	// Note: do not transition the render target to present here.
	// the transition will occur when the wrapped 11On12 render
	// target resource is released.

	//ThrowIfFailed(commandLists[frameIndex]->Close());
	//// Execute the command list.
	//ID3D12CommandList* ppCommandLists[] = { commandLists[frameIndex].Get() };
	//xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//xapp().d3d11On12Device->AcquireWrappedResources(xapp().wrappedBackBuffers[frameIndex].GetAddressOf(), 1);
	//xapp().d3d11DeviceContext->CopyResource(xapp().wrappedTextures[frameIndex].Get(), xapp().wrappedBackBuffers[frameIndex].Get());
	//xapp().d3d11On12Device->ReleaseWrappedResources(xapp().wrappedBackBuffers[frameIndex].GetAddressOf(), 1);
	//xapp().d3d11DeviceContext->Flush();

	xapp().vr.submitFrame();

	//rDesc = resource->GetDesc();
}

void PostEffect::postDraw() {
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
	// Note: do not transition the render target to present here.
	// the transition will occur when the wrapped 11On12 render
	// target resource is released.

	// Indicate that the back buffer will now be used to present.
	ID3D12Resource *resource = xapp().renderTargets[frameIndex].Get();
	D3D12_RESOURCE_DESC rDesc = resource->GetDesc();
	//commandLists[frameIndex]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PRESENT,
	//	D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE));
	commandLists[frameIndex]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE));
	// Record commands.
	ThrowIfFailed(commandLists[frameIndex]->Close());
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { commandLists[frameIndex].Get() };
	xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	//Sleep(50);
	rDesc = resource->GetDesc();
}
