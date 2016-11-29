// dotcross effect
// draw cross with x, y and z axis at specified point (world coordinates)

class Dotcross : EffectBase {
public:
	struct Vertex {
		XMFLOAT3 pos;
		XMFLOAT4 color;
	};
	struct CBV {
		XMFLOAT4X4 wvp;
		float linelen;
	};

	void init();
	void update(vector<XMFLOAT3> &points);
	void update();
	void draw();
	void setLineLength(float lineLength);
	void destroy();
	vector<XMFLOAT3> points;

private:
	float currentLinerLenth = 1.0f;

	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	void preDraw(int eyeNum);
	void postDraw();
	CBV cbv;
	mutex mutex_dotcross;
	void drawInternal(int eyeNum = 0);
	void updateTask();
};

