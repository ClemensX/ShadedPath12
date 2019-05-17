#include "stdafx.h"
#include "simple2dframe.h"



Simple2dFrame::Simple2dFrame()
{
}


Simple2dFrame::~Simple2dFrame()
{
	if (texture != nullptr) {
		texture->Release();
	}
	if (dxgiSurface != nullptr) {
		dxgiSurface->Release();
	}
	if (d2RenderTarget != nullptr) {
		d2RenderTarget->Release();
	}
	//if (d2RenderTarget != nullptr) {
	//	d2RenderTarget->Release();
	//}
}

// run tests with NUM_SLOTS sized frame buffer
void Simple2dFrame::init() {
	auto& pc = pipeline.getPipelineConfig();
	pc.setWorldSize(2048.0f, 382.0f, 2048.0f);
	pc.setFrameBufferSize(FRAME_BUFFER_SIZE);
	pc.backbufferWidth = 1024;
	pc.backbufferHeight = 768;
	pipeline.init();
	Log("pipeline initialized via" << endl);
	dxGlobal.init();
	//dxGlobal.initSwapChain(&pipeline);
	// create d2d texture:
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = 256;
	desc.Height = 256;
	desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	//desc.Usage = D3D11_USAGE_STAGING;//D3D11_USAGE_DYNAMIC;  // CPU and GPU read/write
	desc.Usage = D3D11_USAGE_DEFAULT;
	//desc.BindFlags = 0; // D3D11_BIND_RENDER_TARGET;// D3D11_BIND_SHADER_RESOURCE  -- no bind flags for staging texture
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE| D3D11_CPU_ACCESS_READ;
	desc.MiscFlags = 0;

	ThrowIfFailed(dxGlobal.device11->CreateTexture2D(&desc, NULL, &texture));

	ThrowIfFailed(texture->QueryInterface(&dxgiSurface));
	D2D1_RENDER_TARGET_PROPERTIES props =
		D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
	ThrowIfFailed(dxGlobal.d2dFactory->CreateDxgiSurfaceRenderTarget(dxgiSurface, &props, &d2RenderTarget));
	// create brush
	ID2D1SolidColorBrush* whiteBrush = nullptr;
	ThrowIfFailed(d2RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White),&whiteBrush));
	// draw to texture:
	d2RenderTarget->BeginDraw();
	d2RenderTarget->DrawRectangle(D2D1::RectF(50.0f, 50.0f, desc.Width - 50.0f, desc.Height - 50.0f), whiteBrush);
	ThrowIfFailed(d2RenderTarget->EndDraw());
	whiteBrush->Release();

	// GPU texture to read bitmap data:
	//D3D11_TEXTURE2D_DESC desc{};
	desc.Width = 256;
	desc.Height = 256;
	desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_STAGING;//D3D11_USAGE_DYNAMIC;  // CPU and GPU read/write
	//desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = 0; // D3D11_BIND_RENDER_TARGET;// D3D11_BIND_SHADER_RESOURCE  -- no bind flags for staging texture
	//desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	//desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
	desc.MiscFlags = 0;

	ThrowIfFailed(dxGlobal.device11->CreateTexture2D(&desc, NULL, &textureCPU));
	dxGlobal.deviceContext11->CopyResource(textureCPU, texture);
	D3D11_MAPPED_SUBRESOURCE mapInfo;
	mapInfo.RowPitch;
	ThrowIfFailed(dxGlobal.deviceContext11->Map(
		textureCPU,
		0,
		D3D11_MAP_READ,
		0,
		&mapInfo
	));
	Util::DumpBMPFile("pic1.bmp", DXGI_FORMAT_R8G8B8A8_UNORM, mapInfo.pData, mapInfo.RowPitch, mapInfo.DepthPitch);
	dxGlobal.deviceContext11->Unmap(textureCPU, 0);
	textureCPU->Release();
}

// static void methods are used in threaded code
static mutex monitorMutex;
static long long skipped = 0; // count skipped frames
static long long last_processed = -1; // last processed frame number

// after a frame has been processed this is called to consume it
// (present or store usually)
void Simple2dFrame::presentFrame(Frame* frame, Pipeline* pipeline) {
	unique_lock<mutex> lock(monitorMutex);
	//cout << "present frame slot " << frame->slot << " frame " << frame->absFrameNumber << endl;
	if (frame->absFrameNumber >= (FRAMES_COUNT - 1)) {
		//cout << "pipeline should shutdown" << endl;
		pipeline->shutdown();
	}
	if (frame->absFrameNumber < last_processed) {
		// received an out-of-order frame: discard
		skipped++;
		return;
	}
	// normal in-order frame: process
	cout << "present frame slot " << frame->slot << " frame " << frame->absFrameNumber << endl;
	// TODO
	last_processed = frame->absFrameNumber;
	pipeline->updateStatistics(frame);
}

void Simple2dFrame::runTest() {
	std::cout << "ShadedPath12 Performance Test: Simple2dFrame\n";
	init();
	// start timer
	auto t0 = chrono::high_resolution_clock::now();
	pipeline.setFinishedFrameConsumer(presentFrame);
	pipeline.startRenderThreads();
	pipeline.waitUntilShutdown();
	// stop timer
	auto t1 = chrono::high_resolution_clock::now();
	cout << "Empty Frame throughput ( " << FRAMES_COUNT << " frames): " << chrono::duration_cast<chrono::milliseconds>(t1 - t0).count() << " ms\n";
	cout << "  Skipped out-of-order frames: " << skipped << endl;

	cout << pipeline.getStatistics();
}
