#include "stdafx.h"
#include "simple2dframe.h"



Simple2dFrame::Simple2dFrame()
{
}


Simple2dFrame::~Simple2dFrame()
{
	// wait until all frames have finished GPU usage
	dxGlobal.destroy(&pipeline);
}

// run tests with NUM_SLOTS sized frame buffer
void Simple2dFrame::init(HWND hwnd) {
	auto& pc = pipeline.getPipelineConfig();
	pc.setWorldSize(2048.0f, 382.0f, 2048.0f);
	pc.setFrameBufferSize(FRAME_BUFFER_SIZE);
	pc.backbufferWidth = 1024;
	pc.backbufferHeight = 768;
	pipeline.init();
	Log("pipeline initialized" << endl);
	dxGlobal.init();
	if (hwnd != 0) {
		dxGlobal.initSwapChain(&pipeline, hwnd);
	}
	// init framedata
	//afd->setData(&afd[0]);
	for (int i = 0; i < FRAME_BUFFER_SIZE; i++) {
		pipeline.afManager.setAppDataForSlot(&afd[i], i);
		AppFrameData* fd = (AppFrameData *) pipeline.afManager.getAppDataForSlot(i);
		Dx2D* d2d = &fd->d2d;
		FrameDataD2D *fd2d = &fd->d2d_fd;
		FrameDataGeneral *fd_gen = &fd->fd_general;
		dxGlobal.initFrameBufferResources(fd_gen, fd2d, i, &pipeline);
		d2d->init(&dxGlobal, fd2d, fd_gen, &pipeline);
	}
}

