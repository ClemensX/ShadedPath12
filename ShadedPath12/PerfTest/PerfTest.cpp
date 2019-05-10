// PerfTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"
#include "PerfTest.h"

#define FRAME_BUFFER_SIZE 3
#define FRAMES_COUNT 10

// TODO now orchestrate frame creation in multiple tests

// run tests with NUM_SLOTS sized frame buffer
void initPipeline(Pipeline& pipeline) {
	auto& pc = pipeline.getPipelineConfig();
	pc.setWorldSize(2048.0f, 382.0f, 2048.0f);
	pc.setFrameBufferSize(FRAME_BUFFER_SIZE);
	pipeline.init();
	LogF("pipeline initiaized via LogF" << endl);
}

static void produceFrames(Pipeline* pipeline) {
	cout << "produce frames" << endl;
	for (int i = 0; i < FRAMES_COUNT; i++) {
		cout << "P " << i << endl;
		//pipeline->
	}
}

static void consumeFrames(Pipeline* pipeline) {
	cout << "consume frames" << endl;
	for (int i = 0; i < FRAMES_COUNT; i++) {
		cout << "C " << i << endl;
	}
}

int main()
{
    std::cout << "ShadedPath12 Performance Tests\n"; 
	Pipeline pipeline;
	initPipeline(pipeline);
	ThreadGroup threads;
	threads.add_t(Pipeline::run, &pipeline);
	cout << "pipeline thread started " << endl;

	threads.add_t(produceFrames, &pipeline);
	threads.add_t(consumeFrames, &pipeline);
}



// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
