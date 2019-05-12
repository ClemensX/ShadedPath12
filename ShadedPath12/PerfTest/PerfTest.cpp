// PerfTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"
#include "PerfTest.h"

#define FRAME_BUFFER_SIZE 3
#define FRAMES_COUNT 1000

// TODO now orchestrate frame creation in multiple tests

// run tests with NUM_SLOTS sized frame buffer
void initPipeline(Pipeline& pipeline) {
	auto& pc = pipeline.getPipelineConfig();
	pc.setWorldSize(2048.0f, 382.0f, 2048.0f);
	pc.setFrameBufferSize(FRAME_BUFFER_SIZE);
	pipeline.init();
	LogF("pipeline initiaized via LogF" << endl);
}

// static void methods are used in threaded code
static mutex monitorMutex;
static bool in_shutdown{ false };
static long long skipped = 0; // count skipped frames
static long long last_processed = -1; // last processed frame number

// after a frame has been processed this is called to consume it
// (present or store usually)
// after that the buffer slot should be freed
static void presentFrame(Frame* frame) {
	unique_lock<mutex> lock(monitorMutex);
	//cout << "present frame slot " << frame->slot << " frame " << frame->absFrameNumber << endl;
	if (frame->absFrameNumber < last_processed) {
		// received an out-of-order frame: discard
		skipped++;
		return;
	}
	// normal in-order frame: process
	cout << "present frame slot " << frame->slot << " frame " << frame->absFrameNumber << endl;
	// TODO
	last_processed = frame->absFrameNumber;
}

static void runFrameSlot(Pipeline *pipeline, Frame* frame, int slot) {
	boolean shutdown = false;
	while (!shutdown) {
		auto frameNum = pipeline->getNextFrameNumber();
		// if next line is commentd out we see garbled text because of multile threads writing
		//cout << "run frame slot " << slot << " frame " << frameNum << endl;
		frame->absFrameNumber = frameNum;
		frame->slot = slot;
		// frame now considered processed
		// call synchronized present method
		presentFrame(frame);
		if (frameNum >= (FRAMES_COUNT-1)) {
			shutdown = true;
		}
	}
}

int main()
{
    std::cout << "ShadedPath12 Performance Tests\n"; 
	Pipeline pipeline;
	initPipeline(pipeline);
	ThreadGroup threads;
	//threads.add_t(Pipeline::run, &pipeline);
	//cout << "pipeline thread started " << endl;

	//threads.add_t(produceFrames, &pipeline);
	//threads.add_t(consumeFrames, &pipeline);

	// start timer
	auto t0 = chrono::high_resolution_clock::now();
	// create as many frames as we will have parallel threads producing them
	Frame frames[3];
	threads.add_t(runFrameSlot, &pipeline, &frames[0], 0);
	threads.add_t(runFrameSlot, &pipeline, &frames[1], 1);
	threads.add_t(runFrameSlot, &pipeline, &frames[2], 2);
	//this_thread::sleep_for(chrono::milliseconds(200));
	threads.join_all();
	// stop timer
	auto t1 = chrono::high_resolution_clock::now();
	cout << "Empty Frame throughput ( " << FRAMES_COUNT << " frames): " << chrono::duration_cast<chrono::milliseconds>(t1 - t0).count() << " ms\n";
	cout << "  Skipped out-of-order frames: " << skipped << endl;
}

