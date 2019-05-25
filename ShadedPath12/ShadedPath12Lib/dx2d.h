// Utility class for BMP graphics format and Direct2D
// Each thread needs its own instance!
// partly adapted from https://stackoverflow.com/questions/2654480/writing-bmp-image-in-pure-c-c-without-other-libraries
class Dx2D {
public:
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
};
