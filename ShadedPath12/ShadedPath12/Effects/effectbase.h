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
};