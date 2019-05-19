class Dx2D {
public:
	// adapted from https://stackoverflow.com/questions/2654480/writing-bmp-image-in-pure-c-c-without-other-libraries
	static void DumpBMPFile(string  filename, DXGI_FORMAT format, void* mem, UINT rowLengthInBytes, int height, int width);
	static void generateBitmapImage(unsigned char* image, int height, int width, int pitch, DXGI_FORMAT format, string imageFileName);
private:
	static const int bytesPerPixel = 4; /// red, green, blue
	static const int fileHeaderSize = 14;
	static const int infoHeaderSize = 40;
	static void getBMPPixelValueFromImage_DXGI_FORMAT_R8G8B8A8_UNORM(char* dest, int x, int y, int pitch, unsigned char* image);
	static unsigned char* createBitmapFileHeader(int height, int width, int pitch, int paddingSize);
	static unsigned char* createBitmapInfoHeader(int height, int width);
};
