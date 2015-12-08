// linetext effect
// draw lines of text anywhere in the world. Uses a geometry shader to form letters out of simple lines

struct TextElement {
	//XMFLOAT4X4 rot;
	// pos x,y,z is pos in world coords, w is char to draw (used as int):
	XMFLOAT4 pos;
	// info x is index to rot matrices, w is position in string (used as int):
	XMFLOAT4 info;
	//int ch; // letter to draw
	//int charPos; // draw letter on character position charPos from start of text line
	//int textlen;
	//char *text;
};

class Linetext : EffectBase {
public:
	struct Vertex {
		XMFLOAT3 pos;
		XMFLOAT4 color;
	};
	struct CBV {
		// define size of letters: height = 4 * dx, width = 2 * dy (in world coords)
		XMFLOAT4X4 wvp;
		XMFLOAT4X4 rot_xy;
		XMFLOAT4X4 rot_zy;
		XMFLOAT4X4 rot_yx;
		XMFLOAT4X4 rot_cs; // angular, for coordinate system text
		float dx;
		float dy;
	};

	enum Plane { XY, ZY, YX, CS };
	void init();
	void update(vector<TextElement> &all_text);
	void update();
	void draw();
	void drawAll();
	int addTextLine(XMFLOAT4 pos, string text, Plane plane);
	int addTextLine(XMFLOAT4 pos, string text, UINT rotIndex);
	void changeTextLine(int linenum, string text);
	// set size of letters by specifying char height in units (default is 0.05)
	void setSize(float charHeight);
	void destroy();

private:
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	void preDraw();
	void postDraw();
	CBV cbv;
	mutex mutex_Linetext;
	void drawInternal();
	void updateTask();
	struct Line {
		UINT rotIndex;
		vector<TextElement> letters;
	};
	void createTextLine(XMFLOAT4 pos, string text, Line& line);
	vector<TextElement> texts;
	int getLetterValue(char c);
	vector<Line> lines;
	size_t vertexBufferElements[XApp::FrameCount]; // counts all TextElements 
};

