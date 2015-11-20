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
	struct CBV {
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
	void updateCBV(CBV newCBV);
	// draw all lines in single call to GPU
	void draw();
	void destroy();
private:
	vector<LineDef> lines;
	vector<LineDef> addLines;
	bool dirty;
	int drawAddLinesSize;

	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	void preDraw();
	void postDraw();
	CBV cbv, updatedCBV;
	bool signalUpdateCBV = false;
	mutex mutex_lines;
	void drawInternal();

};
