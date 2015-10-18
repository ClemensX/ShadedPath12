class EffectBase {
protected:
	HANDLE fenceEvent;
	ComPtr<ID3D12Fence> fence;
	UINT64 fenceValues[XApp::FrameCount];
	void WaitForGpu();
};