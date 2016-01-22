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
#include <fstream>
#include <array>
#include <vector>
#include <unordered_map>
#include <locale>
#include <codecvt>
#include <mutex>
#include <future>
//#include <new>
using namespace std;
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
#include <d3d11on12.h>
#include <d3d12.h>
#include <pix.h>

#include <wrl.h>
using namespace Microsoft::WRL;
using namespace DirectX;

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
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

// framework headers
#include "d3dx12.h"
#include "DDSTextureLoader.h"
#include "texture.h"
#include "gametime.h"
#include "util.h"
#include "world.h"
#include "camera.h"
#include "vr.h"
#include "xapp.h"
#include "Effects\effectbase.h"
#include "Effects\lines.h"
#include "Effects\dotcross.h"
#include "Effects\linetext.h"
#include "Effects\posteffect.h"
