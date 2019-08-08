#include "stdafx.h"

void Dx2D::init(DXGlobal* dxGlobal_, FrameDataD2D* fd_, FrameDataGeneral* fd_general_, Pipeline* pipeline)
{
	this->fd = fd_;
	this->dxGlobal = dxGlobal_;
	this->fd_general = fd_general_;
	auto config = pipeline->getPipelineConfig();
	// wrap resources:
	D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
	ThrowIfFailed(fd_general->device11On12->CreateWrappedResource(
		fd_general->renderTargetRenderTexture.Get(),
		&d3d11Flags,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		IID_PPV_ARGS(&fd_general->wrappedDx12Resource)
	));

	// Query the desktop's dpi settings, which will be used to create
	// D2D's render targets.
	float dpiX;
	float dpiY;
	// fix GetDesktopDpi no longer supported:
	//fd->d2dFactory->GetDesktopDpi(&dpiX, &dpiY);
	UINT dpi = GetDpiForSystem();
	dpiX = (float) dpi;
	dpiY = dpiX;
	D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
		dpiX,
		dpiY
	);
	// Create a render target for D2D to draw directly to this back buffer.
	ComPtr<IDXGISurface> surface;
	ThrowIfFailed(fd_general->wrappedDx12Resource.As(&surface));
	ThrowIfFailed(fd->d2dDeviceContext->CreateBitmapFromDxgiSurface(
		surface.Get(),
		&bitmapProperties,
		&fd->d2dRenderTargetBitmap
	));

	// create d2d texture:
	auto &desc = fd->desc;
	desc.Width = config.backbufferWidth;
	desc.Height = config.backbufferHeight;
	desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	//desc.Usage = D3D11_USAGE_STAGING;//D3D11_USAGE_DYNAMIC;  // CPU and GPU read/write
	desc.Usage = D3D11_USAGE_DEFAULT;
	//desc.BindFlags = 0; // D3D11_BIND_RENDER_TARGET;// D3D11_BIND_SHADER_RESOURCE  -- no bind flags for staging texture
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
	desc.MiscFlags = 0;

	ThrowIfFailed(fd_general->device11->CreateTexture2D(&desc, NULL, &fd->texture));

	ThrowIfFailed(fd_general->device11->CreateRenderTargetView(
		fd->texture,
		NULL, //const D3D11_RENDER_TARGET_VIEW_DESC * pDesc,
		&fd->d2Rtv
	));
	// GPU texture to read bitmap data:
	//D3D11_TEXTURE2D_DESC desc{};
	desc.Width = config.backbufferWidth;
	desc.Height = config.backbufferHeight;
	desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_STAGING;//D3D11_USAGE_DYNAMIC;  // CPU and GPU read/write
	//desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = 0; // D3D11_BIND_RENDER_TARGET;// D3D11_BIND_SHADER_RESOURCE  -- no bind flags for staging texture
	//desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	//desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
	desc.MiscFlags = 0;

	ThrowIfFailed(fd_general->device11->CreateTexture2D(&desc, NULL, &fd->textureCPU));

	ThrowIfFailed(fd->texture->QueryInterface(&fd->dxgiSurface));
/*	D2D1_RENDER_TARGET_PROPERTIES props =
		D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE));
	ThrowIfFailed(fd->d2dFactory->CreateDxgiSurfaceRenderTarget(fd->dxgiSurface, &props, &fd->d2RenderTarget));
*/
	D2D1_RENDER_TARGET_PROPERTIES props =
		D2D1::RenderTargetProperties(
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));
	ThrowIfFailed(fd->d2dFactory->CreateDxgiSurfaceRenderTarget(surface.Get(), &props, &fd->d2RenderTarget));
	// prepare DWrite factory
	ThrowIfFailed(DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown * *>(&fd->pDWriteFactory_)
	));
}

void Dx2D::copyTextureToCPUAndExport(string filename)
{
	fd_general->deviceContext11->CopyResource(fd->textureCPU, fd->texture);
	D3D11_MAPPED_SUBRESOURCE mapInfo;
	mapInfo.RowPitch;
	ThrowIfFailed(fd_general->deviceContext11->Map(
		fd->textureCPU,
		0,
		D3D11_MAP_READ,
		0,
		&mapInfo
	));
	exportBMP(mapInfo.pData, fd->desc.Height, fd->desc.Width, mapInfo.RowPitch, DXGI_FORMAT_R8G8B8A8_UNORM, filename);
	fd_general->deviceContext11->Unmap(fd->textureCPU, 0);
}

ID2D1RenderTarget* Dx2D::getRenderTarget()
{
	//return fd->d2dRenderTargetBitmap.Get();
	return fd->d2RenderTarget;
	//return fd->d2dDeviceContext->GetTarget(fd->d2dRenderTargetBitmap.Get());
}

ID3D11RenderTargetView* Dx2D::getRenderTargetView()
{
	return fd->d2Rtv;
}

IDWriteFactory* Dx2D::getWriteFactory()
{
	return fd->pDWriteFactory_;
}

const D3D11_TEXTURE2D_DESC* Dx2D::getTextureDesc()
{
	return &fd->desc;
}

