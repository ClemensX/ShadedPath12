#pragma once

class Pipeline;

// define all frame data
// it is up to the app to organize this, we recommend to define a struct
// with all frame data entries for all effects and make an array with [FRAME_BUFFER_SIZE]
struct LineAppFrameData : AppFrameDataBase
{
	FrameDataGeneral fd_general;
	FrameDataLine line_fd;

	// Inherited via AppFrameDataBase
	virtual FrameDataGeneral* getFrameDataGeneral() override {
		return &fd_general;
	};
};


class LineApp
{
public:
	LineApp();
	virtual ~LineApp();
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
	static const int FRAME_COUNT = 4;
	// after a frame has been processed this is called to consume it
	// (present or store usually)
	// this call is synced by pipeline, so only 1 thread at any time running this method
	void presentFrame(Frame* frame, Pipeline* pipeline);
	// draw frame, will be called with from multiple threads in parallel
	// usually the app just calls the draw methods of all used effects
	void draw(Frame* frame, Pipeline* pipeline, void* afd);
	void update(Pipeline* pipeline);
	Pipeline pipeline;
	DXGlobal& dxGlobal = *DXGlobal::getInstance();
	LineAppFrameData afd[FRAME_BUFFER_SIZE];
	boolean isAutomatedTestMode = false;

	// Effects and other framework classes:
	Lights lights;
	LinesEffect lineEffect;
	Camera c,c2; //cameras need to be local to threads, copy these in effects
	Input* input = nullptr;
	//VR2 *vr = nullptr;
	VR vr;
	vector<Effect*> updateEffectList;
	float aspectRatio = 0.0f;
	Path path; // calculate animation
};

