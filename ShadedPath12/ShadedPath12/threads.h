/*
 * Threads, ThreadGroups and Queues (Monitors)
 * We have two queues: 
 * RenderQueue is filled from all worker threads when they have finished filling command lists,
 *   one thread is taking the command lists and executes them
 * WorkerQueue is filled by the main effects with instructions waht to actually do
 *   one worker thread from the queue is chosen to run the instructions. Usually ending by filling the RenderQueue
 */

// commands:

enum CommandType { RenderCommandList, WorkerCopyTexture };

class Command {
public:
	CommandType type;
	// worker thread for running effect commands continuously
	static void task(XApp *xapp);
	// single worker thread for rendering to presentation back buffer
	static void renderQueueTask(XApp *xapp);
	virtual void perform() { Log("perform base" << endl); };
	virtual ~Command() {};
	Command() {};
	// each command must know on which frame (0..2) it operates
	int draw_slot = -1;
	// store absolute frame num to be able to sync on rendering
	long long absFrameCount = -1;
protected:
	// allow copy and move only for children (prevent object slicing)
	Command(Command&&) = default;
	Command(Command const&) = default;
	Command& operator=(const Command&) = default;
};

class RenderCommand : public Command {
public:
	ID3D12GraphicsCommandList * commandList = nullptr;
	bool writesToSwapchain = false;
	unsigned int frameIndex;
	AppWindowFrameResource *frameResource;
};

// each effect stores an array of its details objects that have the info on what do do
class WorkerCommand : public Command {
public:
	XApp * xapp = nullptr;
	WorkerThreadState requiredThreadState;
	EffectFrameResource * effectFrameResource;
	int commandIndex; // index to WorkerQueue
	int renderPhase;  // connect to RenderPlan: phase number, when this command should be excecuted
	// check if this command has the right required state compared to current effectFrameResouce state
	// will be pushed back to queue if not
	bool isValidSequence();
	void addPushedBackCount();
};

class RenderQueue {
public:
	RenderCommand pop() {
		unique_lock<mutex> lock(monitorMutex);
		while (myqueue.empty()) {
			cond.wait(lock);
			if (in_shutdown) {
				throw "RenderQueue shutdown in pop";
			}
		}
		assert(myqueue.empty() == false);
		RenderCommand renderCommand = myqueue.front();
		myqueue.pop();
		cond.notify_one();
		return renderCommand;
	}

	void push(RenderCommand renderCommand) {
		unique_lock<mutex> lock(monitorMutex);
		if (in_shutdown) {
			throw "RenderQueue shutdown in push";
		}
		myqueue.push(renderCommand);
		cond.notify_one();
	}

	void shutdown() {
		in_shutdown = true;
		cond.notify_all();
	}

	size_t size() {
		return myqueue.size();
	}
private:
	queue<RenderCommand> myqueue;
	mutex monitorMutex;
	condition_variable cond;
	bool in_shutdown{ false };
};

// helper struct of data needed for queue handling (one for each frame)
struct QueueFrameState {
	WorkerThreadState state;
	long long absFrameCount = -1;
	list<int> initSlots;
	list<int> renderSlots;
	list<int> finalizeSlots;
	list<int> working;
};

// we need one queue for all frames, to be able to assign a free thread slot efficiently
class WorkerQueue {
public:
	// set number of slots to max thread count
	void init(int maxSlots, int maxFrames, int neededCommandSlots) {
		// we need at least frames * neededCommandSlots places to store commands
		int commandSlots = maxFrames * neededCommandSlots;
		commands.resize(commandSlots);
		for (int i = 0; i < commandSlots; i++) {
			freeSlots.push_front(i);
		}
		qframeStates.resize(maxFrames);
		for (auto& state : qframeStates) {
			state.state = WorkerThreadState::InitFrame;
		}
	};
	WorkerCommand* pop();
	void push(WorkerCommand *workerCommand);
	void endCommand(WorkerCommand *workerCommand);
	void shutdown() {
		in_shutdown = true;
		cond.notify_all();
	}
private:
	// all lists index into command vector:
	vector<WorkerCommand*> commands;
	vector<QueueFrameState> qframeStates;
	list<int> freeSlots;
	mutex monitorMutex;
	condition_variable cond;
	bool in_shutdown{ false };
	int handleRenderSlot();
	int handleInitSlot();
	bool isSlotAvailable();
};

