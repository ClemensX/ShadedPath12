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

void Util::updateKeyboardState()
{
	BOOL result = GetKeyboardState(key_state);
	if (result == 0) {
		Log("ERROR GetKeyboardState");
		return;
	}
	for (BYTE b = 1; b < 255; b++) {
		if (keyDown(b)) {
			//Log("key down");
			switch (b) {
				// prevent toggle keys from triggering keydown state
			case VK_CAPITAL:
			case VK_NUMLOCK:
			case VK_SCROLL:
				// nothing to do
				break;
			default: anyKeyDown = true;
			}
		}
	}
	// handle keyboard input
	if (keyDown('W') || keyDown(VK_UP))
		Log("W");// camera.walk(dt);
	if (keyDown('S') || keyDown(VK_DOWN))
		;// camera.walk(-dt);
	if (keyDown('D') || keyDown(VK_RIGHT))
		;// camera.strafe(dt);
	if (keyDown('A') || keyDown(VK_LEFT))
		;// camera.strafe(-dt);


}

bool Util::keyDown(BYTE key) {
	return (key_state[key] & 0x80) != 0;
}

