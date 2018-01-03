// test worker sub types
class WorkerCopyTextureCommand : public WorkerCommand {
public:
	string textureName;
	void perform();
};