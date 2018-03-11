#pragma once

enum QueueState {
	Undefined, // should never happen
	Running,   // any command doing some work can come in
	SyncRequest, // dry out sommands until no one is running
	Synced       // all commands ended, save to copy results
};

// execute commands and sync them
class SingleQueue {
public:
	SingleQueue();
	// sync will wait until all older commands have finished
	void sync();
	const QueueState getState() { return state; }
	const int getRenderPhase() { return renderPhase; }
	void push(WorkerCommand *cmd);
	WorkerCommand* pop();
	void endCommand(WorkerCommand *workerCommand);
	void shutdown() {
		in_shutdown = true;
		//cond.notify_all();
	}
private:
	QueueState state;
	list<WorkerCommand> commands; // all other lists contain indexes into this
	list<int> running; // commands currently executing
	list<int> queuedCommands; // commands waiting for next phase(s)
	list<int> freeSlots; // free entries in commands list
	int renderPhase = 0; // each render phase is between syncs, we need to handle each phase separately
	boolean in_shutdown = false;
};