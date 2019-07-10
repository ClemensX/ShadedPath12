#include "stdafx.h"
#include "BillboardApp.h"



BillboardApp::BillboardApp()
{
}


BillboardApp::~BillboardApp()
{
	// wait until all frames have finished GPU usage
	dxGlobal.destroy(&pipeline);
}

// run tests with NUM_SLOTS sized frame buffer
void BillboardApp::init(HWND hwnd) {
	auto& pc = pipeline.getPipelineConfig();
	pc.setWorldSize(2048.0f, 382.0f, 2048.0f);
	pc.setFrameBufferSize(FRAME_BUFFER_SIZE);
	pc.backbufferWidth = 1024;
	pc.backbufferHeight = 768;
	pipeline.init();
	Log("pipeline initialized" << endl);
	dxGlobal.init();
	//billboard.init(&dxGlobal);
	if (hwnd != 0) {
		dxGlobal.initSwapChain(&pipeline, hwnd);
	}
	// init framedata
	//afd->setData(&afd[0]);
	for (int i = 0; i < FRAME_BUFFER_SIZE; i++) {
		pipeline.afManager.setAppDataForSlot(&afd[i], i);
		BillboardAppFrameData* fd = (BillboardAppFrameData*) pipeline.afManager.getAppDataForSlot(i);
		Dx2D* d2d = &fd->d2d;
		FrameDataD2D *fd2d = &fd->d2d_fd;
		FrameDataGeneral *fd_gen = &fd->fd_general;
		FrameDataBillboard* fdb = &fd->billboard_fd;
		dxGlobal.initFrameBufferResources(fd_gen, fd2d, i, &pipeline);
		d2d->init(&dxGlobal, fd2d, fd_gen, &pipeline);
		billboard.init(&dxGlobal, fdb, fd_gen);
	}
	// test texture packs:
	util.initPakFiles();
	textureStore.init(&dxGlobal, &util);
	textureStore.loadTexture(L"dirt6_markings.dds", "markings");
	// create effect application data:
	//auto bdata = billboard.getInactiveAppDataSet();
	BillboardElement be1{ {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, -1.0f, 0.0f}, {1.0f, 2.0f} }; // pos, normal, size
	// add to inactive data set:
	billboard.add("markings", be1);
	//bdata->billboards.
}

void BillboardApp::presentFrame(Frame* frame, Pipeline* pipeline) {
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
		dxGlobal.present2Window(pipeline, frame);
	}
}

void BillboardApp::draw(Frame* frame, Pipeline* pipeline, void *data)
{
	BillboardAppFrameData* afd = (BillboardAppFrameData*)frame->frameData;
	FrameDataGeneral* fdg = &afd->fd_general;
	FrameDataD2D *fd = &afd->d2d_fd;
	Dx2D *d2d = &afd->d2d;
	FrameDataBillboard* fdb = &afd->billboard_fd;

	dxGlobal.clearRenderTexture(fdg);
	//cout << "  start draw() for frame: " << frame->absFrameNumber << " slot " << frame->slot << endl;
	billboard.draw(fdg, fdb, pipeline);
	dxGlobal.prepare2DRendering(frame, pipeline, fd);

	d2d->drawStatisticsOverlay(frame, pipeline);
	dxGlobal.end2DRendering(frame, pipeline, fd);
}

void BillboardApp::runTest() {
	std::cout << "ShadedPath12 Test: Billboard Test Application\n";
	isAutomatedTestMode = true;
	init();
	// start timer
	auto t0 = chrono::high_resolution_clock::now();
	pipeline.setFinishedFrameConsumer(bind(&BillboardApp::presentFrame, this, placeholders::_1, placeholders::_2));
	pipeline.setApplicationFrameData(&afd);
	pipeline.setCallbackDraw(bind(&BillboardApp::draw, this, placeholders::_1, placeholders::_2, placeholders::_3));
	pipeline.startRenderThreads();
	pipeline.waitUntilShutdown();
	// stop timer
	auto t1 = chrono::high_resolution_clock::now();
	//cout << "Empty Frame throughput ( " << FRAME_COUNT << " frames): " << chrono::duration_cast<chrono::milliseconds>(t1 - t0).count() << " ms\n";
	//cout << "  Skipped out-of-order frames: " << skipped << endl;

	cout << pipeline.getStatistics();
}

void BillboardApp::start() {
	Log("Simple2dFrame UI mode started\n");
	pipeline.setFinishedFrameConsumer(bind(&BillboardApp::presentFrame, this, placeholders::_1, placeholders::_2));
	pipeline.setApplicationFrameData(&afd);
	pipeline.setCallbackDraw(bind(&BillboardApp::draw, this, placeholders::_1, placeholders::_2, placeholders::_3));
	pipeline.startRenderThreads();
}

void BillboardApp::stop() {
	pipeline.shutdown();
	pipeline.waitUntilShutdown();
	Log("Simple2dFrame and pipeline stopped\n");
	LogF("Simple2dFrame and pipeline stopped\n");
	Log(s2w(pipeline.getStatistics()));
	LogF(s2w(pipeline.getStatistics()));
}
