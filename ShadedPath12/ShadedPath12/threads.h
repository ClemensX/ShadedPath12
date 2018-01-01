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
	void execute();
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

// from 'Der C++ Programmierer', Listing 14.7, ISBN 978-3-446-44346-4
class ThreadGroup {
public:
	ThreadGroup() = default;
	ThreadGroup(const ThreadGroup&) = delete;

	~ThreadGroup() {
		for (const auto& pthread : pthreads) {
			delete pthread;
		}
	}

	ThreadGroup& operator=(const ThreadGroup&) = delete;

	// create thread and start
	template<typename F>
	void add_thread(F& f) {
		pthreads.push_back(new thread(ref(f)));
	}

	// wait for all threads to finish
	void join_all() {
		for (const auto& pthread : pthreads) {
			pthread->join();
		}
	}

	size_t size() const {
		return pthreads.size();
	}

private:
	vector<thread*> pthreads;
};