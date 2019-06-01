#pragma once
// pipeline queue for runner thread:
class PipelineQueue {
public:
	// wait until next frame has finished rendering
	Frame* pop() {
		unique_lock<mutex> lock(monitorMutex);
		while (myqueue.empty()) {
			cond.wait_for(lock, chrono::milliseconds(3000));
			LogF("PipelineQueue wait suspended\n");
			if (in_shutdown) {
				LogF("PipelineQueue shutdown in pop\n");
				return nullptr;
			}
		}
		assert(myqueue.empty() == false);
		Frame * frame = myqueue.front();
		myqueue.pop();
		cond.notify_one();
		return frame;
	}

	// push finished frame
	void push(Frame *frame) {
		unique_lock<mutex> lock(monitorMutex);
		if (in_shutdown) {
			throw "RenderQueue shutdown in push";
		}
		myqueue.push(frame);
		LogF("PipelineQueue length " << myqueue.size() << endl);
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
	queue<Frame*> myqueue;
	mutex monitorMutex;
	condition_variable cond;
	bool in_shutdown{ false };
};

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
	void setFrameBufferSize(size_t size) { frameBufferSize = size; };
	size_t getFrameBufferSize() { return frameBufferSize; };
	// all drawing takes place in backbuffer - output to whatever size the window currently has is only last step
	unsigned int backbufferWidth = 0, backbufferHeight = 0;
	float aspectRatio;
private:
	// world size in absolute units around origin, e.g. x is from -x to x
	float sizex = 0.0f, sizey = 0.0f, sizez = 0.0f;
	int wait_timeout_ms = 100; // check for interruption 10 times per second
	size_t frameBufferSize = 0;
};

class Pipeline
{
public:
	Pipeline();
	virtual ~Pipeline();
	PipelineConfig& getPipelineConfig() { return pipelineConfig; }
	// Initialize default pipeline with framecount 0 and one FrameBuffer
	void init();
	// return next frame number. Frame numbers ar increased by 1 with every call to this method. 
	// Only be called once for every frame
	long long getNextFrameNumber() { return frameNum++; }
	// return current frame number. This is the highest frame number that has not yet been finally processed.
	long long getCurrentFrameNumber() { return frameNum; }
	// signal that a frame has been fully processed and all associated resources can be freed
	void finallyProcessed(Frame* frame);
	// wait for a specific frame to be ready for consumption. May wait forever if this frame is never generated.
	// removed: application code must be able to deal with any returned frame...
	//Frame* waitForFinishedFrame(long long frameNum);

	// Is this pipeline running? 
	boolean isRunning() { return running; }
	// set run mode
	void setRunning(boolean isRunning) { running = isRunning; }
	// Is pipeline in shutdown mode? 
	boolean isShutdown() { return shutdown_mode; }
	// enable shutdown mode: The run thread will dry out and terminate
	void shutdown() { shutdown_mode = true; queue.shutdown(); }
	// start the processing thread in the background and return immediately. May only be called once
	static void run(Pipeline* pipeline_instance);
	// wait until next frame is available to render to, null returned on shutdown
	Frame* getNextFrameSlot() { return frameBuffer.getNextFrameSlot(); }
	// add a fully rendered frame to the framebuffer
	void pushRenderedFrame(Frame* frame) { queue.push(frame); }
	size_t currentlyFreeSlots() {
		return frameBuffer.currentlyFreeSlots();
	}
	// set function to be called after frame has been fully created.
	// the pipeline will make sure that calls to this method will be synchronized (only 1 thread at a time)
	void setFinishedFrameConsumer(function<void(Frame*, Pipeline*)> consumer) { this->consumer = consumer; }
	void setApplicationFrameData(void* data) { applicationFrameData = data; };
	void setCallbackDraw(function<void(Frame*, Pipeline*, void* afd)> callback) { this->drawCallback = callback; }
	// start rendering 
	void startRenderThreads();
	// Wait until pipeline has ended rendering. Needed for console apps that have no event loop
	void waitUntilShutdown();
	// get cumulated statistics message
	string getStatistics() { stringstream s; s << "created " << (frameNum + 1) << " frames with average [microseconds] to render: " << averageFrameRenderDuration << " tot FPS: " << totalFPS << " skipped " << skipped << endl; return s.str(); }
	void updateStatistics(Frame* frame);
	AppFrameDataManager afManager;
	long long lastFrameRenderDuration = 0L; // microseconds
	long totalFPS = 0; // FPS since starting render threads (skipped frames do not count)
private:
	// Pipeline part of creating a frame
	static void runFrameSlot(Pipeline* pipeline, Frame* frame, int slot);
	PipelineQueue queue;
	FrameBuffer frameBuffer;
	PipelineConfig pipelineConfig;
	atomic<long long> frameNum = 0;
	boolean running = false;
	boolean shutdown_mode = false;
	function<void(Frame*, Pipeline*)> consumer = nullptr;
	function<void(Frame*, Pipeline*, void* afd)> drawCallback = nullptr;
	void* applicationFrameData = nullptr; // used to pass pointer of app data back during callbacks
	boolean initialized = false;
	ThreadGroup threads;
	long long averageFrameRenderDuration = 0L; // microseconds
	long long cumulatedFrameRenderDuration = 0L; // microseconds
	mutex appSyncMutex; // to synchronize callbacks to application
	boolean inSyncCode = false;  // indicate single thread access code, check this var before global operations
	chrono::time_point<chrono::high_resolution_clock> pipelineStartTime;
	long long skipped = 0; // count skipped frames
	long long last_processed = -1; // last processed frame number
protected:
};

