class MeshObject {
public:
	MeshObject();
	virtual ~MeshObject();
	//XMFLOAT4 pos;
	XMFLOAT3& pos();
	XMFLOAT4 rot_quaternion;
	float scale = 1.0f;
	bool drawBoundingBox = false;
	bool drawNormals = false;
	Material material;
};


// MeshObject Store:
class MeshObjectStore : public EffectBase {
public:
	//singleton:
	static MeshObjectStore *getStore() {
		static MeshObjectStore singleton;
		return &singleton;
	};

	// uploading mesh data to GPU can only be done in GpuUploadPhase
	// this prevents unnecessary waits between upload requests
	void gpuUploadPhaseStart() { inGpuUploadPhase = true; };
	void gpuUploadPhaseEnd();

	// objects
	// load object definition from .b file, save under given hash name
	void loadObject(wstring filename, string id, float scale = 1.0f, XMFLOAT3 *displacement = nullptr);
	// add loaded object to scene
	void addObject(string groupname, string id, XMFLOAT3 pos, TextureID tid = 0);
	void addObject(WorldObject &w, string id, XMFLOAT3 pos, TextureID tid = 0);
	// obbject groups: give fast access to specific objects (e.g. all worm NPCs)
	void createGroup(string groupname);
	const vector<unique_ptr<WorldObject>> *getGroup(string groupname);
	// draw all objects within a group (all have same mesh), set threadNum > 1 to draw with multiple threads
	void drawGroup(string groupname, size_t threadNum = 0);
	void setWorldObjectEffect(WorldObjectEffect *objectEffect);
private:
	unordered_map<string, vector<unique_ptr<WorldObject>>> groups;
	unordered_map<string, Mesh> meshes;

	MeshObjectStore() {};									// prevent creation outside this class
	MeshObjectStore(const MeshObjectStore&);				// prevent creation via copy-constructor
	MeshObjectStore & operator = (const MeshObjectStore &);	// prevent instance copies

	bool inGpuUploadPhase = false;	// signal that this effect is uploading data to GPU

public:
	// effect methods / members:
	void init();
	void createAndUploadVertexAndIndexBuffer(Mesh *mesh);
	void createRootSigAndPSO(ComPtr<ID3D12RootSignature>& sig, ComPtr<ID3D12PipelineState>& pso);
	struct CBV {
		XMFLOAT4X4 wvp;
		XMFLOAT4X4 world;  // needed for normal calculations
		XMFLOAT3   cameraPos;
		float    alpha;
	};
private:
	// globally enable wireframe display of objects
	bool wireframe = false;

	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12GraphicsCommandList> updateCommandList;
	ComPtr<ID3D12CommandAllocator> updateCommandAllocator;
	FrameResource updateFrameData;
};