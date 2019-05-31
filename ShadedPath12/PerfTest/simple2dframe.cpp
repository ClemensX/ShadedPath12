#include "stdafx.h"
#include "simple2dframe.h"



Simple2dFrame::Simple2dFrame()
{
}


Simple2dFrame::~Simple2dFrame()
{
}

// run tests with NUM_SLOTS sized frame buffer
void Simple2dFrame::init() {
	auto& pc = pipeline.getPipelineConfig();
	pc.setWorldSize(2048.0f, 382.0f, 2048.0f);
	pc.setFrameBufferSize(FRAME_BUFFER_SIZE);
	pc.backbufferWidth = 1024;
	pc.backbufferHeight = 768;
	pipeline.init();
	Log("pipeline initialized" << endl);
	dxGlobal.init();
	// init framedata
	//afd->setData(&afd[0]);
	for (int i = 0; i < FRAME_BUFFER_SIZE; i++) {
		pipeline.afManager.setAppDataForSlot(&afd[i], i);
		AppFrameData* fd = (AppFrameData *) pipeline.afManager.getAppDataForSlot(i);
		Dx2D* d2d = &fd->d2d;
		FrameDataD2D *fd2d = &fd->d2d_fd;
		FrameDataGeneral *fd_gen = &fd->fd_general;
		dxGlobal.initFrameBufferResources(fd_gen, fd2d);
		d2d->init(&dxGlobal, fd2d, fd_gen);
	}
}

// static void methods are used in threaded code
static long long skipped = 0; // count skipped frames
static long long last_processed = -1; // last processed frame number

void Simple2dFrame::presentFrame(Frame* frame, Pipeline* pipeline) {
	//cout << "present frame slot " << frame->slot << " frame " << frame->absFrameNumber << endl;
	if (frame->absFrameNumber >= (FRAME_COUNT - 1)) {
		//cout << "pipeline should shutdown" << endl;
		pipeline->shutdown();
	}
	if (frame->absFrameNumber < last_processed) {
		// received an out-of-order frame: discard
		skipped++;
		return;
	}
	// normal in-order frame: process
	//cout << "present frame slot " << frame->slot << " frame " << frame->absFrameNumber << endl;
	// TODO
	//d2d->copyTextureToCPUAndExport("pic" + to_string(frame->absFrameNumber) + ".bmp");

	last_processed = frame->absFrameNumber;
	pipeline->updateStatistics(frame);
}

void Simple2dFrame::draw(Frame* frame, Pipeline* pipeline, void *data)
{
	if (data == nullptr) {
		cout << "  null frame data in draw() for frame: " << frame->absFrameNumber << endl;
		return;
	}
	AppFrameData* afd = (AppFrameData*)frame->frameData;
	FrameDataD2D *fd = &afd->d2d_fd;
	Dx2D *d2d = &afd->d2d;
	//cout << "  start draw() for frame: " << frame->absFrameNumber << " slot " << frame->slot << endl;

	// create brush
	ID2D1SolidColorBrush* whiteBrush = nullptr;
	ID2D1SolidColorBrush* redBrush = nullptr;
	D2D1::ColorF red(1, 0, 0, 1);  // fully opaque red
	D2D1::ColorF wh(1, 1, 1, 1);  // fully opaque white
	auto d2RenderTarget = d2d->getRenderTarget();
	ThrowIfFailed(d2RenderTarget->CreateSolidColorBrush(red, &redBrush));
	ThrowIfFailed(d2RenderTarget->CreateSolidColorBrush(wh, &whiteBrush));

	static const WCHAR msc_fontName[] = L"Verdana";
	static const FLOAT msc_fontSize = 50;
	auto writeFactory = d2d->getWriteFactory();
	IDWriteTextFormat* pTextFormat_;
	ThrowIfFailed(writeFactory->CreateTextFormat(
		msc_fontName,
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		msc_fontSize,
		L"", //locale
		&pTextFormat_
	));
	pTextFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
	pTextFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	static const WCHAR sc_helloWorld[] = L"Hello, World!";

	// draw to texture:
	d2RenderTarget->BeginDraw();
	auto desc = d2d->getTextureDesc();
	d2RenderTarget->DrawRectangle(D2D1::RectF(50.0f, 50.0f, desc->Width - 50.0f, desc->Height - 50.0f), redBrush);
	d2RenderTarget->FillRectangle(D2D1::RectF(5.0f, 5.0f, desc->Width - 150.0f, desc->Height - 150.0f), redBrush);
	d2RenderTarget->DrawText(
		sc_helloWorld,
		ARRAYSIZE(sc_helloWorld) - 1,
		pTextFormat_,
		D2D1::RectF(0.0f, 0.0f, (float)desc->Width, (float)desc->Height),
		whiteBrush
	);

	ThrowIfFailed(d2RenderTarget->EndDraw());
	whiteBrush->Release();
	redBrush->Release();
	d2d->copyTextureToCPUAndExport("pic" + to_string(frame->absFrameNumber) + ".bmp");
	//cout << "  END draw() for frame: " << frame->absFrameNumber << " slot" << frame->slot << endl;
}

void Simple2dFrame::runTest() {
	std::cout << "ShadedPath12 Performance Test: Simple2dFrame\n";
	init();
	// start timer
	auto t0 = chrono::high_resolution_clock::now();
	pipeline.setFinishedFrameConsumer(presentFrame);
	pipeline.setApplicationFrameData(&afd);
	pipeline.setCallbackDraw(draw);
	pipeline.startRenderThreads();
	pipeline.waitUntilShutdown();
	// stop timer
	auto t1 = chrono::high_resolution_clock::now();
	cout << "Empty Frame throughput ( " << FRAME_COUNT << " frames): " << chrono::duration_cast<chrono::milliseconds>(t1 - t0).count() << " ms\n";
	cout << "  Skipped out-of-order frames: " << skipped << endl;

	cout << pipeline.getStatistics();
}
