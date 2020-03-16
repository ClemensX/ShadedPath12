#include "stdafx.h"
#include "SkelApp.h"



SkelApp::SkelApp()
{
}


SkelApp::~SkelApp()
{
	//if (vr) delete vr;
	// wait until all frames have finished GPU usage
	dxGlobal.destroy(&pipeline);
}

// run tests with NUM_SLOTS sized frame buffer
void SkelApp::init(HWND hwnd) {
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
		SkelAppFrameData* fd = (SkelAppFrameData*) pipeline.afManager.getAppDataForSlot(i);
		Dx2D* d2d = &fd->d2d;
		FrameDataD2D *fd2d = &fd->d2d_fd;
		FrameDataGeneral *fd_gen = &fd->fd_general;
		FrameDataBillboard* fdb = &fd->billboard_fd;
		FrameDataLine* fdl = &fd->line_fd;
		dxGlobal.initFrameBufferResources(fd_gen, fd2d, i, &pipeline);
		d2d->init(&dxGlobal, fd2d, fd_gen, &pipeline);
		billboard.init(&dxGlobal, fdb, fd_gen, &pipeline);
		lineEffect.init(&dxGlobal, fdl, fd_gen, &pipeline);
	}
	// store effects that will be called during data updates
	updateEffectList.push_back((Effect*)&billboard);
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

	// test texture packs:
	dxGlobal.util.initPakFiles();
	textureStore.init();
	textureStore.loadTexture(L"dirt6_markings.dds", "markings");
	textureStore.loadTexture(L"metal1.dds", "metal");
	//xapp().textureStore.loadTexture(L"worm1.dds", "worm");
	//xapp().textureStore.loadTexture(L"mars_6k_color.dds", "planet");
	//xapp().textureStore.loadTexture(L"met1.dds", "meteor1");
	//xapp().textureStore.loadTexture(L"axistest.dds", "axistest");
	textureStore.loadTexture(L"white.dds", "white");
	// create effect application data:
	//auto bdata = billboard.getInactiveAppDataSet();
	BillboardElement be1{ {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, -1.0f, 0.0f}, {0.5f, 0.5f} }; // pos, normal, size
	BillboardElement be2{ {0.5f, 0.1f, 2.1f}, {0.2f, 0.0f, -1.0f, 0.0f}, {0.5f, 0.5f} }; // pos, normal, size
	//BillboardElement be3{ {-3.5f, 0.0f, 14.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.5f, 0.5f} }; // pos, normal, size
	BillboardElement be3{ {0.0f, 0.0f, 0.1f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.5f, 0.5f} }; // pos, normal, size
	unsigned long billboardUser = 0;
	billboard.updateQueue.getLockedInactiveDataSet(billboardUser);
	billboard.add("markings", be1, billboardUser);
	billboard.add("markings", be2, billboardUser);
	billboard.add("markings", be3, billboardUser);

	// activate changes:
	billboard.activateAppDataSet(billboardUser);
	billboard.updateQueue.releaseLockedInactiveDataSet(billboardUser);

	TextureInfo* GrassTex, * HouseTex, * MetalTex, * WormTex, * PlanetTex, * Meteor1Tex, * AxistestTex;
	MetalTex = textureStore.getTexture("metal");
	HouseTex = textureStore.getTexture("default");
	TextureInfo* WhiteTex = textureStore.getTexture("white");
	//xapp().lights.init();
	object.material.ambient = XMFLOAT4(1, 1, 1, 1);
	if (true) {
		vector<ObjectDef> ods {
			{ string("Joint"), string("Beta_JointsMesh")  },
			{ string("Surface"), string("Beta_SurfaceMesh"), }
		};
		objectStore.loadObjects(L"Walking2.b", ods);
		objectStore.addObject(object, "Joint", XMFLOAT3(10.0f, 10.0f, 10.0f), MetalTex);
		//object.setAction("Armature");

		//object.pathDescBone->pathMode = PathMode::Path_Reverse;
		//object.forceBoundingBox(BoundingBox(XMFLOAT3(3.16211f, 3.16214f, 7.28022f), XMFLOAT3(4.51012f, 4.51011f, 7.6599f)));
		object.material.specExp = 1.0f;       // no spec color
		object.material.specIntensity = 0.0f; // no spec color
		object.drawNormals = true;
		object.drawBoundingBox = true;
	}
	if (use2ndObject) {
		object2.material.ambient = XMFLOAT4(1, 1, 1, 1);
		objectStore.addObject(object2, "Surface", XMFLOAT3(10.0f, 10.0f, 10.0f), MetalTex);
		//object.setAction("Armature");

		//object.pathDescBone->pathMode = PathMode::Path_Reverse;
		//object.forceBoundingBox(BoundingBox(XMFLOAT3(3.16211f, 3.16214f, 7.28022f), XMFLOAT3(4.51012f, 4.51011f, 7.6599f)));
		object2.material.specExp = 1.0f;       // no spec color
		object2.material.specIntensity = 0.0f; // no spec color
		object2.drawNormals = true;
		object2.drawBoundingBox = true;
	}


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

void SkelApp::presentFrame(Frame* frame, Pipeline* pipeline) {
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
		SkelAppFrameData* afd = (SkelAppFrameData*)frame->frameData;
		FrameDataGeneral* fdg = &afd->fd_general;
		dxGlobal.present2Window(pipeline, frame);
		dxGlobal.submitVR(frame, pipeline, fdg);
#if defined (_SVR_)
		//dxGlobal.vr->UpdateHMDMatrixPose();
		//dxGlobal.vr->UpdateHMDMatrixPose();
#endif
	}
}

