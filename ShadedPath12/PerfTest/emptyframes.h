#pragma once

#define FRAME_BUFFER_SIZE 3
#define FRAMES_COUNT 1000

class EmptyFrames
{
public:
	EmptyFrames();
	virtual ~EmptyFrames();
	void runTest();
private:
	void init();
	static void presentFrame(Frame* frame, Pipeline* pipeline);
	Pipeline pipeline;
};

