#pragma once

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
	DXManager(int frameCount) { this->frameCount = frameCount; };
	void setCurrentFrame(int frameNum) { currentFrame = frameNum; };
	void createConstantBuffer(UINT maxThreads, UINT maxObjects, size_t singleObjectSize, wchar_t * name);
private:
	int currentFrame = 0;
	int frameCount;
	vector<ComPtr<ID3D12Resource>> singleCBVResources; // one for each thread and frame count
	UINT maxObjects = 0;	// max number of entities allowed in this buffer
	UINT slotSize = 0;      // allocated size for single CVB element (aligned size)
};