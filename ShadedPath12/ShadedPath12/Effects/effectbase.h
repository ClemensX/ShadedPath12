
class ResourceStateInfo {
public:
	D3D12_RESOURCE_STATES current_state;
};

class ResourceStateHelper {
public:
	//singleton:
	static ResourceStateHelper *getResourceStateHelper() {
		static ResourceStateHelper singleton;
		return &singleton;
	};
	void add(ID3D12Resource* res, D3D12_RESOURCE_STATES state) {
		assert(resourceStates.count(res) == 0);
		ResourceStateInfo si;
		si.current_state = state;
		resourceStates[res] = si;
	};
	void toState(ID3D12Resource* res, D3D12_RESOURCE_STATES state, ID3D12GraphicsCommandList *cl) {
		assert(resourceStates.count(res) > 0);
		ResourceStateInfo &resourceStateInfo = resourceStates.at(res);
		if (resourceStateInfo.current_state != state) {
			cl->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(res, resourceStateInfo.current_state, state));
			resourceStateInfo.current_state = state;
		}
	};
private:
	unordered_map<ID3D12Resource*, ResourceStateInfo> resourceStates;
	ResourceStateHelper() {};										// prevent creation outside this class
	ResourceStateHelper(const ResourceStateHelper&);				// prevent creation via copy-constructor
	ResourceStateHelper & operator = (const ResourceStateHelper &);	// prevent instance copies
};

class EffectBase {
protected:
	//HANDLE fenceEvent;
	//ComPtr<ID3D12Fence> fence;
	//UINT64 fenceValues[XApp::FrameCount];
	//void WaitForGpu();
	// fences for update
	FrameResource frameData[XApp::FrameCount];
public:
	static void createSyncPoint(FrameResource &f, ComPtr<ID3D12CommandQueue> queue);
	static void waitForSyncPoint(FrameResource &f);
protected:
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
	UINT8* cbvGPUDest;  // memcpy() changed cbv data to this address before draw()

	// enable single CBV mode: one large constand buffer for all objects.
	// duplicated for each worker thread
	// max number of objects has to be given 
	// setting maxObjects to 0 disables single buffer mode
	// has to be called in init() before rendering
	void setSingleCBVMode(UINT maxThreads, UINT maxObjects, size_t s, wchar_t * name);
	vector<ComPtr<ID3D12Resource>> singleCBVResources;
	//ComPtr<ID3D12Resource> singleCBVResources[XApp::FrameCount*2]; // TODO
	vector<UINT8*> singleCBV_GPUDests;
	//UINT8* singleCBV_GPUDests[XApp::FrameCount*4];  // memcpy() changed cbv data to this address before draw()
	UINT slotSize = 0;  // allocated size for single CVB element
	D3D12_GPU_VIRTUAL_ADDRESS getCBVVirtualAddress(int frame, int thread, UINT objectIndex, int eyeNum);
	UINT8* getCBVUploadAddress(int frame, int thread, UINT objectIndex, int eyeNum);


	// vertex buffer
	static void createAndUploadVertexBuffer(size_t bufferSize, size_t vertexSize, void *data, ID3D12PipelineState *pipelineState, LPCWSTR baseName,
		ComPtr<ID3D12Resource> &vertexBuffer,
		ComPtr<ID3D12Resource> &vertexBufferUpload,
		ComPtr<ID3D12CommandAllocator> &commandAllocator,
		ComPtr<ID3D12GraphicsCommandList> &commandList,
		D3D12_VERTEX_BUFFER_VIEW &vertexBufferView
		);
	// index buffer
	static void createAndUploadIndexBuffer(size_t bufferSize, void *data, ID3D12PipelineState *pipelineState, LPCWSTR baseName,
		ComPtr<ID3D12Resource> &indexBuffer,
		ComPtr<ID3D12Resource> &indexBufferUpload,
		ComPtr<ID3D12CommandAllocator> &commandAllocator,
		ComPtr<ID3D12GraphicsCommandList> &commandList,
		D3D12_INDEX_BUFFER_VIEW &indexBufferView
		);
	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> vertexBufferUpload;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
protected:
	bool singleCbvBufferMode = false;
	UINT maxObjects = 0;	// max number of entities allowed in this effect
	vector<thread> workerThreads;
	void waitForWorkerThreads();
	VR_Eyes vr_eyes;
	// initialize next ovr draw by copying globals from VR class to vr_eyes
	void prepareDraw(VR *vr);
	virtual ~EffectBase();
	bool initialized = false;  // set to true in init(). All effects that need to do something in destructor should check if effect was used at all...
	ResourceStateHelper *resourceStateHelper = ResourceStateHelper::getResourceStateHelper();
public:
};
