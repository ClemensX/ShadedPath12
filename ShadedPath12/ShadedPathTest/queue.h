#pragma once

enum QueueState {
	Undefined, // should never happen
	Running,   // any command doing some work can come in
	SyncRequest, // dry out commands until no one is running
	Synced       // all commands ended, save to copy results
};

// Execute a single RenderPlan from start to finish
// execute commands and sync them, threds are assigned per frame, no global thread pool for all frames
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
		cond.notify_all();
	}
private:
	QueueState state;
	list<WorkerCommand> commands; // all other lists contain indexes into this, grows with push() , shrinks with endCommand()
	list<int> running; // commands currently executing
	list<int> queuedCommands; // commands waiting for next phase(s)
	list<int> freeSlots; // free entries in commands list
	int renderPhase = 0; // each render phase is between syncs, we need to handle each phase separately
	unordered_map<int, int> renderCommandsPerPhase;
	int finishedThisPhase = 0; // count finished commands for this phase
	boolean in_shutdown = false;
	mutex monitorMutex;
	condition_variable cond;
	// re-evaluate state (called after every change to the queue)
	void stateUpdate();
};