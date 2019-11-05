#include "stdafx.h"
#include "BillboardApp.h"



BillboardApp::BillboardApp()
{
}


BillboardApp::~BillboardApp()
{
	//if (vr) delete vr;
	// wait until all frames have finished GPU usage
	dxGlobal.destroy(&pipeline);
}

// run tests with NUM_SLOTS sized frame buffer
void BillboardApp::init(HWND hwnd) {
	auto& pc = pipeline.getPipelineConfig();
	pc.setWorldSize(2048.0f, 382.0f, 2048.0f);
	pc.setFrameBufferSize(FRAME_BUFFER_SIZE);
	pc.setVRMode();
	//pc.setSingleThreadMode();
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
	dxGlobal.setTextureStore(&textureStore);
	//vr = new VR2();
	dxGlobal.vr = &vr;
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
		billboard.init(&dxGlobal, fdb, fd_gen, &pipeline);
	}
	// store effects that will be called during data updates
	updateEffectList.push_back((Effect*)&billboard);
	// test texture packs:
	util.initPakFiles();
	textureStore.init(&dxGlobal, &util);
	textureStore.loadTexture(L"dirt6_markings.dds", "markings");
	textureStore.loadTexture(L"vac_00.dds", "vac00");
	textureStore.loadTexture(L"vac_01.dds", "vac01");
	textureStore.loadTexture(L"vac_02.dds", "vac02");
	textureStore.loadTexture(L"vac_03.dds", "vac03");
	textureStore.loadTexture(L"vac_04.dds", "vac04");
	textureStore.loadTexture(L"vac_05.dds", "vac05");
	textureStore.loadTexture(L"vac_06.dds", "vac06");
	textureStore.loadTexture(L"vac_07.dds", "vac07");
	textureStore.loadTexture(L"vac_08.dds", "vac08");
	textureStore.loadTexture(L"vac_09.dds", "vac09");
	textureStore.loadTexture(L"vac_10.dds", "vac10");
	textureStore.loadTexture(L"vac_11.dds", "vac11");
	// create effect application data:
	//auto bdata = billboard.getInactiveAppDataSet();
	BillboardElement be1{ {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, -1.0f, 0.0f}, {0.5f, 0.5f} }; // pos, normal, size
	BillboardElement be2{ {0.5f, 0.1f, 2.1f}, {0.2f, 0.0f, -1.0f, 0.0f}, {0.5f, 0.5f} }; // pos, normal, size
	//BillboardElement be3{ {-3.5f, 0.0f, 14.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.5f, 0.5f} }; // pos, normal, size
	BillboardElement be3{ {0.0f, 0.0f, 0.1f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.5f, 0.5f} }; // pos, normal, size
	// add to inactive data set:
	unsigned long user = 0;
	billboard.updateQueue.getLockedInactiveDataSet(user);
	billboard.add("markings", be1, user);
	billboard.add("markings", be2, user);
	billboard.add("vac11", be3, user);
	if (true) {
		BillboardElement b;
		b.pos = XMFLOAT3(15.0f, 0.0f, 2.0f);
		b.normal = XMFLOAT4(-1.0f, 0.0f, -1.0f, 1.0f);
		//b.size = XMFLOAT2(1.5f, 1.0f);
		b.size = XMFLOAT2(3.629f, 2.4192f);
		///	unsigned long total_billboards = 4000000;
		//unsigned long total_billboards = 1000000;
		unsigned long total_billboards = 500000;
		//unsigned long total_billboards = 5000;
		// unsigned long total_billboards = 12;
		unsigned long billboards_per_texture = total_billboards / 12;

		// create randomly positioned billboards for each vacXX texture we have:
		for (int tex_number = 0; tex_number < 12; tex_number++) {
			//assemble texture name:
			string texName;
			if (tex_number < 10)
				texName = string("vac0").append(to_string(tex_number));
			else
				texName = string("vac").append(to_string(tex_number));
			//Log(elvec.first.c_str() << endl);
			auto* tex = textureStore.getTexture(texName);
			for (unsigned long i = 0; i < billboards_per_texture; i++) {
				XMFLOAT3 rnd = pipeline.getWorld()->getRandomPos();
				b.pos.x = rnd.x;
				b.pos.y = rnd.y;
				b.pos.z = rnd.z;
				billboard.add(texName, b, user);
			}
		}

	}
	//billboard.
	// activate changes:
	billboard.activateAppDataSet(user);
	billboard.updateQueue.releaseLockedInactiveDataSet(user);
	//bdata->billboards.
	input = Input::getInstance();
	c.init();
	c.nearZ = 0.2f;
	c.farZ = 2000.0f;
	c.pos = XMFLOAT4(0.0f, 0.0f, -3.0f, 0.0f);
	c.setSpeed(15.5f); // faster for dev usability // 15.5
	c.fieldOfViewAngleY = 1.289f;
	//world.setWorldSize(2048.0f, 382.0f, 2048.0f);
	c.projectionTransform();
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
		BillboardAppFrameData* afd = (BillboardAppFrameData*)frame->frameData;
		FrameDataGeneral* fdg = &afd->fd_general;
		dxGlobal.present2Window(pipeline, frame);
		dxGlobal.submitVR(frame, pipeline, fdg);
