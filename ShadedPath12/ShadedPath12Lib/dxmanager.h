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
	// add resource, fails if already addaed
	void add(ID3D12Resource* res, D3D12_RESOURCE_STATES state) {
		assert(resourceStates.count(res) == 0);
		ResourceStateInfo si;
		si.current_state = state;
		resourceStates[res] = si;
	};
	// add resource with state, keeps old state if already added earlier
	void addOrKeep(ID3D12Resource* res, D3D12_RESOURCE_STATES state) {
		if (resourceStates.count(res) == 0) {
			// new resource
			add(res, state);
		} 
		// nothing to do if already added
	};
	void toState(ID3D12Resource* res, D3D12_RESOURCE_STATES state, ID3D12GraphicsCommandList *cl) {
		assert(resourceStates.count(res) > 0);
		ResourceStateInfo &resourceStateInfo = resourceStates.at(res);
		if (resourceStateInfo.current_state != state) {
			cl->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(res, resourceStateInfo.current_state, state));
			//Log("to State " << res << " " << resourceStateInfo.current_state << " --> " << state << endl);
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
	// FrameCount should be a copy of XApp::FrameCount, but we don't want to reference XApp from here
	// That the size is the same as in XApp is checked in intializer
	static const UINT FrameCount = 3;
private:
	//int currentFrameIndex = 0;
	int frameCount;
};
