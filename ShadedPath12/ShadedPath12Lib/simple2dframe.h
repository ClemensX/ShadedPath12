#pragma once

// define all frame data
// it is up to the app to organize this, we recommend to define a struct
// with all frame data entries for all effects and make an array with [FRAME_BUFFER_SIZE]
struct AppFrameData : AppFrameDataBase
{
	FrameDataGeneral fd_general;
	FrameDataD2D d2d_fd;
	Dx2D d2d;

	// Inherited via AppFrameDataBase
	virtual FrameDataGeneral* getFrameDataGeneral() override {
		return &fd_general;
	};
};


class Simple2dFrame
{
public:
	Simple2dFrame();
	virtual ~Simple2dFrame();
	// automated non-UI Test run for perfomance measurement 
	void runTest();
	// first initialization - has to be called first
	void init(HWND hwnd = 0);
	// start pipeline and rendering threads
	void start();
	// stop and shutdown pipeline
	void stop();
private:
	static const int FRAME_BUFFER_SIZE = 3;
	static const int FRAME_COUNT = 10;
	// after a frame has been processed this is called to consume it
	// (present or store usually)
	// this call is synced by pipeline, so only 1 thread at any time running this method
	void presentFrame(Frame* frame, Pipeline* pipeline);
	// draw frame, will be called with from multiple threads in parallel
	// usually the app just calls the draw methods of all used effects
	void draw(Frame* frame, Pipeline* pipeline, void* afd);
	Pipeline pipeline;
	DXGlobal dxGlobal;
	AppFrameData afd[FRAME_BUFFER_SIZE];
	boolean isAutomatedTestMode = false;
	//IDWriteTextFormat* pTextFormat_;
};

