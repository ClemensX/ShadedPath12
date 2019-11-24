// buffer handling
struct BufferResource {
	size_t bufferSize = 0;
	size_t vertexSize = 0;
	void* data = nullptr;
	ID3D12PipelineState* pipelineState = nullptr;
	LPCWSTR baseName = nullptr;
	ComPtr<ID3D12Resource> vertexBuffer = nullptr;
	ComPtr<ID3D12Resource> vertexBufferUpload = nullptr;
	ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	bool is_free = true;
	atomic<int> useCounter;
	int generation;
	BufferResource(const BufferResource&) {
		// nothing to do
	}
	BufferResource() {
		// nothing to do
	}
};

class ResourceStore {
public:
	//singleton:
	static ResourceStore* getInstance() {
		static ResourceStore singleton;
		return &singleton;
	};

	// thread save public methods:

	// find free slot, create one if necessary 
	BufferResource* getSlot() {
		unique_lock<mutex> lock(monitorMutex);
		static int generation = 0;
		BufferResource* res = findFreeSlot();
		if (res == nullptr) {
			// create new slot
			BufferResource r;
			resourceList.push_back(r);
			res = findFreeSlot();
			assert(res != nullptr);
		}
		res->is_free = false;
		res->useCounter = 0;
		res->generation = generation++;
		return res;
	}

	// return all free slots (useCount == 0) with given generation or lower
	void freeUnusedSlots(int maxGeneration) {
		unique_lock<mutex> lock(monitorMutex);
		for (auto& res : resourceList) {
			if (!res.is_free && res.useCounter == 0 && res.generation <= maxGeneration) {
				if (res.vertexBuffer != nullptr) {
					res.vertexBufferView.BufferLocation = 0;
					res.vertexBufferView.SizeInBytes= 0;
					res.vertexBufferView.StrideInBytes = 0;
					res.vertexBufferUpload->Release();
					res.vertexBufferUpload = nullptr;
					res.vertexBuffer->Release();
					res.vertexBuffer = nullptr;
					res.is_free = true;
					//Log("freed buffer slot: gen " << res.generation << " count " << res.useCounter << endl);
				}

			}
		}
	}

	//// return slot to pool. Slots are never destroyed and can be reused after returning
	//void returnSlot(BufferResource* res) {
	//	unique_lock<mutex> lock(monitorMutex);
	//	res->is_free = true;
	//}
private:

	// find next free slot - null if none is available
	// has to be synced externally
	BufferResource* findFreeSlot() {
		for (auto& res : resourceList) {
			if (res.is_free) {
				return &res;
			}
		}
		return nullptr;
	}

	mutex monitorMutex;
	list<BufferResource> resourceList;
	ResourceStore() {};									// prevent creation outside this class
	ResourceStore(const ResourceStore&);				// prevent creation via copy-constructor
	ResourceStore& operator = (const ResourceStore&);	// prevent instance copies
};


// base class for App Data
// frame independent data like certex buffers
class EffectAppData {
public:
	virtual ~EffectAppData() = 0 {}; // still need to provide an (empty) base class destructor implementation even for pure virtual destructors
};

// base class for Per Frame Data
class EffectFrameData {
public:
	virtual ~EffectFrameData() = 0 {}; // still need to provide an (empty) base class destructor implementation even for pure virtual destructors
};

// frame dependent data like CBV for MVP matrix
struct FrameDataBase {
public:
	ComPtr<ID3D12PipelineState> pipelineStateX;
	ComPtr<ID3D12RootSignature> rootSignatureX;
	ComPtr<ID3D12CommandAllocator> updateCommandAllocatorX;
	ComPtr<ID3D12GraphicsCommandList> updateCommandListX;
	ComPtr<ID3D12Resource> cbvResource;
	ComPtr<ID3D12Resource> cbvResource2;
	UINT8* cbvGPUDest;  // memcpy() changed cbv data to this address before draw()
	UINT8* cbvGPUDest2;  // memcpy() changed cbv data to this address before draw()
	long long frameNumOfDataActivation; // ???

	friend class DXGlobal;
};

// update queue for update threads (ech effect has its own):
// never queue more than one entry: if new entry arrives and there is still one unprocessed,
// the unprocessed will be replaced
class UpdateQueue {
public:
	// prepare usage:
	// init condition variable for inactive data set access
	void init() {
	};
	// wait until next data set is available
	// will only be called by update thread
	EffectAppData* pop(Pipeline* pipeline);

	// push finished data set
	void push(EffectAppData* ed, Pipeline* pipeline);

	size_t size() {
		return myqueue.size();
	}

