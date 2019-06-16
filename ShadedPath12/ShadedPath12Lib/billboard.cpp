#include "stdafx.h"
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

	void init();
	// add billboard to the scene, billboardEl will be copied and order number will be returned
	// use get() to get/change existing billboard
	size_t add(string texture_id, BillboardElement billboardEl);
	BillboardElement& get(string texture_id, int order_num);
	void update();
	void draw();
	void drawAll();
	void destroy();
	unordered_map<string, vector<BillboardElement>> billboards;

private:
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	void preDraw(int eyeNum);
	void postDraw();
	CBV cbv;
	mutex mutex_Billboard;
	void drawInternal(int eyeNum = 0);
	void updateTask();
	vector<Vertex>& recreateVertexBufferContent();
	void createBillbordVertexData(Vertex* cur_billboard, BillboardElement& bb);
	//vector<BillboardElement> texts;
	atomic<bool> updateRunning = false;
	future<void> billboardFuture;
	ComPtr<ID3D12CommandAllocator> updateCommandAllocator;
	ComPtr<ID3D12GraphicsCommandList> updateCommandList;
	FrameResource updateFrameData;
	ComPtr<ID3D12Resource> vertexBufferX;
	ComPtr<ID3D12Resource> vertexBufferUploadX;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewX;
	//UINT numberOfVertices = 0;
};