class OldWorkerQueue {
public:
	WorkerCommand* pop() {
		unique_lock<mutex> lock(monitorMutex);
		bool valid = true;
		WorkerCommand *workerCommand;
		do {
			while (myqueue.empty()) {
				cond.wait(lock);
				if (in_shutdown) {
					throw "WorkerQueue shutdown in pop";
				}
			}
			assert(myqueue.empty() == false);
			workerCommand = myqueue.front();
			myqueue.pop();
			if (!workerCommand->isValidSequence()) {
				valid = false;
				// TODO this is practically active wait - use backup queue or else...
				myqueue.push(workerCommand);
			 	workerCommand->addPushedBackCount();
			} else {
				valid = true;
			}
		} while (!valid);
		cond.notify_one();
		return workerCommand;
	}

	void push(WorkerCommand *workerCommand) {
		unique_lock<mutex> lock(monitorMutex);
		if (in_shutdown) {
			throw "WorkerQueue shutdown in push";
		}
		myqueue.push(workerCommand);
		cond.notify_one();
	}

	void shutdown() {
		in_shutdown = true;
		cond.notify_all();
	}

private:
	queue<WorkerCommand*> myqueue;
	mutex monitorMutex;
	condition_variable cond;
	bool in_shutdown{ false };
};

// currently size() is not guarded against xapp->getMaxThreadCount(), but fails on using comand vectors if index too high
class ThreadGroup {
public:
	ThreadGroup() = default;
	ThreadGroup(const ThreadGroup&) = delete;
	ThreadGroup& operator=(const ThreadGroup&) = delete;

	// store and start Thread
	template <class Fn, class... Args>
	void add_t(Fn&& F, Args&&... A) {
		threads.emplace_back(std::thread(F, A...));
	}

	// wait for all threads to finish
	void join_all() {
		for (auto& t : threads) {
			t.join();
		}
	}

	~ThreadGroup() {
		join_all();
	}

	std::size_t size() const { return threads.size(); }
private:
	std::vector<std::thread> threads;
};

class ThreadInfo {
public:
	static DWORD thread_osid() {
		DWORD osid = GetCurrentThreadId();
		return osid;
	}
};

enum FrameState { Free, Drawing, Executing };

// hold all infos needed to syncronize render and command threads
class ThreadState {
private:
	mutex stateMutex;
	condition_variable drawSlotAvailable;
	FrameState frameState[DXManager::FrameCount];
public:
	void init() {
		for (int i = 0; i < DXManager::FrameCount; i++) {
			frameState[i] = Free;
		}
	};

	// wait until draw slot available and return slot index (0..2)
	// slots are returned in order: 0,1,2,0,1,2,...
	int waitForNextDrawSlot(int old_slot) {
		// lock needed for contition_variables
		unique_lock<mutex> myLock(stateMutex);
		// wait for next free slot
		int next_slot = old_slot + 1;
		if (next_slot >= DXManager::FrameCount) {
			next_slot = 0;
		}
		//Log("wait for next draw slot: " << next_slot << endl);
		drawSlotAvailable.wait(myLock, [this,next_slot]() {return frameState[next_slot] == Free; });

		// now we run exclusively - freely set any state
		//Log(" got next draw slot: " << next_slot << endl);
		assert(frameState[next_slot] == Free);
		frameState[next_slot] = Drawing;

		// leave critical section
		myLock.unlock();
		return next_slot;
	};

	// free draw slot. called right after swapChain->Present()
	void freeDrawSlot(int i) {
		// lock needed for contition_variables
		unique_lock<mutex> myLock(stateMutex);
		// now we run exclusively - freely set any state
		// free given slot:
		assert(frameState[i] == Drawing);
		frameState[i] = Free;

		// leave critical section
		myLock.unlock();
		//Log(" freed draw slot: " << i << endl);
		// notify waiting threads that new slot is available
		drawSlotAvailable.notify_one();
	};
};