void Simple2dFrame::presentFrame(Frame* frame, Pipeline* pipeline) {
	//cout << "present frame slot " << frame->slot << " frame " << frame->absFrameNumber << endl;
	AppFrameData* af_renderTexture = (AppFrameData*)pipeline->afManager.getAppDataForSlot(frame->slot);
	// we need to get the app data for the current back buffer:
	int slot = dxGlobal.swapChain->GetCurrentBackBufferIndex();
	AppFrameData* af_swapChain = (AppFrameData*)pipeline->afManager.getAppDataForSlot(slot);
	if (isAutomatedTestMode) {
		if (frame->absFrameNumber >= (FRAME_COUNT - 1)) {
			//cout << "pipeline should shutdown" << endl;
			pipeline->shutdown();
		}
	}
	// we have to copy the render texture of this slot to the current back buffer render target:
	dxGlobal.copyRenderTexture2Window(&af_renderTexture->fd_general, &af_swapChain->fd_general);

	//// wait for last frame with this index to be finished:
	//dxGlobal.waitGPU(&af_swapChain->fd_general, dxGlobal.commandQueue);
	//// d3d12 present:
	//ID3D12GraphicsCommandList* commandList = af_swapChain->fd_general.commandList.Get();
	//ThrowIfFailed(af_swapChain->fd_general.commandAllocator->Reset());
	//ThrowIfFailed(commandList->Reset(af_swapChain->fd_general.commandAllocator.Get(), af_swapChain->fd_general.pipelineState.Get()));
	//auto resourceStateHelper = dxGlobal.resourceStateHelper;
	//resourceStateHelper->toState(af_swapChain->fd_general.renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, commandList);
	//CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(af_swapChain->fd_general.rtvHeap->GetCPUDescriptorHandleForHeapStart(), 0, af_swapChain->fd_general.rtvDescriptorSize);
	//commandList->ClearRenderTargetView(rtvHandle, dxGlobal.clearColor, 0, nullptr);
	//commandList->ClearDepthStencilView(af_swapChain->fd_general.dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	//resourceStateHelper->toState(af_swapChain->fd_general.renderTarget.Get(), D3D12_RESOURCE_STATE_COMMON, commandList);
	//ThrowIfFailed(commandList->Close());
	//ID3D12CommandList* ppCommandLists[] = { commandList };

	//dxGlobal.commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	ThrowIfFailedWithDevice(dxGlobal.swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING), dxGlobal.device.Get());


	// copy frame to HD
	if (isAutomatedTestMode || frame->absFrameNumber % 10000 == 0) {
		af_swapChain->d2d.copyTextureToCPUAndExport("pic" + to_string(frame->absFrameNumber) + ".bmp");
	}
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

	dxGlobal.clearRenderTexture(&afd->fd_general);
	float col[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	//afd->fd_general.deviceContext11->ClearRenderTargetView(d2d->getRenderTargetView(), col); // we should clear RT from dx12 part
	//cout << "  start draw() for frame: " << frame->absFrameNumber << " slot " << frame->slot << endl;

	// create brush
	ID2D1SolidColorBrush* whiteBrush = nullptr;
	ID2D1SolidColorBrush* redBrush = nullptr;
	D2D1::ColorF red(1, 0, 0, 1);  // fully opaque red
	D2D1::ColorF wh(1, 1, 1, 1);  // fully opaque white
	D2D1::ColorF black(0, 0, 0, 1);  // fully opaque black
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
	//d2RenderTarget->Clear(black);
	//d2RenderTarget->Clear(NULL);
	auto desc = d2d->getTextureDesc();
	d2RenderTarget->DrawRectangle(D2D1::RectF(50.0f, 50.0f, desc->Width - 50.0f, desc->Height - 50.0f), redBrush);
	// fillrectangle produces memeory leaks and leads to device removed error
	//d2RenderTarget->FillRectangle(D2D1::RectF(5.0f, 5.0f, desc->Width - 150.0f, desc->Height - 150.0f), redBrush);
	d2RenderTarget->DrawText(
		sc_helloWorld,
		ARRAYSIZE(sc_helloWorld) - 1,
		pTextFormat_,
		D2D1::RectF(0.0f, 0.0f, (float)desc->Width, (float)desc->Height),
		whiteBrush
	);

	ThrowIfFailed(d2RenderTarget->EndDraw());
	pTextFormat_->Release();
	whiteBrush->Release();
	redBrush->Release();
	//d2d->copyTextureToCPUAndExport("pic" + to_string(frame->absFrameNumber) + ".bmp");
	//cout << "  END draw() for frame: " << frame->absFrameNumber << " slot" << frame->slot << endl;
	d2d->drawStatisticsOverlay(frame, pipeline);
}

void Simple2dFrame::runTest() {
	std::cout << "ShadedPath12 Performance Test: Simple2dFrame\n";
	isAutomatedTestMode = true;
	init();
	// start timer
	auto t0 = chrono::high_resolution_clock::now();
	pipeline.setFinishedFrameConsumer(bind(&Simple2dFrame::presentFrame, this, placeholders::_1, placeholders::_2));
	pipeline.setApplicationFrameData(&afd);
	pipeline.setCallbackDraw(bind(&Simple2dFrame::draw, this, placeholders::_1, placeholders::_2, placeholders::_3));
	pipeline.startRenderThreads();
	pipeline.waitUntilShutdown();
	// stop timer
	auto t1 = chrono::high_resolution_clock::now();
	//cout << "Empty Frame throughput ( " << FRAME_COUNT << " frames): " << chrono::duration_cast<chrono::milliseconds>(t1 - t0).count() << " ms\n";
	//cout << "  Skipped out-of-order frames: " << skipped << endl;

	cout << pipeline.getStatistics();
}

void Simple2dFrame::start() {
	Log("Simple2dFrame UI mode started\n");
	pipeline.setFinishedFrameConsumer(bind(&Simple2dFrame::presentFrame, this, placeholders::_1, placeholders::_2));
	pipeline.setApplicationFrameData(&afd);
	pipeline.setCallbackDraw(bind(&Simple2dFrame::draw, this, placeholders::_1, placeholders::_2, placeholders::_3));
	pipeline.startRenderThreads();
}

void Simple2dFrame::stop() {
	pipeline.shutdown();
	pipeline.waitUntilShutdown();
	Log("Simple2dFrame and pipeline stopped\n");
	Log(s2w(pipeline.getStatistics()));
}
