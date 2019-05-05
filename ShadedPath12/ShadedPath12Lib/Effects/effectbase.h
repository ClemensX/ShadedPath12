class GlobalEffect;
class EffectBase {
protected:
	//HANDLE fenceEvent;
	//ComPtr<ID3D12Fence> fence;
	//UINT64 fenceValues[XApp::FrameCount];
	//void WaitForGpu();
	// fences for update
	FrameResourceSimple frameData[XApp::FrameCount];
public:
	static void createSyncPoint(FrameResourceSimple &f, ComPtr<ID3D12CommandQueue> queue);
	static void waitForSyncPoint(FrameResourceSimple &f);
	EffectFrameResource * getFrameResource(int i) { return &effectFrameResources.at(i); }
	virtual int neededCommandSlots() = 0;
	void init(GlobalEffect *globalEffect);
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
	// if createGPU_RW true another set of buffers will be created for GPU internal operations like in compute shaders
	void setSingleCBVMode(UINT maxThreads, UINT maxObjects, size_t s, wchar_t * name, bool createGPU_RW = false);
	vector<ComPtr<ID3D12Resource>> singleCBVResources;
	vector<ComPtr<ID3D12Resource>> singleCBVResourcesGPU_RW;
	vector<void*> singleMemResources;
	//ComPtr<ID3D12Resource> singleCBVResources[XApp::FrameCount*2]; // TODO
	vector<UINT8*> singleCBV_GPUDests;
	//UINT8* singleCBV_GPUDests[XApp::FrameCount*4];  // memcpy() changed cbv data to this address before draw()
	UINT slotSize = 0;  // allocated size for single CVB element
	D3D12_GPU_VIRTUAL_ADDRESS getCBVVirtualAddress(int frame, int thread, UINT objectIndex, int eyeNum);
	UINT8* getCBVUploadAddress(int frame, int thread, UINT objectIndex, int eyeNum);
	UINT8* getMemUploadAddress(int frame, int thread, UINT objectIndex, int eyeNum);


	// vertex buffer
	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> vertexBufferUpload;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
protected:
	bool singleCbvBufferMode = false;
	UINT maxObjects = 0;	// max number of entities allowed in this effect
	vector<thread> workerThreads;
	void waitForWorkerThreads();
	//VR_Eyes vr_eyes;
	// initialize next ovr draw by copying globals from VR class to vr_eyes
	//void prepareDraw(VR *vr);
	virtual ~EffectBase();
	bool initialized = false;  // set to true in init(). All effects that need to do something in destructor should check if effect was used at all...
	ResourceStateHelper *resourceStateHelper = ResourceStateHelper::getResourceStateHelper();
	XApp * xapp = nullptr;
	DXManager *dxmanager = nullptr;
	vector<EffectFrameResource> effectFrameResources;
	virtual void initFrameResource(EffectFrameResource *effectFrameResource, int frameIndex) = 0;
	// init frame ressources for this effect, calls back to effect class for intitializing the fields
	void initFrameResources() {
		for (int i = 0; i < XApp::FrameCount; i++) {
			EffectFrameResource effectFrameResource;
			initFrameResource(&effectFrameResource, i);
			effectFrameResources.push_back(effectFrameResource);
			assert(effectFrameResources.size() == i + 1);
		}
	}
public:
};

class WorkerGlobalCopyTextureCommand : public WorkerCommand {
public:
	void perform();
	ResourceStateHelper *resourceStateHelper = nullptr;
	AppWindowFrameResource *appFrameResource;
};

// global effect, render target for all other effects
class GlobalEffect : public EffectBase {
	// Inherited via EffectBase
	virtual void initFrameResource(EffectFrameResource * effectFrameResource, int frameIndex) override;
public:
	void init();
	void endInitPhase();
	bool isInitPhaseEnded() { return initPhaseEnded; }
	void setThreadCount(int max);
	void draw();
	// Inherited via EffectBase
	virtual int neededCommandSlots() override {
		return 1;
	}
	void addNeededCommandSlots(int n) {	
		countNeededCommandSlots += n;
		Log("command slots needed increased to " << countNeededCommandSlots << endl);
	}
	int getNeededCommandSlots() { return countNeededCommandSlots; }
private:
	vector<WorkerGlobalCopyTextureCommand> worker;
	// counter for ALL effects:
	int countNeededCommandSlots = 0;
	bool initPhaseEnded = false;
};

// clear effect, should be first effect called by an app

class WorkerClearCommand : public WorkerCommand {
public:
	void perform();
	ResourceStateHelper *resourceStateHelper = nullptr;
};

class ClearEffect : public EffectBase {
	// Inherited via EffectBase
	virtual void initFrameResource(EffectFrameResource * effectFrameResource, int frameIndex) override;
public:
	void init(GlobalEffect *globalEffect);
	void draw();
	// Inherited via EffectBase
	virtual int neededCommandSlots() override {
		return 1;
	}
private:
	vector<WorkerClearCommand> worker;
	GlobalEffect *globalEffect;
};

