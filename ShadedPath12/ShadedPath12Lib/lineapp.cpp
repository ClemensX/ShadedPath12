#include "stdafx.h"
#include "LineApp.h"



LineApp::LineApp()
{
}


LineApp::~LineApp()
{
	//if (vr) delete vr;
	// wait until all frames have finished GPU usage
	dxGlobal.destroy(&pipeline);
}

// run tests with NUM_SLOTS sized frame buffer
void LineApp::init(HWND hwnd) {
	auto& pc = pipeline.getPipelineConfig();
	pc.setWorldSize(2048.0f, 382.0f, 2048.0f);
	pc.setFrameBufferSize(FRAME_BUFFER_SIZE);
	pc.setVRMode();
	pc.setLineEffectUtilEnabled(true);
#if defined(SINGLE_THREAD_MODE)
	pc.setSingleThreadMode();
#endif
	pc.setSingleThreadMode();
	pc.setMaxUpdatesPerSecond(30); // limit update thread to 30 calls / second
#if defined (_SVR_)
	pc.setHMDMode();
#endif
	// increasing back buffer width/height has huge effect on overall picture quality and sharpness
	// small:
	//pc.backbufferWidth = 1024;
	//pc.backbufferHeight = 768;
	// 4k:
	pc.backbufferWidth = 3840;
	pc.backbufferHeight = 2160;
	// Full HD:
	//pc.backbufferWidth = 3840/2;
	//pc.backbufferHeight = 2160/2;
	// 3840 2160
	pipeline.init();
	Log("pipeline initialized" << endl);
	dxGlobal.init();
	//dxGlobal.setTextureStore(&textureStore);
	//vr = new VR2();
	dxGlobal.vr = &vr;
	if (hwnd != 0) {
		dxGlobal.initSwapChain(&pipeline, hwnd);
	}
	// init framedata
	//afd->setData(&afd[0]);
	for (int i = 0; i < FRAME_BUFFER_SIZE; i++) {
		pipeline.afManager.setAppDataForSlot(&afd[i], i);
		LineAppFrameData* fd = (LineAppFrameData*) pipeline.afManager.getAppDataForSlot(i);
		FrameDataGeneral *fd_gen = &fd->fd_general;
		FrameDataLine* fdl = &fd->line_fd;
		dxGlobal.initFrameBufferResources(fd_gen, nullptr, i, &pipeline);
		lineEffect.init(&dxGlobal, fdl, fd_gen, &pipeline);
	}
	// store effects that will be called during data updates
	updateEffectList.push_back((Effect*)&lineEffect);

	// add some lines:
	aspectRatio = pipeline.getAspectRatio();
	LineDef myLines[] = {
		// start, end, color
		{ XMFLOAT3(0.0f, 0.25f * aspectRatio, 0.0f), XMFLOAT3(0.25f, -0.25f * aspectRatio, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(0.25f, -0.25f * aspectRatio, 0.0f), XMFLOAT3(-0.25f, -0.25f * aspectRatio, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-0.25f, -0.25f * aspectRatio, 0.0f), XMFLOAT3(0.0f, 0.25f * aspectRatio, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }
	};
	vector<LineDef> lines;
	// add all intializer objects to vector:
	for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l); });

	// add to inactive data set:
	unsigned long lineUser = 0;
	lineEffect.updateQueue.getLockedInactiveDataSet(lineUser);
	lineEffect.add(lines, lineUser);
	// activate changes:
	lineEffect.activateAppDataSet(lineUser);
	lineEffect.updateQueue.releaseLockedInactiveDataSet(lineUser);

	// create effect application data:

	input = Input::getInstance();
	c.init();
	c.nearZ = 0.2f;
	c.farZ = 2000.0f;
	c.pos = XMFLOAT4(0.0f, 0.0f, -3.0f, 0.0f);
	c.setSpeed(15.5f); // faster for dev usability // 15.5
	c.fieldOfViewAngleY = 1.289f;
	//world.setWorldSize(2048.0f, 382.0f, 2048.0f);
	c.projectionTransform();
	lineEffect.reinitializeThreadResources();
}

void LineApp::presentFrame(Frame* frame, Pipeline* pipeline) {
	//cout << "present frame slot " << frame->slot << " frame " << frame->absFrameNumber << endl;
	if (isAutomatedTestMode) {
		if (frame->absFrameNumber >= (FRAME_COUNT - 1)) {
			//cout << "pipeline should shutdown" << endl;
			pipeline->shutdown();
		}
	}

	// copy frame to HD
	if (isAutomatedTestMode /*|| frame->absFrameNumber % 10000 == 0*/) {
		// TODO beware of sync problems:
		//af_swapChain->d2d.copyTextureToCPUAndExport("pic" + to_string(frame->absFrameNumber) + ".bmp");
		dxGlobal.copyTextureToCPUAndExport(frame, pipeline, "pic" + to_string(frame->absFrameNumber) + ".bmp");
	}

	if (dxGlobal.isOutputWindowAvailable()) {
		LineAppFrameData* afd = (LineAppFrameData*)frame->frameData;
		FrameDataGeneral* fdg = &afd->fd_general;
		dxGlobal.present2Window(pipeline, frame);
		dxGlobal.submitVR(frame, pipeline, fdg);
#if defined (_SVR_)
		//dxGlobal.vr->UpdateHMDMatrixPose();
		//dxGlobal.vr->UpdateHMDMatrixPose();
#endif
	}
}

