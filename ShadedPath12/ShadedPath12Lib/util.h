/*
 * Utility methods
 */

inline wstring s2w(string s) {
	return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(s.c_str());
}
inline string w2s(wstring w) {
	return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(w);
}

wchar_t* string2wstring(std::string text);
struct error_desc { long e; char *id; long code; char *m; };
//void Log(std::string text);
char* iGetLastErrorText(DWORD nErrorCode);
void ThrowIfFailed(HRESULT hr);
error_desc* getErrorDesc(DWORD nErrorCode);

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);

class MathHelper
{
public:
	// Returns random float in [0, 1).
	static float RandF()
	{
		return (float)(rand()) / (float)RAND_MAX;
	}

	// Returns random float in [a, b).
	static float RandF(float a, float b)
	{
		return a + RandF()*(b - a);
	}
};

inline UINT calcConstantBufferSize(UINT originalSize) {
	return (originalSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1); // must be a multiple 256 bytes
};


inline XMVECTOR XM_CALLCONV XMVector3ProjectOnVector
(
FXMVECTOR vector,
FXMVECTOR targetVector
)
{
	// a projected on b is: ((ab)/(bb)) b
	XMVECTOR s = XMVectorDivide(XMVector3Dot(vector, targetVector), XMVector3Dot(targetVector, targetVector));
	return XMVectorScale(targetVector, XMVectorGetX(s));
}

inline XMVECTOR XM_CALLCONV XMVector3ReflecttOnVector
(
FXMVECTOR vector,
FXMVECTOR reflectionVector
)
{
	XMVECTOR p = XMVector3ProjectOnVector(vector, reflectionVector);
	return vector + (2 * (p-vector));
}

//struct TextElement;
//// provide helper class for fast vectors of several different types:
//// vectors never decrease in size, on return elemnts are already added, so that memory pointer to array may be used
//// returned size should be kept by app to hold current max value
//
//class VectorHelper {
//public:
//	size_t resize(vector<TextElement> &vec, size_t oldmax, size_t newmax) {
//		if (newmax <= oldmax) {
//			return oldmax;
//		}
//		vec.resize(newmax);
//		return newmax;
//	};
//};
//
//extern VectorHelper vectorHelper;

class Util {
public:

	// calculate distance from point to line, first point is considered starting point of beam, that goes indefinitely long
	// though 2nd point.
	// returns -1 if beam does not reach point at all (pointing to wrong side)
	static float distancePoint2Beam(XMVECTOR beamStart, XMVECTOR beamPoint2, XMVECTOR p);

	// calculate beam pointing away from an object by providing two pints in model coordinates: 
	// a start point and a 2nd point
	// the object's current rotation is applied to both points and object pos is added to get world coordinates for the beam
	// returned beam is in world coordinates
	// used WorldObjectmust have rotation specified with a quaternion
	//static void calcBeamFromObject(XMVECTOR *beamStart, XMVECTOR *beampoint2, WorldObject *o, XMVECTOR beamStartModelPose, XMVECTOR beamPoint2ModelPose);

	// decide if a beam emitting from source worldObject is hitting a target Object
	// using calcBeamFromObject(), see there for details
	// hitting distance is how far the beam can miss (center of) target to be considered a hit.
	// hitting distance defaults to 10 cm.
	//static bool isTargetHit(WorldObject *source, XMVECTOR beamStartModelPose, XMVECTOR beamPoint2ModelPose, WorldObject *target, float hittingDist = 0.1f);

	// move control point to specific distance to start point along line formed by the two
	// If you consider a beam starting from 1st point going through the second you will have the same beam afterwards, but the second crontrol 
	// point is at a specific distance to the start point
	static XMVECTOR movePointToDistance(XMVECTOR start, XMVECTOR controlPoint, float dist);

	static float distance3(XMFLOAT3 *a, XMFLOAT3 *b) {
		XMVECTOR av = XMLoadFloat3(a);
		XMVECTOR bv = XMLoadFloat3(b);
		return XMVectorGetX(XMVector3Length(bv - av));
	};

};

// log events to memory
class MemLog {

};