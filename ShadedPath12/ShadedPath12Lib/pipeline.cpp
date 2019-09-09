#include "stdafx.h"


PipelineConfig::PipelineConfig()
{
}


PipelineConfig::~PipelineConfig()
{
}

Pipeline::Pipeline()
{
}


Pipeline::~Pipeline()
{
}

void Pipeline::init()
{
	auto pc = getPipelineConfig();
	// some plausibility checks:
	if (pc.getFrameBufferSize() <= 0) Error(L"Frame buffer size cannot be 0");
	if (pc.backbufferWidth <= 0) Error(L"backbuffer width cannot be 0");
	if (pc.backbufferHeight <= 0) Error(L"backbuffer height cannot be 0");

	frameBuffer.resize(getPipelineConfig().getFrameBufferSize());
	Log("Pipeline::init" << endl);
	LogF("Pipeline::init with LogF" << endl);
	initialized = true;
	world.setWorldSize(pc.getSizeX(), pc.getSizeY(), pc.getSizeZ());
	vrMode = pc.getVRMode();
	hmdMode = pc.getHMDMode();
	singleThreadMode = pc.getSingleThreadMode();
	gametime.init(pc.getGamedayFactor());
	SetThreadDescription(GetCurrentThread(), L"main_pipeline_thread");
}

void Pipeline::finallyProcessed(Frame* frame)
{
	frameBuffer.returnFrame(frame);
}

/*Frame* Pipeline::waitForFinishedFrame(long long frameNum)
{
	if (!running) {
		LogF("waitForFinishedFrame: pipeline not running\n");
	}
	return nullptr;
}*/

// run pipeline, expected to be called in its own thread
// wait on queue until new frame is ready and return it (as a pointer)
// frames are always pointers into the frame buffer - you should never copy a frame instance
void Pipeline::run(Pipeline* pipeline_instance)
{
	pipeline_instance->setRunning(true);
	LogF("pipeline run" << endl);
	while (pipeline_instance->isShutdown() == false) {
		auto v = pipeline_instance->queue.pop();
		if (v == nullptr) {
			LogF("pipeline shutdown" << endl);
			break;
		}
		LogF("pipeline received frame: " << v->absFrameNumber << endl);
		//pipeline_instance->frameBuffer.getNextFrameSlot();
	}
	pipeline_instance->setRunning(false);
}

void Pipeline::runFrameSlot(Pipeline* pipeline, Frame* frame, int slot)
{
	while (!pipeline->isShutdown()) {
		auto frameNum = pipeline->getNextFrameNumber();
		//LogF("Pipeline::runFrameSlot " << frameNum << endl);
		// if next line is commentd out we see garbled text because of multile threads writing
		//cout << "run frame slot " << slot << " frame " << frameNum << endl;
		frame->renderStartTime = chrono::high_resolution_clock::now();
		frame->gametime = pipeline->gametime.getTimeAbs();
		frame->absFrameNumber = frameNum;
		frame->slot = slot;
		frame->frameData = pipeline->afManager.getAppDataForSlot(slot);
		// let application draw:
		
		pipeline->drawCallback(frame, pipeline, pipeline->applicationFrameData);
		pipeline->updateStatisticsDraw(frame);
		bool unfinished = true;
		// frame now considered processed
		// call synchronized present method
		while (unfinished)
		{
			unique_lock<mutex> lock(pipeline->appSyncMutex);
			pipeline->inSyncCode = true;
			//pipeline->drawCallback(frame, pipeline, pipeline->applicationFrameData);
			bool skippedDueToWVP = false;
			if (pipeline->lastWvpId+1 != frame->wvpId) {
				Log("WVP MISMATCH: last id: " << pipeline->lastWvpId << " cur frame: " << frame->wvpId << endl);
				skippedDueToWVP = true;
			}
			pipeline->lastWvpId = frame->wvpId;
			if (pipeline->lastWVPTime >= frame->wvpTime) {
				//Log("WVP OUT OF ORDER: " << frame->absFrameNumber << " last time: " << pipeline->lastWVPTime << " cur frame: " << frame->wvpTime << endl);
				//skippedDueToWVP = true;
			}
			pipeline->lastWVPTime = frame->wvpTime;
			if (pipeline->lastFrameGametime >= frame->gametime) {
				//Log("Frame OUT OF ORDER: " << frame->absFrameNumber << endl);
			}
			pipeline->lastFrameGametime = frame->gametime;
			if (frame->absFrameNumber < pipeline->last_processed || skippedDueToWVP) {
				// received an out-of-order frame: discard
				pipeline->skipped++;
			} else {
				pipeline->vr->startFrame();
				pipeline->consumer(frame, pipeline);
				pipeline->last_processed = frame->absFrameNumber;
			}
			pipeline->updateStatisticsPresent(frame);
			pipeline->inSyncCode = false;
			unfinished = false;
			pipeline->gametime.advanceTime();
		}
		//pipeline->updateStatistics(frame);
	}
}