void LineApp::draw(Frame* frame, Pipeline* pipeline, void* data)
{
	// handle input first:
	KeyTicks ticks;
	input->getAndClearKeyTicks(ticks);
	//input->applyTicksToCameraPosition(ticks, &c, 0.00001f);
	input->applyTicksToCameraPosition(ticks, &c, 0.011f);
	input->applyMouseEvents(&c, 0.003f); // 0.003f
	// draw effects;
	LineAppFrameData* afd = (LineAppFrameData*)frame->frameData;
	FrameDataGeneral* fdg = &afd->fd_general;
	FrameDataLine* fdl = &afd->line_fd;

	dxGlobal.waitAndReset(fdg);
	//dxGlobal.startStatisticsDraw(fdg, frame);
	dxGlobal.clearRenderTexture(fdg);
	//cout << "  start draw() for frame: " << frame->absFrameNumber << " slot " << frame->slot << endl;
	c2 = c; // TODO copy camera for now - get from HMD later
	//Log("cam x y z: " << c.pos.x << " " << c.pos.y << " " << c.pos.z << endl);
	dxGlobal.prepareCameras(frame, pipeline, &c, &c2);
	lineEffect.draw(frame, fdg, fdl, pipeline);

}

void LineApp::update(Pipeline* pipeline)
{
#if defined(DISABLE_UPDATE_THREADS)
	return;
#endif

	auto now = chrono::high_resolution_clock::now();
	auto millis = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count();
	//Log("LineApp update since game start [millis] " << millis << endl);

	unsigned long user = 0;
	LineEffectAppData *actDataSet = lineEffect.getActiveAppDataSet();

	static float plus = 0.0f;
	LineDef myLines[] = {
		// start, end, color
		{ XMFLOAT3(0.0f, 0.25f * aspectRatio, 1.0f + plus), XMFLOAT3(0.25f, -0.25f * aspectRatio, 1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(0.25f, -0.25f * aspectRatio, 1.0f), XMFLOAT3(-0.25f, -0.25f * aspectRatio, 1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-0.25f, -0.25f * aspectRatio, 1.0f), XMFLOAT3(0.0f, 0.25f * aspectRatio, 1.0f + plus), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) }
	};
	plus += 0.001f;
	vector<LineDef> lines;
	// add all intializer objects to vector:
	for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l); });

	// add to inactive data set:
	unsigned long lineUser = 0;
	//lineEffect.updateQueue.getLockedInactiveDataSet(lineUser);
	//lineEffect.addOneTime(lines, lineUser);

	_CrtMemState s1,s2,s3;
	_CrtMemCheckpoint(&s1);

	// activate changes:
	static int count = 0;
	//if (count < 5)
	//lineEffect.updateQueue.releaseLockedInactiveDataSet(lineUser);
	////Log(" inactive set: " << active.size() << endl);
	//assert(active.size() == inactive.size());
	//Effect::update(updateEffectList, pipeline, user);
	_CrtMemCheckpoint(&s2);
	if (_CrtMemDifference(&s3, &s1, &s2))
		_CrtMemDumpStatistics(&s3);
	lineEffect.activateAppDataSet(lineUser);

	//if (count < 5)
	lineEffect.releaseActiveAppDataSet(actDataSet);
	count++;
}

void LineApp::runTest() {
	std::cout << "ShadedPath12 Test: Line Test Application\n";
	isAutomatedTestMode = true;
	init();
	// start timer
	auto t0 = chrono::high_resolution_clock::now();
	pipeline.setFinishedFrameConsumer(bind(&LineApp::presentFrame, this, placeholders::_1, placeholders::_2));
	pipeline.setApplicationFrameData(&afd);
	pipeline.setCallbackDraw(bind(&LineApp::draw, this, placeholders::_1, placeholders::_2, placeholders::_3));
	pipeline.startRenderThreads();
	pipeline.waitUntilShutdown();
	// stop timer
	auto t1 = chrono::high_resolution_clock::now();
	//cout << "Empty Frame throughput ( " << FRAME_COUNT << " frames): " << chrono::duration_cast<chrono::milliseconds>(t1 - t0).count() << " ms\n";
	//cout << "  Skipped out-of-order frames: " << skipped << endl;

	cout << pipeline.getStatistics();
}

void LineApp::start() {
	Log("LineApp UI mode started\n");
	pipeline.setFinishedFrameConsumer(bind(&LineApp::presentFrame, this, placeholders::_1, placeholders::_2));
	pipeline.setApplicationFrameData(&afd);
	pipeline.setCallbackDraw(bind(&LineApp::draw, this, placeholders::_1, placeholders::_2, placeholders::_3));
#if !defined(DISABLE_UPDATE_THREADS)
	pipeline.setCallbackUpdate(bind(&LineApp::update, this, placeholders::_1));
	pipeline.startUpdateThread();
#endif
	pipeline.startRenderThreads();
}

void LineApp::stop() {
	pipeline.shutdown();
	pipeline.waitUntilShutdown();
	Log("LineApp and pipeline stopped\n");
	LogF("LineApp and pipeline stopped\n");
	Log(s2w(pipeline.getStatistics()));
	LogF(s2w(pipeline.getStatistics()));
	LogF("swap chain submit timings:");
	ThemedTimer::getInstance()->logEntries("SwapChain");
	LogF("vr submit timings:");
	ThemedTimer::getInstance()->logEntries("vr");
}
