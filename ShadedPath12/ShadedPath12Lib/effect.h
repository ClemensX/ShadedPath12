// base class for App Data
class EffectAppData {
public:
	virtual ~EffectAppData() = 0 {}; // still need to provide an (empty) base class destructor implementation even for pure virtual destructors
};

// base class for Per Frame Data
class EffectFrameData {
public:
	virtual ~EffectFrameData() = 0 {}; // still need to provide an (empty) base class destructor implementation even for pure virtual destructors
};

struct FrameDataBase {
public:
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12CommandAllocator> updateCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> updateCommandList;
	ComPtr<ID3D12Resource> cbvResource;
	ComPtr<ID3D12Resource> cbvResource2;
	UINT8* cbvGPUDest;  // memcpy() changed cbv data to this address before draw()
	UINT8* cbvGPUDest2;  // memcpy() changed cbv data to this address before draw()
	long long frameNumOfDataActivation; // ???

	friend class DXGlobal;
};

/*
 Update effect date:
 1) app gets inactive data set IDS
 2) app updates IDS (TODO: in app thread? effect thread?) TODO: what about updates while IDS is inuse?
 3) app calls activateAppDataSet()
 4) Inactive GPU ressources are updated, render code still sees old data
 5) after finished update of GPU we have to start transition to new data set for render code:
 5a) There will be render code running still - old data cannot be changed right away
 5b) new effect render code will get new data set
 5c) after all threads have switched to new data, the old data is free again - it will be used to update for next cycle
 --> idea: usage counter for active/inactive data set?
 --> one update thread per effect: may work on one update for arbitrary duration, only one more will be queued
     if another update comes while one is already queued, the queued one will simply be replaced
 */

// base class for effects

class Effect {
	// effects need 3 kinds of data:
	// 1) Global data for the effect, only needed once. Like PipelineState and RootSignature
	// 2) Application data thats resembles the application state with regards for an effect. Like position and normal for billboard elements
	//    Needs to be there twice: there is alway an acive data set used for rendering, the inacive set can be manipulated by the application until it is uploaded to GPU and made active
	// 3) Per Frame data. Usually 3 sets. Frame specific resources the effect needs to run in parallel, like CBVs with per frame WVP matrix

public:
	// get inactive data set. It can be changed by effect or app code
	virtual EffectAppData* getInactiveAppDataSet() = 0;
	// get data set that is currently used by render code, should never be changed
	virtual EffectAppData* getActiveAppDataSet() = 0;
	// activate the curerently inactive data set. Includes uploading to GPU
	// rendering after this call returns will use the new data set. 
	// returns nullptr if there is no active set yet
	virtual void activateAppDataSet() = 0;
	void update(EffectAppData* data);
	// update thread runs this method:
	static void runUpdate(Pipeline* pipeline);

	virtual ~Effect() = 0 {}; // still need to provide an (empty) base class destructor implementation even for pure virtual destructors
	//function<void(Frame*, Pipeline*)> updater = nullptr;
	//void setFinishedFrameConsumer(function<void(Frame*, Pipeline*)> consumer) { this->consumer = consumer; }
protected:
	bool initialized = false;  // set to true in init(). All effects that need to do something in destructor should check if effect was used at all...
	DXManager dxmanager;
	ResourceStateHelper* resourceStateHelper = ResourceStateHelper::getResourceStateHelper();
	DXGlobal* dxGlobal;
	VR_Eyes eyes;
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

	void createConstantBuffer(size_t s, LPCWSTR name, FrameDataBase *frameData);

	// vertex buffer
	void createAndUploadVertexBuffer(size_t bufferSize, size_t vertexSize, void* data, ID3D12PipelineState* pipelineState, LPCWSTR baseName,
		ComPtr<ID3D12Resource>& vertexBuffer,
		ComPtr<ID3D12Resource>& vertexBufferUpload,
		ComPtr<ID3D12CommandAllocator>& commandAllocator,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		D3D12_VERTEX_BUFFER_VIEW& vertexBufferView
	);
	// index buffer
	void createAndUploadIndexBuffer(size_t bufferSize, void* data, ID3D12PipelineState* pipelineState, LPCWSTR baseName,
		ComPtr<ID3D12Resource>& indexBuffer,
		ComPtr<ID3D12Resource>& indexBufferUpload,
		ComPtr<ID3D12CommandAllocator>& commandAllocator,
		ComPtr<ID3D12GraphicsCommandList>& commandList,
		D3D12_INDEX_BUFFER_VIEW& indexBufferView
	);
	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> vertexBufferUpload;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
};