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

struct TextElement;
// provide helper class for fast vectors of several different types:
// vectors never decrease in size, on return elemnts are already added, so that memory pointer to array may be used
// returned size should be kept by app to hold current max value

class VectorHelper {
public:
	size_t resize(vector<TextElement> &vec, size_t oldmax, size_t newmax) {
		if (newmax <= oldmax) {
			return oldmax;
		}
		vec.resize(newmax);
		return newmax;
	};
};

extern VectorHelper vectorHelper;