	long finishedGen = 1;
	void waitForEffectUpdateFinish() {
		unique_lock<mutex> lock(monitorMutex_finished);
		bool cont = true;
		while (cont) {
			cv_status status = cond_finished.wait_for(lock, chrono::milliseconds(3000));
			if (status == cv_status::timeout) {
				Log("ERROR: unexpected timeout in waitForEffectUpdateFinish, gen == " << finishedGen << endl); // should not happen except during debugging
			} else {
				//Log("waitForEffectUpdateFinish, gen == " << finishedGen << endl);
				cont = false;
			}
		}
	}

	void triggerEffectUpdateFinished() {
		unique_lock<mutex> lock(monitorMutex_finished);
		cond_finished.notify_one();
		//Log("update finished gen == " << finishedGen << endl); // should not happen except during debugging
		finishedGen++;
	}

	// get inactive data set
	// only one thread can work on the inactive data set at one time, next caller will be blocked
	// while caller has this lock he should upload inactive data set to GPU
	EffectAppData* getLockedInactiveDataSet(unsigned long& user) {
		unique_lock<mutex> lock(monitorMutex_inactiveDataSet);
		while (inactive_in_use) {
			cv_status status = cond_inactiveDataSet.wait_for(lock, chrono::milliseconds(3000));
			if (status == cv_status::timeout) {
				Log("ERROR: unexpected timeout in getLockedinactiveDataSet" << endl); // should not happen except during debugging
			}
			//assert(inactive_in_use == false);
		}
		++inactiveUser;
		if (inactiveUser == 0) ++inactiveUser;
		user = inactiveUser;
		inactive_in_use = true;
		//Log("lock user " << user << endl);
		return nullptr;
	};

	bool has_inactiveLock(unsigned long user) {
		return user == inactiveUser;
	}

	// get inactive data set
	// only one thread can work on the inactive data set at one time, next caller will be blocked
	// while caller has this lock he should upload inactive data set to GPU
	void releaseLockedInactiveDataSet(unsigned long& user) {
		unique_lock<mutex> lock(monitorMutex_inactiveDataSet);
		assert(inactive_in_use == true);
		assert(has_inactiveLock(user));
		assert(user > 0);
		inactive_in_use = false;
		//Log("unlock user " << user << endl);
	};

	// activate the inactive data set and make it active
	// this will block the caller until active data set is no longer used by anyone
	// also cleanup unused data sets (?)
	// activate()

	// get current active data set and increase use count
	// accessActiveDataSet()

	// release active data set and decrease use count
	// realeaseActiveDataSet()

	atomic<int> activeUseCount = 0;
	// access protection for inactive data set:
	mutex monitorMutex_inactiveDataSet;
	condition_variable cond_inactiveDataSet;

private:
	atomic<unsigned long> inactiveUser = 1;  // simple user id, needed to check if caller has lock
	queue<EffectAppData*> myqueue;
	mutex monitorMutex;
	mutex monitorMutex_finished;
	condition_variable cond;
	condition_variable cond_finished; // effect update thread finished
	bool inactive_in_use = false;
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
 --> user update thread triggers all effect updates, so that they can run in parallel
     after that it waits for all efects to have consumed the update and switches incative/active set
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
	virtual EffectAppData* getInactiveAppDataSet(unsigned long& user) = 0;
	// get data set that is currently used by render code, should never be changed
	virtual EffectAppData* getActiveAppDataSet() = 0;
	// activate the curerently inactive data set. Includes uploading to GPU
	// rendering after this call returns will use the new data set. 
	// user id must have locked the data set previously
	virtual void activateAppDataSet(unsigned long user) = 0;
	// initate effect updates: Each effect is called with the inactive data set and triggers its update thread
	// before returning all effect updates are guaranteed to have finished, so it is save to switch app data afterwards
	static void update(vector<Effect*> effectList, Pipeline* pipeline, unsigned long& user);
	// update thread of each effect runs this method:
	static void runUpdate(Pipeline* pipeline, Effect* effectInstance);

	virtual ~Effect() = 0 {}; // still need to provide an (empty) base class destructor implementation even for pure virtual destructors
	//function<void(Frame*, Pipeline*)> updater = nullptr;
	//void setFinishedFrameConsumer(function<void(Frame*, Pipeline*)> consumer) { this->consumer = consumer; }
	UpdateQueue updateQueue;
protected:
	// copy effect data to GPU
	// called only from effect update thread.
	virtual void updateInactiveDataSet() = 0;
	bool initialized = false;  // set to true in init(). All effects that need to do something in destructor should check if effect was used at all...
	DXManager dxmanager;
	ResourceStateHelper* resourceStateHelper = ResourceStateHelper::getResourceStateHelper();
	DXGlobal* dxGlobal = nullptr;
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
	// queue to handle updates to constant data
	FenceData updateFenceData;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12CommandAllocator> updateCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> updateCommandList;
};
