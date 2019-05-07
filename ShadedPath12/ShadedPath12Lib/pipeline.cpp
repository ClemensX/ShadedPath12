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
}

void Pipeline::finallyProcessed(long long frameNumProcessed)
{
}

Frame* Pipeline::waitForFinishedFrame(long long frameNum)
{
	if (!running) {
		LogF("Pipeline not running\n");
	}
	return nullptr;
}

void Pipeline::run(Pipeline* pipeline_instance)
{
	pipeline_instance->setRunning(true);
	auto v = pipeline_instance->queue.pop();
	pipeline_instance->setRunning(false);
}