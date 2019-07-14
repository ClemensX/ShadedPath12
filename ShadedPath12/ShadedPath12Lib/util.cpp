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

void Util::initPakFiles()
{
	wstring binFile = findFile(L"texture01.pak", Util::TEXTUREPAK, false);
	if (binFile.size() == 0) {
		Log("pak file texture01.pak not found!" << endl);
		return;
	}
	ifstream bfile(binFile, ios::in | ios::binary);
#if defined(_DEBUG)
	Log("pak file opened: " << binFile << "\n");
#endif

	// basic assumptions about data types:
	assert(sizeof(long long) == 8);
	assert(sizeof(int) == 4);

	long long magic;
	bfile.read((char*)& magic, 8);
	magic = _byteswap_uint64(magic);
	if (magic != 0x5350313250414B30L) {
		// magic "SP12PAK0" not found
		Log("pak file invalid: " << binFile << endl);
		return;
	}
	long long numEntries;
	bfile.read((char*)& numEntries, 8);
	if (numEntries > 30000) {
		Log("pak file invalid: contained number of textures: " << numEntries << endl);
		return;
	}
	int num = (int)numEntries;
	for (int i = 0; i < num; i++) {
		PakEntry pe;
		long long ll;
		bfile.read((char*)& ll, 8);
		pe.offset = (long)ll;
		bfile.read((char*)& ll, 8);
		pe.len = (long)ll;
		int name_len;
		bfile.read((char*)& name_len, 4);

		char* tex_name = new char[108 + 1];
		bfile.read((char*)tex_name, 108);
		tex_name[name_len] = '\0';
		//Log("pak entry name: " << tex_name << "\n");
		pe.name = std::string(tex_name);
		pe.pakname = binFile;
		pak_content[pe.name] = pe;
		delete[] tex_name;
	}
	// check:
	for (auto p : pak_content) {
		Log(" pak file entry: " << p.second.name.c_str() << endl);
	}
}

PakEntry* Util::findFileInPak(wstring filename)
{
	string name = w2s(filename);
	auto gotit = pak_content.find(name);
	if (gotit == pak_content.end()) {
		return nullptr;
	}
	return &gotit->second;
	//if (pak_content.count(name) == 0) {
	//	return nullptr;
	//}
	//PakEntry *pe = &pak_content[name];
	//return pe;
}

#if defined(DEBUG) || defined(_DEBUG)
#define FX_PATH L"..\\Debug\\"
#else
#define FX_PATH L"..\\Release\\"
#endif

#define TEXTURE_PATH L"..\\..\\data\\texture\\"
#define MESH_PATH L"..\\..\\data\\mesh\\"
#define SOUND_PATH L"..\\..\\data\\sound\\"

wstring Util::findFile(wstring filename, FileCategory cat, bool errorIfNotFound, bool generateFilenameMode) {
	// try without path:
	ifstream bfile(filename.c_str(), ios::in | ios::binary);
	if (!bfile) {
		// try with Debug or release path:
		switch (cat) {
		case FX:
			filename = FX_PATH + filename;
			break;
		case TEXTURE:
		case TEXTUREPAK:
			filename = TEXTURE_PATH + filename;
			break;
		case MESH:
			filename = MESH_PATH + filename;
			break;
		case SOUND:
			filename = SOUND_PATH + filename;
			break;
		}
		if (generateFilenameMode) {
			return filename.c_str();
		}
		bfile.open(filename.c_str(), ios::in | ios::binary);
		if (!bfile && cat == TEXTURE) {
			wstring oldname = filename;
			// try loading default texture
			filename = TEXTURE_PATH + wstring(L"default.dds");
			bfile.open(filename.c_str(), ios::in | ios::binary);
			if (bfile) Log("WARNING: texture " << oldname << " not found, replaced by default.dds texture" << endl);

		}
		if (!bfile && errorIfNotFound) {
			Error(L"failed reading file: " + filename);
		}
	}
	if (bfile) {
		bfile.close();
		return filename;
	}
	return wstring();
}

void Util::readFile(PakEntry* pakEntry, vector<byte>& buffer, FileCategory cat)
{
	Log("read file from pak: " << pakEntry->name.c_str() << endl);
	ifstream bfile(pakEntry->pakname.c_str(), ios::in | ios::binary);
	if (!bfile) {
		Error(L"failed re-opening pak file: " + pakEntry->pakname);
	}
	else {
		// position to start of file in pak:
		bfile.seekg(pakEntry->offset);
		buffer.resize(pakEntry->len);
		bfile.read((char*) & (buffer[0]), pakEntry->len);
		bfile.close();
	}
}

void Util::readFile(wstring filename, vector<byte>& buffer, FileCategory cat) {
	//ofstream f("fx\\HERE");
	//f.put('c');
	//f.flush();
	//f.close();
	//if (filename.)
	filename = findFile(filename, cat);
	ifstream bfile(filename.c_str(), ios::in | ios::binary);
	if (!bfile) {
		Error(L"failed reading file: " + filename);
	}
	else {
		streampos start = bfile.tellg();
		bfile.seekg(0, std::ios::end);
		streampos len = bfile.tellg() - start;
		bfile.seekg(start);
		buffer.resize((SIZE_T)len);
		bfile.read((char*) & (buffer[0]), len);
		bfile.close();
	}

}