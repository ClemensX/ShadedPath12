
struct FrameResource {
	HANDLE fenceEvent;
	ComPtr<ID3D12Fence> fence;
	UINT64 fenceValue;
};