void SkelApp::draw(Frame* frame, Pipeline* pipeline, void* data)
{
	// handle input first:
	KeyTicks ticks;
	input->getAndClearKeyTicks(ticks);
	//input->applyTicksToCameraPosition(ticks, &c, 0.00001f);
	input->applyTicksToCameraPosition(ticks, &c, 0.011f);
	input->applyMouseEvents(&c, 0.003f); // 0.003f
	// draw effects;
	SkelAppFrameData* afd = (SkelAppFrameData*)frame->frameData;
	FrameDataGeneral* fdg = &afd->fd_general;
	FrameDataD2D* fd = &afd->d2d_fd;
	Dx2D* d2d = &afd->d2d;
	FrameDataBillboard* fdb = &afd->billboard_fd;
	FrameDataLine* fdl = &afd->line_fd;

	dxGlobal.waitAndReset(fdg);
	dxGlobal.startStatisticsDraw(fdg, frame);
	dxGlobal.clearRenderTexture(fdg);
	//cout << "  start draw() for frame: " << frame->absFrameNumber << " slot " << frame->slot << endl;
	c2 = c; // TODO copy camera for now - get from HMD later
	//Log("cam x y z: " << c.pos.x << " " << c.pos.y << " " << c.pos.z << endl);
	dxGlobal.prepareCameras(frame, pipeline, &c, &c2);
	billboard.draw(frame, fdg, fdb, pipeline);
	lineEffect.draw(frame, fdg, fdl, pipeline);
	dxGlobal.prepare2DRendering(frame, pipeline, fd);

	d2d->drawStatisticsOverlay(frame, pipeline);
	dxGlobal.end2DRendering(frame, pipeline, fd);
	dxGlobal.endStatisticsDraw(fdg);
}

void SkelApp::update(Pipeline* pipeline)
{
#if defined(DISABLE_UPDATE_THREADS)
	return;
#endif
	//unique_lock<mutex> lock(billboard.dataSetMutex);
	//return; // TODO here
	auto now = chrono::high_resolution_clock::now();
	auto millis = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count();
	//Log("BillboardApp update since game start [millis] " << millis << endl);

	unsigned long user = 0;
	//billboard.updateQueue.getLockedInactiveDataSet(user);
	//auto& inactive = billboard.getInactiveAppDataSet(user)->billboards;
	LineEffectAppData *actDataSet = lineEffect.getActiveAppDataSet();
	//auto& active = actDataSet->billboards;

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
	lineEffect.updateQueue.getLockedInactiveDataSet(lineUser);
	lineEffect.addOneTime(lines, lineUser);
	// draw skeleton and/ ormesh wirefarme form lines:
	object.drawSkeleton(XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), &path, &lineEffect, plus, lineUser);  // blue skeleton
	if (use2ndObject) {
		object2.drawSkeleton(XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), &path, &lineEffect, plus, lineUser);  // blue skeleton
	}
	// activate changes:
	lineEffect.activateAppDataSet(lineUser);
	lineEffect.updateQueue.releaseLockedInactiveDataSet(lineUser);
	////Log(" inactive set: " << active.size() << endl);
	//assert(active.size() == inactive.size());
	Effect::update(updateEffectList, pipeline, user);
	lineEffect.releaseActiveAppDataSet(actDataSet);
}

void SkelApp::runTest() {
	std::cout << "ShadedPath12 Test: Billboard Test Application\n";
	isAutomatedTestMode = true;
	init();
	// start timer
	auto t0 = chrono::high_resolution_clock::now();
	pipeline.setFinishedFrameConsumer(bind(&SkelApp::presentFrame, this, placeholders::_1, placeholders::_2));
	pipeline.setApplicationFrameData(&afd);
	pipeline.setCallbackDraw(bind(&SkelApp::draw, this, placeholders::_1, placeholders::_2, placeholders::_3));
	pipeline.startRenderThreads();
	pipeline.waitUntilShutdown();
	// stop timer
	auto t1 = chrono::high_resolution_clock::now();
	//cout << "Empty Frame throughput ( " << FRAME_COUNT << " frames): " << chrono::duration_cast<chrono::milliseconds>(t1 - t0).count() << " ms\n";
	//cout << "  Skipped out-of-order frames: " << skipped << endl;

	cout << pipeline.getStatistics();
}

void SkelApp::start() {
	Log("BillboardApp UI mode started\n");
	pipeline.setFinishedFrameConsumer(bind(&SkelApp::presentFrame, this, placeholders::_1, placeholders::_2));
	pipeline.setApplicationFrameData(&afd);
	pipeline.setCallbackDraw(bind(&SkelApp::draw, this, placeholders::_1, placeholders::_2, placeholders::_3));
#if !defined(DISABLE_UPDATE_THREADS)
	pipeline.setCallbackUpdate(bind(&SkelApp::update, this, placeholders::_1));
	pipeline.startUpdateThread();
#endif
	pipeline.startRenderThreads();
}

void SkelApp::stop() {
	pipeline.shutdown();
	pipeline.waitUntilShutdown();
	Log("BillboardApp and pipeline stopped\n");
	LogF("BillboardApp and pipeline stopped\n");
	Log(s2w(pipeline.getStatistics()));
	LogF(s2w(pipeline.getStatistics()));
	LogF("swap chain submit timings:");
	ThemedTimer::getInstance()->logEntries("SwapChain");
	LogF("vr submit timings:");
	ThemedTimer::getInstance()->logEntries("vr");
}
