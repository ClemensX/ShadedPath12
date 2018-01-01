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
	// run the content of this command in a thread
	void task(XApp *xapp);
	virtual ~Command() {};
};

class RenderCommand : public Command {
public:
	ID3D12GraphicsCommandList * commandList = nullptr;
};

// each effect stores an array of its details objects that have the info on what do do
class WorkerCommand : public Command {
public:
	void * commandDetails = nullptr;

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

private:
	queue<RenderCommand> myqueue;
	mutex monitorMutex;
	condition_variable cond;
	bool in_shutdown{ false };
};

class WorkerQueue {
public:
	WorkerCommand pop() {
		unique_lock<mutex> lock(monitorMutex);
		while (myqueue.empty()) {
			cond.wait(lock);
			if (in_shutdown) {
				throw "WorkerQueue shutdown in pop";
			}
		}
		assert(myqueue.empty() == false);
		WorkerCommand workerCommand = myqueue.front();
		myqueue.pop();
		cond.notify_one();
		return workerCommand;
	}

	void push(WorkerCommand workerCommand) {
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
	queue<WorkerCommand> myqueue;
	mutex monitorMutex;
	condition_variable cond;
	bool in_shutdown{ false };
};

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

	std::size_t size() const { return threads.size(); }
private:
	std::vector<std::thread> threads;
};