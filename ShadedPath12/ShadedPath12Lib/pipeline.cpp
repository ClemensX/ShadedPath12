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
	frameBuffer.resize(getPipelineConfig().getFrameBufferSize());
	Log("Pipeline::init" << endl);
	LogF("Pipeline::init" << endl);
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