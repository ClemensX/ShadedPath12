// test worker sub types
class WorkerCopyTextureCommand : public WorkerCommand {
public:
	string textureName;
	void perform();
	XApp *xapp = nullptr;
	ResourceStateHelper *resourceStateHelper = nullptr;
	AppWindowFrameResource *frameResource;
};

class CopyTextureEffect {
public:
	void init();
	void setThreadCount(int max);
	void draw(string textureName);
private:
	XApp *xapp = nullptr;
	// used to resize WorkerCommands vector
	int threadCount;
	vector<WorkerCopyTextureCommand> worker;
};