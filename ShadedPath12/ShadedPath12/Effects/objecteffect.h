// world object effect
class WorldObjectEffect : public EffectBase {
public:
	struct ConstantBufferFixed {
		XMFLOAT4 color;
	};
	struct CBV {
		XMFLOAT4X4 wvp;
		float    alpha;
	};
	// gather all info needed to draw one object here
	struct DrawInfo {
		ComPtr<ID3D12Resource> &vertexBuffer;
		ComPtr<ID3D12Resource> &indexBuffer;
		XMFLOAT4X4 wvp;
		long numIndexes;
		TextureID tex;
		Material *material;
		float alpha;
		Mesh* mesh;
		DrawInfo(ComPtr<ID3D12Resource> &vBuffer, ComPtr<ID3D12Resource> &iBuffer)
			: vertexBuffer(vBuffer), indexBuffer(iBuffer)
		{};
	};

	void init(WorldObjectStore *objectStore);
	void prepare();
	// update cbuffer and vertex buffer
	void update();
	//void draw();
	// create and upload vertex buffer for a newly loaded mesh
	void createAndUploadVertexBuffer(Mesh *mesh);
	void draw(Mesh * mesh, ComPtr<ID3D12Resource> &vertexBuffer, ComPtr<ID3D12Resource> &indexBuffer, XMFLOAT4X4 wvp, long numIndexes, TextureID tex, Material &material, float alpha = 1.0f);
	void draw(DrawInfo &di);

private:
	ConstantBufferFixed cb;
	// globally enable wireframe display of objects
	bool wireframe = false;

private:
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	void preDraw();
	void postDraw();
	CBV cbv;
	mutex mutex_Object;
	void drawInternal(DrawInfo &di);
	void updateTask();
	atomic<bool> updateRunning = false;
	future<void> objecteffectFuture;
	ComPtr<ID3D12CommandAllocator> updateCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> updateCommandList;
	FrameResource updateFrameData;
	ComPtr<ID3D12Resource> vertexBufferX;
	ComPtr<ID3D12Resource> vertexBufferUploadX;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewX;
	WorldObjectStore *objectStore;
};
