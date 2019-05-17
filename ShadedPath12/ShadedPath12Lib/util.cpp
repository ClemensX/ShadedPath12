#include "stdafx.h"

wchar_t* string2wstring(std::string text)
{
	static wchar_t wstring[5000];
	MultiByteToWideChar(CP_ACP, 0, text.c_str(), -1, wstring, 5000);
	return wstring;
}

// allow parsing like this: vector<string> topics = split(commandline, ' ');
// discards empty strings which occur when multiple delimiter chars are ina row
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		if (item.size() > 0) {
			elems.push_back(item);
		}
	}
	return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

//VectorHelper vectorHelper;

// Util class:

float Util::distancePoint2Beam(XMVECTOR beamStart, XMVECTOR beamPoint2, XMVECTOR p)
{
	// if distance of 2nd beam point to p is bigger than for first point the beam
	// is considered to point in wrong direction: return -1
	if (XMVectorGetX(XMVector3Length(p - beamStart)) < XMVectorGetX(XMVector3Length(p - beamPoint2))) {
		return -1.0f;
	}
	// we are pointing roughly in the right direction - use standard XM method to calc distance
	return XMVectorGetX(XMVector3LinePointDistance(beamStart, beamPoint2, p));
}

//void Util::calcBeamFromObject(XMVECTOR * beamStart, XMVECTOR * beampoint2, WorldObject *o, XMVECTOR beamStartModelPose, XMVECTOR beamPoint2ModelPose)
//{
//	assert(o->useQuaternionRotation);
//	XMVECTOR t = XMLoadFloat3(&o->pos());
//	XMVECTOR rot = XMLoadFloat4(&o->quaternion);
//	*beamStart = XMVector3Rotate(beamStartModelPose, rot) + t;
//	*beampoint2 = XMVector3Rotate(beamPoint2ModelPose, rot) + t;
//}

XMVECTOR Util::movePointToDistance(XMVECTOR start, XMVECTOR controlPoint, float dist)
{
	XMVECTOR diff = controlPoint - start;
	float scale = dist / XMVectorGetX(XMVector2LengthEst(diff));
	XMVECTOR distPoint = start + XMVectorScale(diff, scale);
	//Log("dist " << (XMVectorGetX(XMVector3LinePointDistance(start, controlPoint, distPoint))) << endl);
	assert(XMVectorGetX(XMVector3LinePointDistance(start, controlPoint, distPoint)) < 0.0001f);
	return distPoint;
}

//bool Util::isTargetHit(WorldObject * source, XMVECTOR beamStartModelPose, XMVECTOR beamPoint2ModelPose, WorldObject * target, float hittingDist)
//{
//	XMVECTOR targetPoint = XMLoadFloat3(&target->pos());
//	XMVECTOR beamStart, beamPoint2;
//	calcBeamFromObject(&beamStart, &beamPoint2, source, beamStartModelPose, beamPoint2ModelPose);
//	// move beamPoint2 very close to start, otherwise calling distancePoint2Beam makes no sense
//	beamPoint2 = movePointToDistance(beamStart, beamPoint2, 0.01f);
//	float d = distancePoint2Beam(beamStart, beamPoint2, targetPoint);
//	if (d < 0) return false; // wrong direction
//	return d <= hittingDist;
//}

void Util::DumpBMPFile(string  filename, DXGI_FORMAT format, void *mem, UINT rowLengthInBytes, UINT totalLengthInBytes) {
	LogF("dumping\n");
	generateBitmapImage((unsigned char*)mem, 256, 256, rowLengthInBytes, format, "dumpit.bmp");
	ios_base::openmode mode;
	mode = ios_base::out;
	std::ofstream out("dump.bmp", mode);  // remember: folders do not work here, unless they are pre-generated
	out << "test hugo\n";
	out.close();
}





void Util::generateBitmapImage(unsigned char* image, int height, int width, int pitch, DXGI_FORMAT format, const char* imageFileName) {

	if (format != DXGI_FORMAT_R8G8B8A8_UNORM) {
		Error(L"unknown pixel format");
	}
	unsigned char padding[3] = { 0, 0, 0 };
	int paddingSize = (4 - (/*width*bytesPerPixel*/ pitch) % 4) % 4;

	unsigned char* fileHeader = createBitmapFileHeader(height, width, pitch, paddingSize);
	unsigned char* infoHeader = createBitmapInfoHeader(height, width);

	ofstream out(imageFileName, ios_base::binary|ios_base::out);

	out.write((const char*)fileHeader, fileHeaderSize);
	out.write((const char*)infoHeader, infoHeaderSize);

	char pixel[4];
	int i, j;
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			getBMPPixelValueFromImage_DXGI_FORMAT_R8G8B8A8_UNORM(&pixel[0], j, i, pitch, image);
			out.write((const char*)& pixel[0], bytesPerPixel);
		}
		//out.write((const char*)(image + (i * pitch /*width*bytesPerPixel*/)), bytesPerPixel * width);
		out.write((const char*)padding, paddingSize);
	}

	out.close();
}

void Util::getBMPPixelValueFromImage_DXGI_FORMAT_R8G8B8A8_UNORM(char* dest, int x, int y, int pitch, unsigned char* image)
{
	memcpy(dest, (image + (x * bytesPerPixel) + (y * pitch /*width*bytesPerPixel*/)), bytesPerPixel);
	// dest: [0] == blue, [1] == green, [2] = red, [3] = 0 (reserved)
	dest[3] = dest[0]; // save for later
	dest[0] = dest[2]; //blue
	//dest[1] = dest[1]; // green already correct
	dest[2] = dest[3];
	dest[3] = 0;
}

unsigned char* Util::createBitmapFileHeader(int height, int width, int pitch, int paddingSize) {
	int fileSize = fileHeaderSize + infoHeaderSize + (/*bytesPerPixel*width*/pitch + paddingSize) * height;

	static unsigned char fileHeader[] = {
		0,0, /// signature
		0,0,0,0, /// image file size in bytes
		0,0,0,0, /// reserved
		0,0,0,0, /// start of pixel array
	};

	fileHeader[0] = (unsigned char)('B');
	fileHeader[1] = (unsigned char)('M');
	fileHeader[2] = (unsigned char)(fileSize);
	fileHeader[3] = (unsigned char)(fileSize >> 8);
	fileHeader[4] = (unsigned char)(fileSize >> 16);
	fileHeader[5] = (unsigned char)(fileSize >> 24);
	fileHeader[10] = (unsigned char)(fileHeaderSize + infoHeaderSize);

	return fileHeader;
}

unsigned char* Util::createBitmapInfoHeader(int height, int width) {
	static unsigned char infoHeader[] = {
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