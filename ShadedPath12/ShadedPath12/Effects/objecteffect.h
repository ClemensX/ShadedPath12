// world object effect
struct BulkDivideInfo {
	UINT start;
	UINT end;
};

class WorldObjectEffect : public EffectBase {
public:
	struct ConstantBufferFixed {
		XMFLOAT4 color;
	};
	struct CBV {
		XMFLOAT4X4 wvp;
		XMFLOAT4X4 world;  // needed for normal calculations
		XMFLOAT3   cameraPos;
		float    alpha;
	};
	UINT cbvAlignedSize = 0;	// aligned size of cbv for using indexes into larger buffers (256 byte alignment)
	// gather all info needed to draw one object here
	struct DrawInfo {
		ComPtr<ID3D12Resource> &vertexBuffer;
		ComPtr<ID3D12Resource> &indexBuffer;
		XMFLOAT4X4 world;  // world matrix
		long numIndexes;
		TextureID tex;
		Material *material;
		float alpha;
		Mesh* mesh;
		UINT objectNum;
		DrawInfo(ComPtr<ID3D12Resource> &vBuffer, ComPtr<ID3D12Resource> &iBuffer)
			: vertexBuffer(vBuffer), indexBuffer(iBuffer)
		{};
	};

	void init(WorldObjectStore *objectStore, UINT maxNumObjects = 0);
	void prepare();
	// update cbuffer and vertex buffer
	void update();
	//void draw();
	// create and upload vertex buffer for a newly loaded mesh
	void createAndUploadVertexBuffer(Mesh *mesh);
	void draw(Mesh * mesh, ComPtr<ID3D12Resource> &vertexBuffer, ComPtr<ID3D12Resource> &indexBuffer, XMFLOAT4X4 wvp, long numIndexes, TextureID tex, Material &material, UINT objNum, float alpha = 1.0f);
	void draw(DrawInfo &di);

	// bull update is concerning a large number of world objects
	void beginBulkUpdate();
	void endBulkUpdate();
	void divideBulk(size_t numObjects, size_t numThreads, const vector<unique_ptr<WorldObject>> *grp);
	void createRootSigAndPSO(ComPtr<ID3D12RootSignature> &sig, ComPtr<ID3D12PipelineState> &pso);
	bool inThreadOperation = false;
private:
	ConstantBufferFixed cb;
	// globally enable wireframe display of objects
	bool wireframe = false;

	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	void preDraw(DrawInfo &di);
	void postDraw();
	CBV cbv;
	mutex mutex_Object;
	void drawInternal(DrawInfo &di);
	static void updateTask(BulkDivideInfo bi, const vector<unique_ptr<WorldObject>> *grp, WorldObjectEffect *effect);
	atomic<bool> updateRunning = false;
	future<void> objecteffectFuture;
	ComPtr<ID3D12CommandAllocator> updateCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> updateCommandList;
	FrameResource updateFrameData;
	ComPtr<ID3D12Resource> vertexBufferX;
	ComPtr<ID3D12Resource> vertexBufferUploadX;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewX;
	WorldObjectStore *objectStore;
	bool inBulkOperation = false;
	vector<BulkDivideInfo> bulkInfos;
	BulkDivideInfo globbi;
	static thread_local ComPtr<ID3D12GraphicsCommandList> commandList;
	condition_variable render_start; // worker threads wait until new render cycle begins
	condition_variable render_ended; // main thread waits until render threads finished work
	int waiting_for_rendering = 0;
	int finished_rendering = 0;
	mutex multiRenderLock;
};
