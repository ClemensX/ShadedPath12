#include "stdafx.h"

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