#if defined (_SVR_)
		//dxGlobal.vr->UpdateHMDMatrixPose();
		//dxGlobal.vr->UpdateHMDMatrixPose();
#endif
	}
}

void BillboardApp::draw(Frame* frame, Pipeline* pipeline, void* data)
{
	// handle input first:
	KeyTicks ticks;
	input->getAndClearKeyTicks(ticks);
	//input->applyTicksToCameraPosition(ticks, &c, 0.00001f);
	input->applyTicksToCameraPosition(ticks, &c, 0.011f);
	input->applyMouseEvents(&c, 0.003f); // 0.003f
	// draw effects;
	BillboardAppFrameData* afd = (BillboardAppFrameData*)frame->frameData;
	FrameDataGeneral* fdg = &afd->fd_general;
	FrameDataD2D* fd = &afd->d2d_fd;
	Dx2D* d2d = &afd->d2d;
	FrameDataBillboard* fdb = &afd->billboard_fd;

	dxGlobal.waitAndReset(fdg);
	dxGlobal.startStatisticsDraw(fdg);
	dxGlobal.clearRenderTexture(fdg);
	//cout << "  start draw() for frame: " << frame->absFrameNumber << " slot " << frame->slot << endl;
	c2 = c; // TODO copy camera for now - get from HMD later
	//Log("cam x y z: " << c.pos.x << " " << c.pos.y << " " << c.pos.z << endl);
	dxGlobal.prepareCameras(frame, pipeline, &c, &c2);
	billboard.draw(frame, fdg, fdb, pipeline);
	dxGlobal.prepare2DRendering(frame, pipeline, fd);

	d2d->drawStatisticsOverlay(frame, pipeline);
	dxGlobal.end2DRendering(frame, pipeline, fd);
	dxGlobal.endStatisticsDraw(fdg);
}

void BillboardApp::update(Pipeline* pipeline)
{
	//unique_lock<mutex> lock(billboard.dataSetMutex);
	//return; // TODO here
	auto now = chrono::high_resolution_clock::now();
	auto millis = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count();
	//Log("BillboardApp update since game start [millis] " << millis << endl);

	unsigned long user = 0;
	billboard.updateQueue.getLockedInactiveDataSet(user);
	auto& inactive = billboard.getInactiveAppDataSet(user)->billboards;
	auto& active = billboard.getActiveAppDataSet()->billboards;
	// in each cycle we copy all the data from active and apply our changesdepending on duration that has passed
	inactive.clear(); // remove entries - does not deallocate mem

	for (auto& b : active) {
		auto& tex = b.first;
		auto& vec = b.second;
		for (auto& p : vec) {
			BillboardElement moved_p = p;
			moved_p.pos.x += 0.101f;
			billboard.add(tex, moved_p, user);
			//Log(" x " << moved_p.pos.x << endl);
		}
	}
	//Log(" inactive set: " << active.size() << endl);
	assert(active.size() == inactive.size());
	Effect::update(updateEffectList, pipeline, user);
	//billboard.activateAppDataSet();
	//billboard.updateQueue.releaseLockedInactiveDataSet(user);
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
	Log("BillboardApp UI mode started\n");
	pipeline.setFinishedFrameConsumer(bind(&BillboardApp::presentFrame, this, placeholders::_1, placeholders::_2));
	pipeline.setApplicationFrameData(&afd);
	pipeline.setCallbackDraw(bind(&BillboardApp::draw, this, placeholders::_1, placeholders::_2, placeholders::_3));
	pipeline.setCallbackUpdate(bind(&BillboardApp::update, this, placeholders::_1));
	pipeline.startUpdateThread();
	pipeline.startRenderThreads();
}

void BillboardApp::stop() {
	pipeline.shutdown();
	pipeline.waitUntilShutdown();
	Log("BillboardApp and pipeline stopped\n");
	LogF("BillboardApp and pipeline stopped\n");
	Log(s2w(pipeline.getStatistics()));
	LogF(s2w(pipeline.getStatistics()));
}
