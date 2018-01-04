// test worker sub types
class WorkerCopyTextureCommand : public WorkerCommand {
public:
	string textureName;
	void perform();
};

class CopyTextureEffect {
public:
	void init();
	void setThreadCount(int max);
	void draw();
private:
	XApp *xapp = nullptr;
	// used to resize WorkerCommands vector
	int threadCount;
	vector<WorkerCopyTextureCommand> worker;
};