void Dx2D::exportBMP(void* image, int height, int width, int pitch, DXGI_FORMAT format, string imageFileName) {

	if (format != DXGI_FORMAT_R8G8B8A8_UNORM) {
		Error(L"unknown pixel format");
	}
	unsigned char padding[3] = { 0, 0, 0 };
	int paddingSize = (4 - (/*width*bytesPerPixel*/ pitch) % 4) % 4;

	unsigned char* fileHeader = createBitmapFileHeader(height, width, pitch, paddingSize);
	unsigned char* infoHeader = createBitmapInfoHeader(height, width);

	ofstream out(imageFileName, ios_base::binary | ios_base::out);

	out.write((const char*)fileHeader, fileHeaderSize);
	out.write((const char*)infoHeader, infoHeaderSize);

	char pixel[4];
	int i, j;
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			getBMPPixelValueFromImage_DXGI_FORMAT_R8G8B8A8_UNORM(&pixel[0], j, height - 1 - i, pitch, (unsigned char*)image);
			out.write((const char*)& pixel[0], bytesPerPixel);
		}
		//out.write((const char*)(image + (i * pitch /*width*bytesPerPixel*/)), bytesPerPixel * width);
		out.write((const char*)padding, paddingSize);
	}

	out.close();
}

void Dx2D::getBMPPixelValueFromImage_DXGI_FORMAT_R8G8B8A8_UNORM(char* dest, int x, int y, int pitch, unsigned char* image)
{
	memcpy(dest, (image + ((long long)x * bytesPerPixel) + (((long long)y) * pitch /*width*bytesPerPixel*/)), bytesPerPixel);
	// dest: [0] == blue, [1] == green, [2] = red, [3] = 0 (reserved)
	dest[3] = dest[0]; // save for later
	dest[0] = dest[2]; //blue
	//dest[1] = dest[1]; // green already correct
	dest[2] = dest[3];
	dest[3] = 0;
}

unsigned char* Dx2D::createBitmapFileHeader(int height, int width, int pitch, int paddingSize) {
	int fileSize = fileHeaderSize + infoHeaderSize + (/*bytesPerPixel*width*/pitch + paddingSize) * height;

	fileHeader[0] = (unsigned char)('B');
	fileHeader[1] = (unsigned char)('M');
	fileHeader[2] = (unsigned char)(fileSize);
	fileHeader[3] = (unsigned char)(fileSize >> 8);
	fileHeader[4] = (unsigned char)(fileSize >> 16);
	fileHeader[5] = (unsigned char)(fileSize >> 24);
	fileHeader[10] = (unsigned char)(fileHeaderSize + infoHeaderSize);

	return fileHeader;
}

unsigned char* Dx2D::createBitmapInfoHeader(int height, int width) {

	infoHeader[0] = (unsigned char)(infoHeaderSize);
	infoHeader[4] = (unsigned char)(width);
	infoHeader[5] = (unsigned char)(width >> 8);
	infoHeader[6] = (unsigned char)(width >> 16);
	infoHeader[7] = (unsigned char)(width >> 24);
	infoHeader[8] = (unsigned char)(height);
	infoHeader[9] = (unsigned char)(height >> 8);
	infoHeader[10] = (unsigned char)(height >> 16);
	infoHeader[11] = (unsigned char)(height >> 24);
	infoHeader[12] = (unsigned char)(1);
	infoHeader[14] = (unsigned char)(bytesPerPixel * 8);

	return infoHeader;
}

Dx2D::~Dx2D()
{
	if (fd == nullptr) {
		// nothing to cleanup 
		return;
	}
	if (fd->texture != nullptr) {
		fd->texture->Release();
	}
	if (fd->dxgiSurface != nullptr) {
		fd->dxgiSurface->Release();
	}
	if (fd->d2RenderTarget != nullptr) {
		fd->d2RenderTarget->Release();
	}
	if (fd->d2Rtv != nullptr) {
		fd->d2Rtv->Release();
	}
	//if (d2RenderTarget != nullptr) {
	//	d2RenderTarget->Release();
	//}
	if (fd->textureCPU != nullptr) {
		fd->textureCPU->Release();
	}
}

void Dx2D::drawStatisticsOverlay(Frame* frame, Pipeline* pipeline)
{
	//AppFrameDataBase* afd = frame->frameData;
	FrameDataD2D* fd = this->fd;
	Dx2D* d2d = this;
	//cout << "  start draw() for frame: " << frame->absFrameNumber << " slot " << frame->slot << endl;

	// create brush
	ID2D1SolidColorBrush* whiteBrush = nullptr;
	D2D1::ColorF wh(1, 1, 1, 1);  // fully opaque white
	auto d2RenderTarget = d2d->getRenderTarget();
	ThrowIfFailed(d2RenderTarget->CreateSolidColorBrush(wh, &whiteBrush));

	static const WCHAR msc_fontName[] = L"Verdana";
	static const FLOAT msc_fontSize = 10;
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
	pTextFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
	pTextFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	wstringstream s;
	s << "# " << (frame->absFrameNumber) << " last render [micros] " << pipeline->lastFrameRenderDuration << " tot FPS " << pipeline->totalFPS << endl;
	//return s.str();
	//static const WCHAR sc_fps[] = L"FPS: 30";

	// draw to texture:
	d2RenderTarget->BeginDraw();
	auto desc = d2d->getTextureDesc();
	d2RenderTarget->DrawText(
		s.str().c_str(),
		(UINT32)s.str().length(),
		pTextFormat_,
		D2D1::RectF(0.0f, 0.0f, (float)desc->Width, 10.0f),
		whiteBrush
	);

	ThrowIfFailed(d2RenderTarget->EndDraw());
	whiteBrush->Release();
	pTextFormat_->Release();
}