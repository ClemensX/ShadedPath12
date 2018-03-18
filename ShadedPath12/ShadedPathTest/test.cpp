#include "pch.h"
#include "test.h"

TEST(TestCaseName, TestName) {
  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}

TEST(RenderThreads, Init) {
	WorkerQueue workerQueue;
	workerQueue.init(1, 1, 1);
	EXPECT_TRUE(true);
}

TEST(NewQueue, Basic) {
	SingleQueue queue;
	EXPECT_EQ(queue.getState(), QueueState::Undefined);
	queue.sync();
	EXPECT_EQ(queue.getState(), QueueState::Synced);
}

class WorkerTestCommand : public WorkerCommand {
public:
	void perform() {
		Sleep(5);
	};
	ResourceStateHelper *resourceStateHelper = nullptr;
};

ThreadGroup workerThreads;
int numThreads = 1;

static void testTask(SingleQueue *queue) {
	try {
		//Sleep(5000);
		bool cont = true;
		while (cont) {
			WorkerCommand *command = queue->pop();
			if (command)
				command->perform();
			queue->endCommand(command);
		}
	}
	catch (char *s) {
		std::cerr << "task finshing due to exception: " << s << endl;
	}

}

void init(SingleQueue *queue) {
	for (int i = 0; i < numThreads; i++) {
		std::cerr << "start thread " << i << std::endl;
		workerThreads.add_t(testTask, queue);
	}
}

TEST(NewQueue, FullCircle) {
	std::cerr << "start FullCircle test " << std::endl;
	SingleQueue queue;
	queue.sync();
	EXPECT_EQ(queue.getState(), QueueState::Synced);
	WorkerTestCommand cmd;
	init(&queue);
	queue.push(&cmd);
	Sleep(1);
	queue.shutdown();
	Sleep(100);
	ASSERT_TRUE(true);
}