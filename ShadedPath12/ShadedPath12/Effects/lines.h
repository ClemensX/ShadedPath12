// line effect - draw simple lines in world coordinates
struct LineDef {
	XMFLOAT3 start, end;
	XMFLOAT4 color;
};


class Lines {
public:
	struct Vertex {
		XMFLOAT3 pos;
		XMFLOAT4 color;
	};
	struct ConstantBufferFixed {
		XMFLOAT4X4 wvp;
	};

	void prepare();
	// add lines - they will never  be removed
	void add(vector<LineDef> &linesToAdd);
	// add lines just for next draw call
	void addOneTime(vector<LineDef> &linesToAdd);
	// update cbuffer and vertex buffer
	void update();
	// draw all lines in single call to GPU
	void draw();
	//ComPtr<ID3D11Buffer> vertexBuffer;    // for fixed lines
	//ComPtr<ID3D11Buffer> vertexBufferAdd; // for one time lines
private:
	vector<LineDef> lines;
	vector<LineDef> addLines;
	ConstantBufferFixed cb;
	bool dirty;
	int drawAddLinesSize;
};
