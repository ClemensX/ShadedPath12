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
		Material material;  // moved here from light_basic.hlsi
	};
	// declare this struct static thread_local to hold thread specific data:
	struct ThreadLocalData {
		ComPtr<ID3D12GraphicsCommandList> commandList;
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		Camera camera;
		Camera cameram[2];
		CBV cbv;
		CBV cbvm[2];
		VR_Eyes vr_eyesm[2];
		bool initialized;
		//ThreadLocalData(Camera& cam) : camera(cam) {
		//	camera = xapp().camera;
		//};
	};

	UINT cbvAlignedSize = 0;	// aligned size of cbv for using indexes into larger buffers (256 byte alignment)
	~WorldObjectEffect();
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
		int threadNum = 0;
		int eyeNum = 0;
		DrawInfo(ComPtr<ID3D12Resource> &vBuffer, ComPtr<ID3D12Resource> &iBuffer)
			: vertexBuffer(vBuffer), indexBuffer(iBuffer)
		{};
	};

	void init(WorldObjectStore *objectStore, UINT maxThreads, UINT maxNumObjects = 0);
	void prepare();
	// update cbuffer and vertex buffer
	void update();
	//void draw();
	// create and upload vertex buffer for a newly loaded mesh
	void createAndUploadVertexBuffer(Mesh *mesh);
	void draw(Mesh * mesh, ComPtr<ID3D12Resource> &vertexBuffer, ComPtr<ID3D12Resource> &indexBuffer, XMFLOAT4X4 wvp, long numIndexes, TextureID tex, Material &material, UINT objNum, int threadNum, float alpha = 1.0f);
	void draw(DrawInfo &di);

	// bull update is concerning a large number of world objects
	void beginBulkUpdate();
	void endBulkUpdate();
	void divideBulk(size_t numObjects, size_t numThreads, const vector<unique_ptr<WorldObject>> *grp);
	void createRootSigAndPSO(ComPtr<ID3D12RootSignature> &sig, ComPtr<ID3D12PipelineState> &pso);
	bool inThreadOperation = false;
	mutex mutex_wo_drawing;
	static thread_local ThreadLocalData threadLocal;
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
	static void updateTask(BulkDivideInfo bi, int threadIndex, const vector<unique_ptr<WorldObject>> *grp, WorldObjectEffect *effect);
	atomic<bool> updateRunning = false;
	atomic<bool> allThreadsShouldEnd = false;
	atomic<int> workerThreadsCreated = 0;
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
	//static thread_local ComPtr<ID3D12GraphicsCommandList> commandList;
	//static thread_local ComPtr<ID3D12CommandAllocator> commandAllocator;
	//static thread_local Camera camera;
	//static thread_local bool initialized;
	condition_variable render_start; // main thread waits until all render threads are ready to run
	condition_variable render_ended; // main thread waits until render threads finished work
	condition_variable render_wait;  // worker thread waits until main thread signaly render start
	int waiting_for_rendering = 0;
	int finished_rendering = 0;
	mutex multiRenderLock;
public:
	//Camera *getThreadedCamera() { return &threadLocal.camera; };
};
