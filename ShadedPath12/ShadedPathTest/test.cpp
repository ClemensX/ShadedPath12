#include "pch.h"

TEST(TestCaseName, TestName) {
  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}

TEST(RenderThreads, Init) {
	WorkerQueue workerQueue;
	workerQueue.init(1, 1, 1);
	EXPECT_TRUE(true);
}