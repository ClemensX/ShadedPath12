#include "stdafx.h"

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

void TextureStore::loadTexture(wstring filename, string id, ID3D12GraphicsCommandList *commandList, TextureLoadResult &result)
{
	wstring binFile = xapp().findFile(filename.c_str(), XApp::TEXTURE);
	TextureInfo initialTexture;  // only use to initialize struct in texture store - do not access this after assignment to store
	initialTexture.filename = binFile;
	initialTexture.id = id;
	textures[id] = initialTexture;
	TextureInfo *texture = &textures[id];

	//D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	// create heap - TODO adjust for one heap for multiple textures
	// Describe and create a shader resource view (SRV) heap for the texture.
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(xapp().device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&texture->m_srvHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(texture->m_srvHeap->GetCPUDescriptorHandleForHeapStart());
	//CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	CreateDDSTextureFromFile(
		xapp().device.Get(),
		texture->filename.c_str(),
		0,
		true,
		&texture->texSRV,
		srvHandle,
		result
		);

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
}
