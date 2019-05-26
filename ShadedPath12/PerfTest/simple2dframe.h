#pragma once

#define FRAME_BUFFER_SIZE 3
#define FRAMES_COUNT 1000

// define all frame data
// it is up to the app to organize this, we recommend to define a struct
// with all frame data entries for all effects and make an array with [FRAME_BUFFER_SIZE]
struct AppFrameData
{
	FrameDataD2D d2d;
};


class Simple2dFrame
{
public:
	Simple2dFrame();
	virtual ~Simple2dFrame();
	void runTest();
private:
	void init();
	static void presentFrame(Frame* frame, Pipeline* pipeline);
	Pipeline pipeline;
	DXGlobal dxGlobal;
	AppFrameData fd[FRAME_BUFFER_SIZE];
	IDWriteTextFormat* pTextFormat_;
};

