#include "pch.h"
#include "testne3.h"

TEST(TestNewEngine, Empty) {
  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);
}

#define NUM_SLOTS 100

// run tests with NUM_SLOTS sized frame buffer
void initPipeline(Pipeline& pipeline) {
	auto& pc = pipeline.getPipelineConfig();
	pc.setWorldSize(2048.0f, 382.0f, 2048.0f);
	pc.setFrameBufferSize(NUM_SLOTS);
	pc.backbufferWidth = 200;
	pc.backbufferHeight = 100;
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
	Frame* frame = pipeline.getNextFrameSlot();
	frame->absFrameNumber = curFrame;
	pipeline.pushRenderedFrame(frame);
	this_thread::sleep_for(chrono::milliseconds(50));

	// consume frame
	Frame* renderedFrame = pipeline.getNextFrameSlot();
	EXPECT_NE(nullptr, renderedFrame);
	EXPECT_EQ(0LL, renderedFrame->absFrameNumber);

	// cleanup and shutdown
	pipeline.shutdown();

}

TEST(TestNewEngine, FrameBuffer) {
	Pipeline pipeline;
	initPipeline(pipeline);
	ThreadGroup threads;
	threads.add_t(Pipeline::run, &pipeline);
	this_thread::sleep_for(chrono::milliseconds(50));
	EXPECT_TRUE(pipeline.isRunning());
	// all up and running - now produce and consume frames
	Frame* frames[NUM_SLOTS];
	for (int i = 0; i < NUM_SLOTS; i++) {
		frames[i] = pipeline.getNextFrameSlot();
		long long curFrame = pipeline.getCurrentFrameNumber();
		frames[i]->absFrameNumber = curFrame;
		//pipeline.pushRenderedFrame(frame);
	}
	EXPECT_EQ(0, pipeline.currentlyFreeSlots());
	// produce frames:
	for (int i = 0; i < NUM_SLOTS; i++) {
		pipeline.pushRenderedFrame(frames[i]);
	}
	// consume frames:
	for (int i = 0; i < NUM_SLOTS; i++) {
		pipeline.finallyProcessed(frames[i]);
	}
	EXPECT_EQ(NUM_SLOTS, pipeline.currentlyFreeSlots());
	this_thread::sleep_for(chrono::milliseconds(50));

	// cleanup and shutdown
	pipeline.shutdown();

}

/*// need full blown running env - disable this est for now, maybe activate it later...
TEST(TestNewEngine, Billboard) {
	Pipeline pipeline;
	initPipeline(pipeline);
	DXGlobal dxGlobal;
	dxGlobal.init();
	Billboard b;
	BillboardAppFrameData afd[3];
	// init effect
	VR vr;
	dxGlobal.vr = &vr;
	int i = 0;
	pipeline.afManager.setAppDataForSlot(&afd[i], i);
	BillboardAppFrameData* fd = (BillboardAppFrameData*)pipeline.afManager.getAppDataForSlot(i);
	Dx2D* d2d = &fd->d2d;
	FrameDataD2D* fd2d = &fd->d2d_fd;
	FrameDataGeneral* fd_gen = &fd->fd_general;
	FrameDataBillboard* fdb = &fd->billboard_fd;
	dxGlobal.initFrameBufferResources(fd_gen, fd2d, i, &pipeline);
	d2d->init(&dxGlobal, fd2d, fd_gen, &pipeline);
	b.init(&dxGlobal, fdb, fd_gen, &pipeline);
	// end effect init

	BillboardElement be1{ {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {1.0f, 2.0f} }; // pos, normal, size
	BillboardElement be2{ {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {1.0f, 2.0f} }; // pos, normal, size
	BillboardElement be3{ {2.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {1.0f, 2.0f} }; // pos, normal, size
	unsigned long user = 0;
	b.updateQueue.getLockedInactiveDataSet(user);
	size_t n1 = b.add("tex_01", be1, user);
	size_t n2 = b.add("tex_01", be2, user);
	size_t n3 = b.add("tex_02", be3, user);

	// order numbers are counted per texture:
	EXPECT_EQ(0, n1);
	EXPECT_EQ(1, n2);
	EXPECT_EQ(0, n3);

	b.activateAppDataSet(user);
	b.updateQueue.releaseLockedInactiveDataSet(user);
	EXPECT_NE(nullptr, b.getInactiveAppDataSet(user));
	EXPECT_THROW(b.get("tex_01", 0, user), std::exception);
	//auto v = b.get("tex_01", 0);
	n1 = b.add("tex_01", be1, user);
	EXPECT_EQ(0, n1);  // did we really restart from 0?
	LogF("billboard finished" << endl);	//b.get()
}
*/