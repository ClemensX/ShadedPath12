// billboard effect
// draw flat rectangular textures.
// vertex positions are constantly updated in one worker thread
// GPU sees only the last completed list, so no wait necessary on draw call

// draw call transfers complete lists of triangles in world coords belonging to one texture
// --> one draw call per used texture

// CPU structures
struct BillboardElement {
	XMFLOAT3 pos;
	XMFLOAT4 normal;
	XMFLOAT2 size; // x = width, y = height
};

class Billboard : EffectBase {
public:
	struct Vertex {
		XMFLOAT3 pos;
		XMFLOAT4 normal;
	};
	struct CBV {
		XMFLOAT4X4 wvp;
	};

	void init();
	// add billboard to the scene, billboardEl will be copied and order number will be returned
	// use get() to get/change existing billboard
	size_t add(string texture_id, BillboardElement billboardEl);
	BillboardElement & get(string texture_id, int order_num);
	void update();
	void draw();
	void drawAll();
	void destroy();

private:
	unordered_map<string, vector<BillboardElement>> billboards;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	void preDraw();
	void postDraw();
	CBV cbv;
	mutex mutex_Billboard;
	void drawInternal();
	void updateTask();
	//vector<BillboardElement> texts;
	size_t vertexBufferElements[XApp::FrameCount]; // counts all TextElements 
	atomic<bool> updateRunning = false;
	future<void> linetextFuture;
	ComPtr<ID3D12CommandAllocator> updateCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> updateCommandList;
	FrameResource updateFrameData;
	ComPtr<ID3D12Resource> vertexBufferX;
	ComPtr<ID3D12Resource> vertexBufferUploadX;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewX;
};

