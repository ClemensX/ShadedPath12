// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <Strsafe.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <array>
#include <vector>
#include <unordered_map>
#include <locale>
#include <codecvt>
#include <mutex>
#include <queue>
#include <future>
#include <condition_variable>
#include <regex>
//#include <new>
using namespace std;
//#include <filesystem>  // moved to class containinf filesystem code due to strange header compile error
//using namespace std::tr2::sys;
#include <assert.h>
#include <typeinfo.h>

#include <DXGItype.h>
#include <dxgi1_4.h>
#include <DXProgrammableCapture.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
using namespace DirectX;
#include <d3d12sdklayers.h>
#include <d3d11on12.h>
#include <d3d12.h>
#include <pix.h>

#include <wrl.h>
using namespace Microsoft::WRL;
using namespace DirectX;

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "xaudio2")
//d2d1.lib;dwrite.lib;d3d11.lib;d3d12.lib;dxgi.lib;

#define _CRTDBG_MAP_ALLOC
#ifdef _DEBUG
#ifndef DBG_NEW
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#define new DBG_NEW
#endif
#endif  // _DEBUG
#pragma warning( disable : 4005 )
#include <crtdbg.h>
#pragma warning( default : 4005 )

inline void LogFile(const WCHAR *s) {
	static bool firstcall = true;
	ios_base::openmode mode;
	if (firstcall) {
		mode = ios_base::out;
		firstcall = false;
	}
	else {
		mode = ios_base::out | ios_base::app;
	}
	std::wofstream out("xapp_run.log", mode);
	out << s;
	out.close();
}

#if defined(DEBUG) | defined(_DEBUG)
#define LogCond(y,x) if(y){Log(x)}
#define Log(x)\
{\
	wstringstream s1764;  s1764 << x; \
	OutputDebugString(s1764.str().c_str()); \
}
#elif defined(LOGFILE)
#define Log(x)\
{\
	wstringstream s1764;  s1764 << x; \
	LogFile(s1764.str().c_str()); \
}
#else
#define Log(x)
#define LogCond(y,x)
#endif

#define LogF(x)\
{\
	wstringstream s1764;  s1764 << x; \
	LogFile(s1764.str().c_str()); \
}


inline void ThrowIfFailedWithDevice(HRESULT hr, ID3D12Device *device)
{
	if (FAILED(hr))
	{
		if (hr == 0x887a0005) {
			Log("Device was removed ");
			HRESULT hr2 = device->GetDeviceRemovedReason();
			Log(hr << endl);
			throw;
		}
		throw;
	}
}
inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw;
	}
}
inline void ErrorExt(wstring msg, const char* file, DWORD line)
{
	wstringstream s;
	s << "ERROR " << msg << '\n';
	s << file << " " << line << '\n';
	Log(s.str());
	s << "\n\nClick 'yes' to break and 'no' to continue.";
	int nResult = MessageBoxW(GetForegroundWindow(), s.str().c_str(), L"Unexpected error encountered", MB_YESNO | MB_ICONERROR);
	if (nResult == IDYES)
		DebugBreak();
}

#define Error(x) ErrorExt((x), __FILE__,  (DWORD)__LINE__)

// Assign a name to the object to aid with debugging.
#if defined(_DEBUG)
inline void SetName(ID3D12Object* pObject, LPCWSTR name)
{
	pObject->SetName(name);
}
inline void SetName(ID3D12Object* pObject, LPCWSTR name, int i)
{
	wstringstream s;
	s << L"" << name << "_" << i;
	pObject->SetName(s.str().c_str());
}
inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
{
	WCHAR fullName[50];
	if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
	{
		pObject->SetName(fullName);
	}
}
#else
inline void SetName(ID3D12Object*, LPCWSTR)
{
}
inline void SetName(ID3D12Object* pObject, LPCWSTR name, int i)
{
}
inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT)
{
}
#endif

// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT_STR_HELPER(x) #x
#define NAME_D3D12_OBJECT_STR(x) NAME_D3D12_OBJECT_STR_HELPER(x)
#define NAME_D3D12_OBJECT(x) SetName(x.Get(), L#x)
#define NAME_D3D12_OBJECT_SUFF(x,y) SetName(x.Get(), L#x, y)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed(x[n].Get(), L#x, n)

//#define DXGI_FORMAT_R8G8B8A8_UNORM_SRGB DXGI_FORMAT_R8G8B8A8_UNORM
// framework headers
#include <XAudio2.h>
#include <x3daudio.h>
//#include "sound.h"
#include "d3dx12.h"
#include "DDSTextureLoader.h"
#include "forward_structures.h"
#include "frameresource.h"
#include "dxmanager.h"
#include "texture.h"
#include "gametime.h"
#include "util.h"
#include "lights.h"
#include "animation.h"
//#include "path.h"
#include "world.h"
//#include "camera.h"
//#include "worldObject.h"
//#include "vr.h"
#include "threads.h"
#include "rendercontrol.h"
#include "applicationWindow.h"
#include "stats.h"
#include "xapp.h"
#include "Effects\effectbase.h"
#include "Effects\copytexture.h"
//#include "Effects\lines.h"
//#include "Effects\dotcross.h"
//#include "Effects\linetext.h"
//#include "Effects\billboard.h"
//#include "Effects\objecteffect.h"
#include "Effects\posteffect.h"
//#include "meshObject.h"
