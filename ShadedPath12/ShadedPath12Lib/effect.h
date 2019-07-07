// base class for App Data
class EffectAppData {
public:
	virtual ~EffectAppData() = 0 {}; // still need to provide an (empty) base class destructor implementation even for pure virtual destructors
};

// base class for Per Frame Data
class EffectFrameData {
public:
	virtual ~EffectFrameData() = 0 {}; // still need to provide an (empty) base class destructor implementation even for pure virtual destructors
};

// base class for effects

class Effect {
	// effects need 3 kinds of data:
	// 1) Global data for the effect, only needed once. Like PipelineState and RootSignature
	// 2) Application data thats resembles the application state with regards for an effect. Like position and normal for billboard elements
	//    Needs to be there twice: there is alway an acive data set used for rendering, the inacive set can be manipulated by the application until it is uploaded to GPU and made active
	// 3) Per Frame data. Usually 3 sets. Frame specific resources the effect needs to run in parallel, like CBVs with per frame WVP matrix

public:
	// get inactive data set. It can be changed by effect or app code
	virtual EffectAppData* getInactiveAppDataSet() = 0;
	// get data set that is currently used by render code, should never be changed
	virtual EffectAppData* getActiveAppDataSet() = 0;
	// activate the curerently inactive data set. Includes uploading to GPU
	// rendering after this call returns will use the new data set. 
	// returns nullptr if there is no active set yet
	virtual void activateAppDataSet() = 0;

	virtual ~Effect() = 0 {}; // still need to provide an (empty) base class destructor implementation even for pure virtual destructors
protected:
	bool initialized = false;  // set to true in init(). All effects that need to do something in destructor should check if effect was used at all...
	DXManager dxmanager;
};