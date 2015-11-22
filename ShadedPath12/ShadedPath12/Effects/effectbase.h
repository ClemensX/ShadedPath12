struct FrameResource {
	HANDLE fenceEvent;
	ComPtr<ID3D12Fence> fence;
	UINT64 fenceValue;
};

class EffectBase {
protected:
	//HANDLE fenceEvent;
	//ComPtr<ID3D12Fence> fence;
	//UINT64 fenceValues[XApp::FrameCount];
	//void WaitForGpu();
	// fences for update
	FrameResource frameData[XApp::FrameCount];
	void createSyncPoint(FrameResource &f, ComPtr<ID3D12CommandQueue> queue);
	void waitForSyncPoint(FrameResource &f);

	ComPtr<ID3D12CommandAllocator> commandAllocators[XApp::FrameCount];
	ComPtr<ID3D12GraphicsCommandList> commandLists[XApp::FrameCount];

	// constant buffers
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

	void createConstantBuffer(size_t s, wchar_t * name);
	ComPtr<ID3D12Resource> cbvResource;
	UINT8* cbvGPUDest;  // mempy changed cbv data to this address before draw()

	// vertex buffer
	void createAndUploadVertexBuffer(size_t bufferSize, size_t vertexSize, void *data, ID3D12PipelineState *pipelineState, LPCWSTR baseName);
	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> vertexBufferUpload;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
};