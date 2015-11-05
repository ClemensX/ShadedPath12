// line effect - draw simple lines in world coordinates
struct LineDef {
	XMFLOAT3 start, end;
	XMFLOAT4 color;
};


class LinesEffect : EffectBase {
public:
	struct Vertex {
		XMFLOAT3 pos;
		XMFLOAT4 color;
	};
	struct cbv_ {
		XMFLOAT4X4 wvp;
	};

	void init();
	// add lines - they will never  be removed
	void add(vector<LineDef> &linesToAdd);
	// add lines just for next draw call
	void addOneTime(vector<LineDef> &linesToAdd);
	// update cbuffer and vertex buffer
	void update();
	void updateTask();
	void updateCBV(LinesEffect::cbv_ newCBV);
	void MoveToNextFrame();
	// draw all lines in single call to GPU
	void draw();
	void next();
	void destroy();
	//ComPtr<ID3D11Buffer> vertexBuffer;    // for fixed lines
	//ComPtr<ID3D11Buffer> vertexBufferAdd; // for one time lines
private:
	vector<LineDef> lines;
	vector<LineDef> addLines;
	bool dirty;
	int drawAddLinesSize;

	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> vertexBufferUpload;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12CommandAllocator> commandAllocators[XApp::FrameCount];
	ComPtr<ID3D12GraphicsCommandList> commandLists[XApp::FrameCount];
	ComPtr<ID3D12RootSignature> rootSignature;
	//XApp *xapp;  done through global instance from xapp.cpp
	void preDraw();
	void postDraw();
	cbv_ cbv, updatedCBV;
	ComPtr<ID3D12Resource> cbvResource;
	UINT8* cbvGPUDest;  // mempy changed cbv data to this address before draw()
	bool signalUpdateCBV = false;
	mutex mutex_lines;
};
