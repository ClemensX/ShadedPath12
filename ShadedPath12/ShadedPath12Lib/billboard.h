// billboard effect
// draw flat rectangular textures.
// vertex positions are constantly updated in one worker thread
// GPU sees only the last completed list, so no wait necessary on draw call

// draw call transfers complete lists of triangles in world coords belonging to one texture
// --> one draw call per used texture
// all textures have to be already in GPU memory and ready to be used as SRV

// CPU structures
struct BillboardElement {
	XMFLOAT3 pos;
	XMFLOAT4 normal;
	XMFLOAT2 size; // x = width, y = height
};

class BillboardEffectAppData : public EffectAppData {
public:
	unordered_map<string, vector<BillboardElement>> billboards;
	~BillboardEffectAppData() override { };
};

// per frame resources for this effect
struct FrameDataBillboard : FrameDataBase {
public:
	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> vertexBufferUpload;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	friend class DXGlobal;
};

class Billboard : Effect {
public:
	struct Vertex {
		XMFLOAT4 pos;
		XMFLOAT4 normal;
		XMFLOAT2 uv;
	};
	struct CBV {
		XMFLOAT4X4 wvp;
		XMFLOAT3 cam;  // camera world position
	};

	void init(DXGlobal* a, FrameDataBillboard* fd, FrameDataGeneral* fd_general_, Pipeline* pipeline);
	// add billboard to inactive data set, billboardElement will be copied and order number will be returned
	// order numbers are counted per texture and start with 0
	// use get() to get/change existing billboard
	size_t add(string texture_id, BillboardElement billboardEl);
	// get billboard with order number order_num for texture_id
	BillboardElement& get(string texture_id, int order_num);
	void update();
	void draw(Frame* frame, FrameDataGeneral *dfg, FrameDataBillboard *fdb, Pipeline* pipeline);
	void drawAll();
	void destroy();

	// Inherited via Effect
	BillboardEffectAppData* getInactiveAppDataSet() override
	{
		return &appDataSets[currentInactiveAppDataSet];
	};
	BillboardEffectAppData* getActiveAppDataSet() override
	{
		if (currentActiveAppDataSet < 0) {
			throw "active data set not available in Billboard";
		}
		return &appDataSets[currentActiveAppDataSet];
	}
	virtual void activateAppDataSet() override
	{
		currentActiveAppDataSet = (currentActiveAppDataSet + 1) % 2;
		currentInactiveAppDataSet = (currentInactiveAppDataSet + 1) % 2;
	}
	~Billboard() {};

private:
	void preDraw(int eyeNum);
	void postDraw();
	CBV cbv;
	mutex mutex_Billboard;
	void drawInternal(int eyeNum = 0);
	void updateTask();
	vector<Vertex>& recreateVertexBufferContent(vector<Vertex>& vertices);
	void createBillbordVertexData(Vertex* cur_billboard, BillboardElement& bb);
	//vector<BillboardElement> texts;
	atomic<bool> updateRunning = false;
	future<void> billboardFuture;
	//FrameResource updateFrameData;
	BillboardEffectAppData appDataSets[2];
	int currentInactiveAppDataSet = 0;
	int currentActiveAppDataSet = -1;
	//UINT numberOfVertices = 0;
};

