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

void Pipeline::waitForFinishedFrame(long long frameNum)
{
	if (!running) {
		LogF("Pipeline not running\n");
	}
}

void Pipeline::run()
{
}