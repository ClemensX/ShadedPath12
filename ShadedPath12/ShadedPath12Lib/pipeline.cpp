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
		// if next line is commentd out we see garbled text because of multile threads writing
		//cout << "run frame slot " << slot << " frame " << frameNum << endl;
		frame->renderStartTime = chrono::high_resolution_clock::now();
		frame->absFrameNumber = frameNum;
		frame->slot = slot;
		// frame now considered processed
		// call synchronized present method
		{
			unique_lock<mutex> lock(pipeline->appSyncMutex);
			pipeline->consumer(frame, pipeline);
		}
		//pipeline->updateStatistics(frame);
	}
}

void Pipeline::updateStatistics(Frame* frame)
{
	auto t1 = chrono::high_resolution_clock::now();
	frame->renderDuration = chrono::duration_cast<chrono::microseconds>(t1 - frame->renderStartTime).count();
	cumulatedFrameRenderDuration += frame->renderDuration;
	//Log("cumulated " << cumulatedFrameRenderDuration);
	averageFrameRenderDuration = cumulatedFrameRenderDuration / (frame->absFrameNumber + 1);
	//Log(" average " << averageFrameRenderDuration << endl);
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
	for (int i = 0; i < frameBuffer.size(); i++) {
		threads.add_t(runFrameSlot, this, frameBuffer.getFrame(i), i);
	}
}

void Pipeline::waitUntilShutdown()
{
	threads.join_all();
}