void Pipeline::runUpdate(Pipeline* pipeline)
{
	while (!pipeline->isShutdown()) {
		//pipeline->gametime.advanceTime(); // TODO thread safe?  - apparently not
		pipeline->updateCallback(pipeline);
	}
}

void Pipeline::updateStatisticsDraw(Frame* frame)
{
	auto t1 = chrono::high_resolution_clock::now();
	frame->drawDuration = chrono::duration_cast<chrono::microseconds>(t1 - frame->renderStartTime).count();
	//Log(" frame draw " << frame->drawDuration << endl);
}

void Pipeline::updateStatisticsPresent(Frame* frame)
{
	assert(inSyncCode);
	// finsh draw statistics
	lastFrameDrawDuration = frame->drawDuration;
	cumulatedFrameDrawDuration += frame->drawDuration;
	//Log("cumulated " << cumulatedFrameRenderDuration);
	averageFrameDrawDuration = cumulatedFrameDrawDuration / (frame->absFrameNumber + 1);
	//Log(" average " << averageFrameRenderDuration << endl);

	// Present statistics
	auto t1 = chrono::high_resolution_clock::now();
	frame->renderDuration = chrono::duration_cast<chrono::microseconds>(t1 - frame->renderStartTime).count();
	lastFrameRenderDuration = frame->renderDuration;
	cumulatedFrameRenderDuration += frame->renderDuration;
	//Log("cumulated " << cumulatedFrameRenderDuration);
	averageFrameRenderDuration = cumulatedFrameRenderDuration / (frame->absFrameNumber + 1);
	//Log(" average " << averageFrameRenderDuration << endl);
	//total processed frames:
	long long tot = (frame->absFrameNumber + 1) - skipped;
	// total run milli seconds:
	auto totalMs = chrono::duration_cast<chrono::milliseconds>(t1 - pipelineStartTime).count();
	if (tot > 0) {
		totalFPS = (long)((1000L * tot) / totalMs);
	}
}

void Pipeline::startRenderThreads()
{
	if (!initialized) {
		Error(L"cannot start render threads: pipeline not initialized\n");
		return;
	}
	if (consumer == nullptr) {
		Error(L"cannot start render threads: no frame consumer specified\n");
		return;
	}
	pipelineStartTime = chrono::high_resolution_clock::now();
	if (singleThreadMode) {
		for (int i = 0; i < 1; i++) {
			threads.add_t(runFrameSlot, this, frameBuffer.getFrame(i), i);
		}
	}
	else {
		for (int i = 0; i < frameBuffer.size(); i++) {
			void* native_handle = threads.add_t(runFrameSlot, this, frameBuffer.getFrame(i), i);
			wstring mod_name = wstring(L"render_pipeline_thread").append(L"_").append(to_wstring(i));
			SetThreadDescription((HANDLE)native_handle, mod_name.c_str());
		}
	}
}

void Pipeline::startUpdateThread()
{
	if (!initialized) {
		Error(L"cannot start update thread: pipeline not initialized\n");
		return;
	}
	if (updateCallback == nullptr) {
		Error(L"cannot start update thread: no update consumer specified\n");
		return;
	}
	// one update thread for application code:
	void* native_handle = threads.add_t(runUpdate, this);
	wstring mod_name = wstring(L"update_application");//.append(L"_").append(to_wstring(i));
	SetThreadDescription((HANDLE)native_handle, mod_name.c_str());
}

void Pipeline::waitUntilShutdown()
{
	threads.join_all();
}