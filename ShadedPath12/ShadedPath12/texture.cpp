#include "stdafx.h"

void TextureStore::init() {
	// Create an empty root signature.
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(xapp().device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
	}
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = rootSignature.Get();
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
//#include "CompiledShaders/PostPS.h"
	// test shade library functions
	//{
	//D3DLoadModule() uses ID3D11Module
	//ComPtr<ID3DBlob> vShader;
	//ThrowIfFailed(D3DReadFileToBlob(L"", &vShader));
	psoDesc.VS = { binShader_PostVS, sizeof(binShader_PostVS) };
	ThrowIfFailed(xapp().device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
	ThrowIfFailed(xapp().device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
	ThrowIfFailed(xapp().device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&commandList)));
	ThrowIfFailed(xapp().device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&updateFrameData.fence)));
	updateFrameData.fence->SetName(L"fence_texture_update");
	updateFrameData.fenceValue = 0;
	updateFrameData.fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (updateFrameData.fenceEvent == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}

TextureInfo* TextureStore::getTexture(string id)
{
	TextureInfo *ret = &textures[id];
	// simple validity check for now:
	if (ret->id.size() > 0) {
		// if there is no id the texture could not be loaded (wrong filename?)
		ret->available = true;
	}
	else {
		ret->available = false;
	}
	return ret;
}

void TextureStore::loadTexture(wstring filename, string id)
{
	TextureLoadResult result;
	TextureInfo initialTexture;  // only use to initialize struct in texture store - do not access this after assignment to store
	vector<byte> file_buffer;

	initialTexture.id = id;
	textures[id] = initialTexture;
	TextureInfo *texture = &textures[id];

	// find texture file, look in pak file first:
	PakEntry *pakFileEntry = nullptr;
	pakFileEntry = xapp().findFileInPak(filename.c_str());
	// try file system if not found in pak:
	initialTexture.filename = filename; // TODO check: field not needed? only in this method? --> remove
	if (pakFileEntry == nullptr) {
		wstring binFile = xapp().findFile(filename.c_str(), XApp::TEXTURE);
		texture->filename = binFile;
		//initialTexture.filename = binFile;
		xapp().readFile(texture->filename.c_str(), file_buffer, XApp::FileCategory::TEXTURE);
	} else {
		xapp().readFile(pakFileEntry, file_buffer, XApp::FileCategory::TEXTURE);
	}


	ID3D12GraphicsCommandList *commandList = this->commandList.Get();
	//D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	// create heap - TODO adjust for one heap for multiple textures
	// Describe and create a shader resource view (SRV) heap for the texture.
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(xapp().device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&texture->m_srvHeap)));
	NAME_D3D12_OBJECT(texture->m_srvHeap);

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(texture->m_srvHeap->GetCPUDescriptorHandleForHeapStart());
	//CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CreateDDSTextureFromMemory(
		xapp().device.Get(),
		&file_buffer[0],
		file_buffer.size(),
		0,
		true,
		&texture->texSRV,
		srvHandle,
		result
	);
	//CreateDDSTextureFromFile(
	//	xapp().device.Get(),
	//	texture->filename.c_str(),
	//	0,
	//	true,
	//	&texture->texSRV,
	//	srvHandle,
	//	result
	//	);

	// upload texture to GPU:
	//ID3D12Resource* UploadBuffer;

	UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture->texSRV.Get(), 0, result.NumSubresources);

	//CommandContext& InitContext = CommandContext::Begin();

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC BufferDesc;
	BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	BufferDesc.Alignment = 0;
	BufferDesc.Width = uploadBufferSize;
	BufferDesc.Height = 1;
	BufferDesc.DepthOrArraySize = 1;
	BufferDesc.MipLevels = 1;
	BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	BufferDesc.SampleDesc.Count = 1;
	BufferDesc.SampleDesc.Quality = 0;
	BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(xapp().device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
		&BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&result.UploadBuffer)));

	// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	//InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture->texSRV.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources(commandList, texture->texSRV.Get(), result.UploadBuffer, 0, 0, result.NumSubresources, result.initData.get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture->texSRV.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	//InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

	// Execute the command list and wait for it to finish so we can release the upload buffer
	//InitContext.CloseAndExecute(true);

	//UploadBuffer->Release();
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { this->commandList.Get() };
	xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	//Sleep(300);
	EffectBase::createSyncPoint(updateFrameData, xapp().commandQueue);
	EffectBase::waitForSyncPoint(updateFrameData);

	//auto &f = frameData[frameIndex];
	//createSyncPoint(f, xapp().commandQueue);
	//waitForSyncPoint(f);
	result.UploadBuffer->Release();
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pipelineState.Get()));

	// create the descriptor table to bind before draw:
	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(texture->m_srvHeap->GetGPUDescriptorHandleForHeapStart());
	texture->descriptorTable = tex;
}
