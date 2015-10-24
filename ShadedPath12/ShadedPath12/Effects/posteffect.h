// post effect: get current frame from buffer and perform after effects

class PostEffect : EffectBase {
public:
	void init();
	void init2();
	void draw();
	void preDraw();
	void postDraw();
	struct Vertex {
		XMFLOAT3 pos;
	};
private:
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12CommandAllocator> commandAllocators[XApp::FrameCount];
	ComPtr<ID3D12GraphicsCommandList> commandLists[XApp::FrameCount];
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	ComPtr<ID3D12Resource> m_texture;

};