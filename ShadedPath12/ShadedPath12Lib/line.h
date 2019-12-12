// line effect - draw simple lines in world coordinates
struct LineDef {
	XMFLOAT3 start, end;
	XMFLOAT4 color;
};

class LineEffectAppData : public EffectAppData {
public:
	vector<LineDef> lines;
	vector<LineDef> oneTimeLines;
	~LineEffectAppData() override { };
	BufferResource* bufferResource = nullptr;
};

// per frame resources for this effect
struct FrameDataLine : FrameDataBase {
public:
	//friend class DXGlobal;
};

class LinesEffect : public Effect {
public:
	struct Vertex {
		XMFLOAT3 pos;
		XMFLOAT4 color;
	};
	struct CBV {
		XMFLOAT4X4 wvp;
	};

	void init(DXGlobal* a, FrameDataLine* fdl, FrameDataGeneral* fd_general_, Pipeline* pipeline);
	// add lines - they will never  be removed
	void add(vector<LineDef>& linesToAdd, unsigned long& user);
	// add lines just for next draw call
	void addOneTime(vector<LineDef>& linesToAdd);
	// update cbuffer and vertex buffer
	void update();
	void updateCBV(CBV newCBV);
	// draw all lines in single call to GPU
	void draw();
	void destroy();
	// Inherited via Effect
	LineEffectAppData* getInactiveAppDataSet(unsigned long& user) override
	{
		LineEffectAppData* inactive = &appDataSets[currentInactiveAppDataSet];
		if (inactive->noUpdate) {
			return inactive;
		}
		assert(updateQueue.has_inactiveLock(user));
		return inactive;
	};
	LineEffectAppData* getActiveAppDataSet() override
	{
		updateQueue.activeUseCount++;
		//assert(updateQueue.activeUseCount <= 3);
		//Log("active data set counter " << updateQueue.activeUseCount << endl);
		if (currentActiveAppDataSet < 0) {
			Error(L"active data set not available in Billboard. Cannot continue.");
		}
		LineEffectAppData* act = &appDataSets[currentActiveAppDataSet];
		act->bufferResource->useCounter++;
		//Log("active data set use counter increased: gen " << act->bufferResource->generation << " count " << act->bufferResource->useCounter << endl);
		return act;
	}

	void activateAppDataSet(unsigned long user) override
	{
	}
private:
	vector<LineDef> lines;
	vector<LineDef> addLines;
	bool dirty;
	int drawAddLinesSize;

	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	void preDraw(int eyeNum);
	void postDraw();
	CBV cbv, updatedCBV;
	bool signalUpdateCBV = false;
	mutex mutex_lines;
	void drawInternal(int eyeNum = 0);
	void updateTask();
	UINT numVericesToDraw = 0;
	LineEffectAppData appDataSets[2];
	bool disabled = false;
	// Inherited via Effect
	// set in init()
};
