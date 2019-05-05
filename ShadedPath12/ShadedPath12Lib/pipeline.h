#pragma once
class PipelineConfig
{
public:
	PipelineConfig();
	virtual ~PipelineConfig();
	void setWorldSize(float x, float y, float z) { sizex = x; sizey = y; sizez = z; };
	float getSizeX() { return sizex; };
	float getSizeY() { return sizey; };
	float getSizeZ() { return sizez; };
	int getWaitTimeout() { return wait_timeout_ms; };
private:
	// world size in absolute units around origin, e.g. x is from -x to x
	float sizex = 0.0f, sizey = 0.0f, sizez = 0.0f;
	int wait_timeout_ms = 100; // check for interruption 10 times per second
};

class Pipeline
{
public:
	Pipeline();
	virtual ~Pipeline();
	PipelineConfig& getPipelineConfig() { return pipelineConfig; }
	// Initialize default pipeline with framecount 0 and one FrameBuffer
	void init();
	// return current frame number. This is the highest fame number that has not yet been finally processed.
	long long getCurrentFrameNumber() { return frameNum; }
	// signal that a frame has been fully processed and all associated resources can be freed
	void finallyProcessed(long long frameNumProcessed);
	// wait for a specific frame to be ready for consumption. May wait forever if this frame is never generated.
	void waitForFinishedFrame(long long frameNum);
	// Is this pipeline running? 
	boolean isRunning() { return running; }
	// enable shutdown mode: The run thread will dry out and terminate
	void shutdown() { shutdown_mode = true; }
	// start the processing thread in the background and return immediately. May only be called once
	void run();
private:
	PipelineConfig pipelineConfig;
	long long frameNum = 0;
	boolean running = false;
	boolean shutdown_mode = false;
};

// pipeline queue for runner thread:
class PipelineQueue {
public:
	RenderCommand pop() {
		unique_lock<mutex> lock(monitorMutex);
		while (myqueue.empty()) {
			cond.wait(lock);
			if (in_shutdown) {
				Log("PipelineQueue shutdown in pop");
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