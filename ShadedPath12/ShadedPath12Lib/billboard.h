// billboard effect
// draw flat rectangular textures.
// vertex positions are constantly updated in one worker thread
// GPU sees only the last completed list, so no wait necessary on draw call

// draw call transfers complete lists of triangles in world coords belonging to one texture
// --> one draw call per used texture
// all textures have to be already in GPU memory and ready to be used as SRV

// CPU structures
struct BillboardElement {
	XMFLOAT3 pos;
	XMFLOAT4 normal;
	XMFLOAT2 size; // x = width, y = height
};

class BillboardEffectAppData : public EffectAppData {
public:
	unordered_map<string, vector<BillboardElement>> billboards;
	~BillboardEffectAppData() override { };
	BufferResource* bufferResource = nullptr;
};

// per frame resources for this effect
struct FrameDataBillboard : FrameDataBase {
public:
	//friend class DXGlobal;
};

class Billboard : public Effect {
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

	void init(DXGlobal* a, FrameDataBillboard* fd, FrameDataGeneral* fd_general_, Pipeline* pipeline);
	// add billboard to inactive data set, billboardElement will be copied and order number will be returned
	// order numbers are counted per texture and start with 0
	// use get() to get/change existing billboard
	size_t add(string texture_id, BillboardElement billboardEl, unsigned long& user);
	// get billboard with order number order_num for texture_id
	BillboardElement& get(string texture_id, int order_num, unsigned long& user);
	void draw(Frame* frame, FrameDataGeneral *dfg, FrameDataBillboard *fdb, Pipeline* pipeline);

	// Inherited via Effect
	BillboardEffectAppData* getInactiveAppDataSet(unsigned long &user) override
	{
		BillboardEffectAppData* inactive = &appDataSets[currentInactiveAppDataSet];
		if (inactive->noUpdate) {
			return inactive;
		}
		assert(updateQueue.has_inactiveLock(user));
		return inactive;
	};
	BillboardEffectAppData* getActiveAppDataSet() override
	{
		updateQueue.activeUseCount++;
		//assert(updateQueue.activeUseCount <= 3);
		//Log("active data set counter " << updateQueue.activeUseCount << endl);
		if (currentActiveAppDataSet < 0) {
			Error(L"active data set not available in Billboard. Cannot continue.");
		}
		BillboardEffectAppData* act = &appDataSets[currentActiveAppDataSet];
		act->bufferResource->useCounter++;
		//Log("active data set use counter increased: gen " << act->bufferResource->generation << " count " << act->bufferResource->useCounter << endl);
		return act;
	}
	void releaseActiveAppDataSet(EffectAppData* act_base) override
	{
		BillboardEffectAppData *act = (BillboardEffectAppData *) act_base;
		updateQueue.activeUseCount--;
		//Log("active data set counter " << updateQueue.activeUseCount << endl);
		assert(updateQueue.activeUseCount >= 0);
		if (currentActiveAppDataSet < 0) {
			Error(L"active data set not available in Billboard. Cannot continue.");
		}
		act->bufferResource->useCounter--;
		//Log("active data set use counter decreased: gen " << act->bufferResource->generation << " count " << act->bufferResource->useCounter << endl);
	}

	// make inactive app data set active and vice versa
	// no synchronization, must not be called from multiple threads at the same time
	virtual void activateAppDataSet(unsigned long user) override;

	~Billboard() {
	};


	//mutex dataSetMutex; // to synchronize updates to billboard data (active/inactive)
private:
	CBV cbv;
	vector<Vertex>& recreateVertexBufferContent(vector<Vertex>& vertices, BillboardEffectAppData *);
	void createBillbordVertexData(Vertex* cur_billboard, BillboardElement& bb);
	BillboardEffectAppData appDataSets[2];
};

