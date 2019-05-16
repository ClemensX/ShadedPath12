#pragma once

#define FRAME_BUFFER_SIZE 3
#define FRAMES_COUNT 1000

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
	ID3D11Texture2D* texture = nullptr;  // 2d texture used for drawing to with D2D
};

