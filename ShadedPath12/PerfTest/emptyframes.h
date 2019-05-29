#pragma once

class EmptyFrames
{
public:
	EmptyFrames();
	virtual ~EmptyFrames();
	void runTest();
private:
	static const int FRAME_COUNT = 1000;
	static const int FRAME_BUFFER_SIZE = 3;
	void init();
	static void presentFrame(Frame* frame, Pipeline* pipeline);
	Pipeline pipeline;
};

