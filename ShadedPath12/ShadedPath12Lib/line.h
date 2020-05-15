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
	UINT numVericesToDraw = 0;
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
	void addOneTime(vector<LineDef>& linesToAdd, unsigned long& user);
	// update cbuffer and vertex buffer
	void update();
	void updateCBV(CBV newCBV);
	// draw all lines in single call to GPU
	void draw(Frame* frame, FrameDataGeneral* dfg, FrameDataLine* fdl, Pipeline* pipeline);
	void destroy();
	// Inherited via Effect
	LineEffectAppData* getInactiveAppDataSet(unsigned long& user) override
	{
		//errorOnThreadChange();
		LineEffectAppData* inactive = &appDataSets[currentInactiveAppDataSet];
		if (inactive->noUpdate) {
			return inactive;
		}
		assert(updateQueue.has_inactiveLock(user));
		return inactive;
	};
	LineEffectAppData* getActiveAppDataSet() override
	{
		//errorOnThreadChange();
		updateQueue.activeUseCount++;
		//assert(updateQueue.activeUseCount <= 3);
		//Log("active data set counter " << updateQueue.activeUseCount << endl);
		if (currentActiveAppDataSet < 0) {
			Error(L"active data set not available in Line Effect. Cannot continue.");
		}
		LineEffectAppData* act = &appDataSets[currentActiveAppDataSet];
		act->bufferResource->useCounter++;
		//Log("active data set use counter increased: gen " << act->bufferResource->generation << " count " << act->bufferResource->useCounter << endl);
		return act;
	}

	void releaseActiveAppDataSet(EffectAppData* act_base)
	{
		LineEffectAppData* act = (LineEffectAppData*)act_base;
		updateQueue.activeUseCount--;
		//Log("active data set counter " << updateQueue.activeUseCount << endl);
		assert(updateQueue.activeUseCount >= 0);
		if (currentActiveAppDataSet < 0) {
			Error(L"active data set not available in Line Effect. Cannot continue.");
		}
		act->bufferResource->useCounter--;
		//Log("active data set use counter decreased: gen " << act->bufferResource->generation << " count " << act->bufferResource->useCounter << endl);
	}

	// make inactive app data set active and vice versa
	// synchronized
	void activateAppDataSet(unsigned long user) override;

	void reinitializeThreadResources() {
		//Error(L"not implemented");
		Log("Warning: other thread used effect update: " << updateThreadId << endl);
		//updateCommandAllocator = nullptr;
		updateCommandList->Release();
		updateCommandAllocator->Release();
		//updateCommandList = nullptr;
		ThrowIfFailed(dxGlobal->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&updateCommandAllocator)));
		//NAME_D3D12_OBJECT_SUFF(updateCommandAllocator, res->generation);
		ThrowIfFailed(dxGlobal->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, updateCommandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&updateCommandList)));
		//NAME_D3D12_OBJECT_SUFF(updateCommandList, res->generation);
		updateCommandList->Close();

		updateThreadId = 0;
	}

private:
	//vector<LineDef> lines;
	//vector<LineDef> addLines;
	bool dirty;
	int drawAddLinesSize;

	//ComPtr<ID3D12PipelineState> pipelineState;
	//ComPtr<ID3D12RootSignature> rootSignature;
	//void preDraw(int eyeNum);
	//void postDraw();
	CBV cbv, updatedCBV;
	//bool signalUpdateCBV = false;
	//mutex mutex_lines;
	//void drawInternal(int eyeNum = 0);
	//void updateTask();
	//UINT numVericesToDraw = 0;
	LineEffectAppData appDataSets[2];
	bool disabled = false;
	// Inherited via Effect
	// set in init()
};
