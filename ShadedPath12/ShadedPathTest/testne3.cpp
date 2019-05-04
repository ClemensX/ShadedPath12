#include "pch.h"
#include "../ShadedPath12/pipeline.h"

TEST(TestNewEngine3, Empty) {
  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}

TEST(TestNewEngine3, Init) {
	Pipeline pipeline;
	auto pc = pipeline.getPipelineConfig();
	pc.setWorldSize(2048.0f, 382.0f, 2048.0f);
	EXPECT_EQ(2048.0f, pc.getSizeX());
	EXPECT_EQ(382.0f, pc.getSizeY());
}
