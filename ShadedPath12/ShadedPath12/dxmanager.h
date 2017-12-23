#pragma once

/*
 * Ultility class to store state attributes for 3d objects.
 * Used to store info if objects already stored on GPU / compute buffer
 */
class ObjectStateList {
private:
	enum State { IN_CPU, IN_GPU, IN_COMPUTE };
	size_t count_in_gpu;
	size_t count_in_compute;

public:
	void init(size_t count) {
		states.resize(count, IN_CPU);
		count_in_gpu = 0; // all antries invalid
		count_in_compute = 0;
	};

	// mark object as copied to GPU mem
	void setObjectValidGPU(unsigned int objNum, bool valid) {
		size_t index = (size_t)objNum;
		if (valid) {
			if (states[index] == IN_CPU) count_in_gpu++;
			states[index] = IN_GPU;
		}
		else {
			if (states[index] != IN_CPU) count_in_gpu--;
			states[index] = IN_CPU;
		}
	};

	// mark object as copied to compute buffer, only objects already in GPU can be ok in compute buffer
	void setObjectValidCompute(unsigned int objNum, bool valid) {
		size_t index = (size_t)objNum;
		auto cur_state = states[index];
		if (valid) {
			if (cur_state == IN_GPU) {
				count_in_compute++;
				states[index] = IN_COMPUTE;
			}
		}
		else {
			if (cur_state == IN_COMPUTE) count_in_compute--;
			states[index] = IN_GPU;
		}
	};

	bool isValid() {
		//Log("count_in_gpu = " << count_in_gpu << endl);
		//Log("count_in_compute = " << count_in_compute << endl);
		return count_in_compute == count_in_gpu;
	};
private:
	vector<State> states;
};

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

/*
 * Utility class to manage DX resources. 
 * One instance should be instantiated for each effect class.
 * All ressources are kept local to current frame
*/

class DXManager {
public:
	// create manager for number of frames
	DXManager(int frameCount) { 
		assert(frameCount == FrameCount);
		this->frameCount = frameCount;
	};
	void init(ID3D12Device *d) { device = d; };
	void setCurrentFrame(int frameNum) { currentFrame = frameNum; };
	// Buffer sets are identified by number, should start from 0
	void createConstantBufferSet(UINT setNum, UINT maxThreads, UINT maxObjects, size_t singleObjectSize, wchar_t * name);
	void uploadConstantBufferSet(UINT setNum, size_t singleObjectSize, void *mem_source);
	D3D12_GPU_VIRTUAL_ADDRESS getConstantBufferSetVirtualAddress(UINT setNum, int eyeNum);
	// buffer sets end

	void createConstantBuffer(UINT maxThreads, UINT maxObjects, size_t singleObjectSize, wchar_t * name);
	void createUploadBuffers();
	void createGraphicsExecutionEnv(ID3D12PipelineState *ps);
	void createComputeExecutionEnv();
	UINT64 getOffsetInConstantBuffer(UINT objectIndex, int eyeNum = 0);
	// copy data to upload buffer
	void upload(UINT objectIndex, int eyeNum, void* mem_source);
	// copy complete constant buffer to compute buffer
	void copyToComputeBuffer(FrameResource & f);
	D3D12_GPU_VIRTUAL_ADDRESS getCBVVirtualAddress(UINT objectIndex, int eyeNum);
	ID3D12Resource *getConstantBuffer() { return singleCBVResources[currentFrame].Get(); };
	//ID3D12CommandAllocator *getGraphicsCommandAllocator() { return commandAllocators[currentFrame].Get(); };
	ComPtr<ID3D12CommandAllocator> &getGraphicsCommandAllocatorComPtr() { return commandAllocators[currentFrame]; };
	ComPtr<ID3D12GraphicsCommandList> &getGraphicsCommandListComPtr() { return commandLists[currentFrame]; };
private:
	// FrameCount should be a copy of XApp::FrameCount, but we don't want to reference XApp from here
	// That the size is the same as in XApp is checked in intializer
	static const UINT FrameCount = 3;
	int currentFrame = 0;
	int frameCount;
	vector<ComPtr<ID3D12Resource>> singleCBVResources; // one for each thread and frame count
	UINT maxObjects = 0;	// max number of entities allowed in this buffer
	UINT slotSize = 0;      // allocated size for single CVB element (aligned size)
	UINT totalSize = 0;     // tozal size of one constant buffer
	// handle constant upload buffer as one large chunk (one buffer for all frames)
	ComPtr<ID3D12Resource> constantBufferUpload;
	UINT8* constantBufferUploadCPU;

	ResourceStateHelper *resourceStateHelper = ResourceStateHelper::getResourceStateHelper();
	// Graphics objects:
	ComPtr<ID3D12CommandQueue> commandQueues[FrameCount];
	ComPtr<ID3D12CommandAllocator> commandAllocators[FrameCount];
	ComPtr<ID3D12GraphicsCommandList> commandLists[FrameCount];
	ID3D12PipelineState *graphics_ps;

	// Compute objects.
	ComPtr<ID3D12PipelineState> computePipelineState;
	ComPtr<ID3D12RootSignature> computeRootSignature;
	ComPtr<ID3D12CommandAllocator> computeAllocator[FrameCount];
	ComPtr<ID3D12CommandQueue> computeCommandQueue[FrameCount];
	ComPtr<ID3D12GraphicsCommandList> computeCommandList[FrameCount];

	ID3D12Device *device = nullptr;
	vector<ComPtr<ID3D12Resource>> cbvSetResources; // for all numbered buffer sets
	vector<UINT8*> cbvSetGPUDest;  // memcpy() changed cbv data to this address before draw()
	UINT setSize = 0;
public:
	ObjectStateList objectStateLists[FrameCount];
};