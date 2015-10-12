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
using namespace std;
#include <assert.h>
#include <typeinfo.h>

#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
using namespace DirectX;
#include <d3d12.h>
#include <pix.h>

#include <wrl.h>
using namespace Microsoft::WRL;
using namespace DirectX;

#pragma comment(lib, "d3d12.lib")
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

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw;
	}
}

// framework headers
#include "d3dx12.h"
#include "gametime.h"
#include "util.h"
#include "world.h"
#include "camera.h"
#include "xapp.h"
#include "Effects\lines.h"
