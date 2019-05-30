struct FrameDataD2D;

// Utility class for BMP graphics format and Direct2D
// Each thread needs its own instance!
// partly adapted from https://stackoverflow.com/questions/2654480/writing-bmp-image-in-pure-c-c-without-other-libraries
class Dx2D {
public:
	virtual ~Dx2D();
	void init(DXGlobal* dxGlobal, FrameDataD2D* fd, FrameDataGeneral* fd_general_);
	ID2D1RenderTarget* getRenderTarget();
	IDWriteFactory* getWriteFactory();
	const D3D11_TEXTURE2D_DESC* getTextureDesc();
	// copy texture from GPU mem to CPU mem and export it as BMP file
	void copyTextureToCPUAndExport(string filename);
	// write Direct2D image to file in BMP format
	void exportBMP(void* image, int height, int width, int pitch, DXGI_FORMAT format, string imageFileName);
private:
	static const int bytesPerPixel = 4; /// red, green, blue
	static const int fileHeaderSize = 14;
	static const int infoHeaderSize = 40;
	void getBMPPixelValueFromImage_DXGI_FORMAT_R8G8B8A8_UNORM(char* dest, int x, int y, int pitch, unsigned char* image);
	unsigned char* createBitmapFileHeader(int height, int width, int pitch, int paddingSize);
	unsigned char* createBitmapInfoHeader(int height, int width);
	unsigned char fileHeader[fileHeaderSize] = {
		0,0, /// signature
		0,0,0,0, /// image file size in bytes
		0,0,0,0, /// reserved
		0,0,0,0, /// start of pixel array
	};
	unsigned char infoHeader[infoHeaderSize] = {
		0,0,0,0, /// header size
		0,0,0,0, /// image width
		0,0,0,0, /// image height
		0,0, /// number of color planes
		0,0, /// bits per pixel
		0,0,0,0, /// compression
		0,0,0,0, /// image size
		0,0,0,0, /// horizontal resolution
		0,0,0,0, /// vertical resolution
		0,0,0,0, /// colors in color table
		0,0,0,0, /// important color count
	};
	DXGlobal *dxGlobal;
	FrameDataD2D* fd;
	FrameDataGeneral* fd_general;
};

// per farme resources for this effect
struct FrameDataD2D {
private:
	ComPtr<ID2D1Factory3> d2dFactory;
	ComPtr<ID2D1Device2> d2dDevice;
	ComPtr<ID2D1DeviceContext2> d2dDeviceContext;
	ComPtr<IDWriteFactory> dWriteFactory;
	ID3D11Texture2D* texture = nullptr;  // 2d texture used for drawing to with D2D
	IDXGISurface* dxgiSurface = nullptr;
	ID2D1RenderTarget* d2RenderTarget;

	ID3D11Texture2D* textureCPU = nullptr;  // 2d texture used for reading bitmap data from GPU to CPU
	IDWriteFactory* pDWriteFactory_;
	D3D11_TEXTURE2D_DESC desc{};
	friend class Dx2D;
	friend class DXGlobal;
};