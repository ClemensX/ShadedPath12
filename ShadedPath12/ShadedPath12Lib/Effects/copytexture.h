// test worker sub types
class WorkerCopyTextureCommand : public WorkerCommand {
public:
	string textureName;
	void perform();
	XApp *xapp = nullptr;
	ResourceStateHelper *resourceStateHelper = nullptr;
	AppWindowFrameResource *frameResource;
};

class CopyTextureEffect : EffectBase {
public:
	void init(GlobalEffect *globalEffect);
	void setThreadCount(int max);
	void draw(string textureName);
	void initFrameResource(EffectFrameResource * effectFrameResource, int frameIndex);
	// Inherited via EffectBase
	virtual int neededCommandSlots() override {
		return 1;
	}
private:
	XApp *xapp = nullptr;
	// used to resize WorkerCommands vector
	int threadCount;
	vector<WorkerCopyTextureCommand> worker;
};