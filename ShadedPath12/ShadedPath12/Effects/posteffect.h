// post effect: get current frame from buffer and perform after effects

class PostEffect {
public:
	void init();
	void draw();
	void preDraw();
	void postDraw();
private:
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12CommandAllocator> commandAllocators[XApp::FrameCount];
	ComPtr<ID3D12GraphicsCommandList> commandLists[XApp::FrameCount];
	ComPtr<ID3D12RootSignature> rootSignature;
	HANDLE fenceEvent;
	ComPtr<ID3D12Fence> fence;
	UINT64 fenceValues[XApp::FrameCount];

};