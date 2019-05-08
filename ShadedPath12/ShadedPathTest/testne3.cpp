#include "pch.h"
#include "testne3.h"

TEST(TestNewEngine, Empty) {
  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}

// run tests with a 10 frame buffer
void initPipeline(Pipeline& pipeline) {
	auto& pc = pipeline.getPipelineConfig();
	pc.setWorldSize(2048.0f, 382.0f, 2048.0f);
	pc.setFrameBufferSize(10);
	pipeline.init();
}

TEST(TestNewEngine, Init) {
	Pipeline pipeline;
	initPipeline(pipeline);
	auto pc = pipeline.getPipelineConfig();
	EXPECT_EQ(2048.0f, pc.getSizeX());
	EXPECT_EQ(382.0f, pc.getSizeY());
}

TEST(TestNewEngine, ShutdownWhileWaitingForFrame) {
	Pipeline pipeline;
	initPipeline(pipeline);
	long long curFrame = pipeline.getCurrentFrameNumber();
	EXPECT_EQ(0LL, curFrame);
	ThreadGroup threads;
	threads.add_t(Pipeline::run, &pipeline);
	this_thread::sleep_for(chrono::milliseconds(50));
	EXPECT_TRUE(pipeline.isRunning());

	// cleanup and shutdown
	pipeline.shutdown();
	// we need to wait until all threads are terminated
	this_thread::sleep_for(chrono::milliseconds(50));
	EXPECT_FALSE(pipeline.isRunning());
	LogF("finished" << endl);
}

TEST(TestNewEngine, FrameCreation) {
	Pipeline pipeline;
	initPipeline(pipeline);
	long long curFrame = pipeline.getCurrentFrameNumber();
	EXPECT_EQ(0LL, curFrame);
	ThreadGroup threads;
	threads.add_t(Pipeline::run, &pipeline);
	this_thread::sleep_for(chrono::milliseconds(50));
	EXPECT_TRUE(pipeline.isRunning());
	// all up and running - now produce a frame
	Frame* frame = pipeline.getNextFrame();
	frame->absFrameNumber = curFrame;
	pipeline.pushRenderedFrame(frame);
	this_thread::sleep_for(chrono::milliseconds(50));

	// consume frame
	Frame* renderedFrame = pipeline.getNextFrame();
	EXPECT_NE(nullptr, renderedFrame);
	EXPECT_EQ(0LL, renderedFrame->absFrameNumber);

	// cleanup and shutdown
	pipeline.shutdown();

}
