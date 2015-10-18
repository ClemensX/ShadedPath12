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
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
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
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		//ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
#include "CompiledShaders/LineVS.h"
#include "CompiledShaders/LinePS.h"
		// test shade library functions
		//{
		//D3DLoadModule() uses ID3D11Module
		//ComPtr<ID3DBlob> vShader;
		//ThrowIfFailed(D3DReadFileToBlob(L"", &vShader));
		psoDesc.VS = { binShader_LineVS, sizeof(binShader_LineVS) };
		psoDesc.PS = { binShader_LinePS, sizeof(binShader_LinePS) };
		ThrowIfFailed(xapp().device->CreateRootSignature(0, binShader_LinePS, sizeof(binShader_LinePS), IID_PPV_ARGS(&rootSignature)));
		rootSignature.Get()->SetName(L"lines_root_signature");
		psoDesc.pRootSignature = rootSignature.Get();
		ThrowIfFailed(xapp().device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
		pipelineState.Get()->SetName(L"state_lines_init");

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
	// Create command allocators and command lists for each frame.
	for (UINT n = 0; n < XApp::FrameCount; n++)
	{
		ThrowIfFailed(xapp().device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[n])));
		ThrowIfFailed(xapp().device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[n].Get(), pipelineState.Get(), IID_PPV_ARGS(&commandLists[n])));
		// Command lists are created in the recording state, but there is nothing
		// to record yet. The main loop expects it to be closed, so close it now.
		ThrowIfFailed(commandLists[n]->Close());
	}
}

void PostEffect::draw()
{
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
	preDraw();
	commandLists[frameIndex]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
//	commandLists[frameIndex]->IASetVertexBuffers(0, 1, &vertexBufferView);
//	auto numVertices = lines.size() * 2;
//	commandLists[frameIndex]->DrawInstanced((UINT)numVertices, 1, 0, 0);
	postDraw();
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

	// Set necessary state.
	commandLists[frameIndex]->SetGraphicsRootSignature(rootSignature.Get());
	commandLists[frameIndex]->RSSetViewports(1, &xapp().viewport);
	commandLists[frameIndex]->RSSetScissorRects(1, &xapp().scissorRect);

	// Set CBV
//	commandLists[frameIndex]->SetGraphicsRootConstantBufferView(0, cbvResource->GetGPUVirtualAddress());

	// Indicate that the back buffer will be used as a render target.
	//	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(xapp().rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, xapp().rtvDescriptorSize);
	commandLists[frameIndex]->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands.
	const float clearColor[] = { 1.0f, 0.2f, 0.4f, 1.0f };
	//commandLists[frameIndex]->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

void PostEffect::postDraw() {
	UINT frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();
	// Note: do not transition the render target to present here.
	// the transition will occur when the wrapped 11On12 render
	// target resource is released.

	// Indicate that the back buffer will now be used to present.
	ID3D12Resource *resource = xapp().renderTargets[frameIndex].Get();
	D3D12_RESOURCE_DESC rDesc = resource->GetDesc();
	commandLists[frameIndex]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE));
	commandLists[frameIndex]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE));
	ThrowIfFailed(commandLists[frameIndex]->Close());
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { commandLists[frameIndex].Get() };
	xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	rDesc = resource->GetDesc();
}
