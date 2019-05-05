#include "pch.h"
#include "testne3.h"

TEST(TestNewEngine, Empty) {
  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}

void initPipeline(Pipeline& pipeline) {
	auto& pc = pipeline.getPipelineConfig();
	pc.setWorldSize(2048.0f, 382.0f, 2048.0f);
	pipeline.init();
}

TEST(TestNewEngine, Init) {
	Pipeline pipeline;
	initPipeline(pipeline);
	auto pc = pipeline.getPipelineConfig();
	EXPECT_EQ(2048.0f, pc.getSizeX());
	EXPECT_EQ(382.0f, pc.getSizeY());
}

TEST(TestNewEngine, FrameCreation) {
	Pipeline pipeline;
	initPipeline(pipeline);
	long long curFrame = pipeline.getCurrentFrameNumber();
	EXPECT_EQ(0LL, curFrame);
	pipeline.run();
	EXPECT_TRUE(pipeline.isRunning());
	pipeline.waitForFinishedFrame(curFrame);

}
