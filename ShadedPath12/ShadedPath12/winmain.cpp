// ShadedPath12.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "winmain.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void registerRawInput()
{
	RAWINPUTDEVICE Rid[2];

	Rid[0].usUsagePage = 0x01;
	Rid[0].usUsage = 0x02;
	Rid[0].dwFlags = 0;// RIDEV_NOLEGACY;   // adds HID mouse and also ignores legacy mouse messages
	Rid[0].hwndTarget = 0;

	//Rid[1].usUsagePage = 0x01;
	//Rid[1].usUsage = 0x06;
	//Rid[1].dwFlags = 0;// RIDEV_NOLEGACY;   // adds HID keyboard and also ignores legacy keyboard messages
	//Rid[1].hwndTarget = 0;

	if (RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])) == FALSE) {
		Log("raw input not working!!");
	}
}

void handleRawInput(LPARAM lParam, XApp *xapp)
{
	BYTE* keystates = xapp->key_state;
	UINT dwSize;
	// store last x and y coord for use with absilute mouse coords, -1 indicates not being used yet
	static LONG lastx = -1;
	static LONG lasty = -1;
	// absolute mouse coords seem to jump many positions for a small movement, use this to divide to smaller values:
	static LONG ABS_MOUSE_DIVIDER = 32;


	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
	LPBYTE lpb = new BYTE[dwSize];
	if (lpb == NULL)
	{
		return;
	}

	if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
		OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));

	RAWINPUT* raw = (RAWINPUT*)lpb;
	TCHAR szTempOutput[500];
	HRESULT hResult;
	// TODO raw keyboard no longer used - remove!!!
	if (raw->header.dwType == RIM_TYPEKEYBOARD)
	{
		hResult = StringCchPrintf(szTempOutput, STRSAFE_MAX_CCH, TEXT(" Kbd: make=%04x Flags:%04x Reserved:%04x ExtraInformation:%08x, msg=%04x VK=%04x \n"),
			raw->data.keyboard.MakeCode,
			raw->data.keyboard.Flags,
			raw->data.keyboard.Reserved,
			raw->data.keyboard.ExtraInformation,
			raw->data.keyboard.Message,
			raw->data.keyboard.VKey);
		if (FAILED(hResult))
		{
			// TODO: write error handler
		}
		//OutputDebugString(szTempOutput);
		USHORT key = raw->data.keyboard.VKey;
		if ((raw->data.keyboard.Flags & RI_KEY_BREAK) && key < 255) {
			// key is not up
			keystates[key] = true;
			xapp->anyKeyDown = true;  // important to reset this at end of frame;
		}
		else {
			keystates[key] = false;
		}
	}
	else if (raw->header.dwType == RIM_TYPEMOUSE)
	{
		hResult = StringCchPrintf(szTempOutput, STRSAFE_MAX_CCH, TEXT("Mouse: usFlags=%04x ulButtons=%04x usButtonFlags=%04x usButtonData=%04x ulRawButtons=%04x lLastX=%04x lLastY=%04x ulExtraInformation=%04x\r\n"),
			raw->data.mouse.usFlags,
			raw->data.mouse.ulButtons,
			raw->data.mouse.usButtonFlags,
			raw->data.mouse.usButtonData,
			raw->data.mouse.ulRawButtons,
			raw->data.mouse.lLastX,
			raw->data.mouse.lLastY,
			raw->data.mouse.ulExtraInformation);

		if (FAILED(hResult))
		{
			// TODO: write error handler
		}
		//OutputDebugString(szTempOutput);
		if (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
			// when using e.g. virtual desktops Windows decides to send absolue instead of relative coord
			// then we must recalculate this to be relative
			if (lastx == -1) {
				lastx = raw->data.mouse.lLastX;
			}
			if (lasty == -1) {
				lasty = raw->data.mouse.lLastY;
			}
			xapp->mouseDx = raw->data.mouse.lLastX - lastx;
			xapp->mouseDy = raw->data.mouse.lLastY - lasty;
			xapp->mouseDx /= ABS_MOUSE_DIVIDER;
			xapp->mouseDy /= ABS_MOUSE_DIVIDER;
			lastx = raw->data.mouse.lLastX;
			lasty = raw->data.mouse.lLastY;
		} else {
			// relative mouse coord - use them directly
			xapp->mouseDx = raw->data.mouse.lLastX;
			xapp->mouseDy = raw->data.mouse.lLastY;
		}
		xapp->mouseTodo = true;
	}

	delete[] lpb;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#if defined(DEBUG) | defined(_DEBUG)
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	// debug heap test
	char *carray = new char[100];
	char *c2array = (char*)malloc(120);
	// debug heap end

	wstring wcmd = wstring(lpCmdLine);
	string cmd = w2s(wcmd);
	xapp().commandline = cmd;
	xapp().parseCommandLine(xapp().commandline);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SHADEDPATH12, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SHADEDPATH12));

	MSG msg = { 0 };
	while (true)
	{
		// Process any messages in the queue.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				break;
		} else {
			xapp().update();
			xapp().draw();
		}
	}
	xapp().destroy();

	// debug heap test
	//_RPTF0(_CRT_WARN, "heap report test\n");
	//_CrtMemDumpAllObjectsSince(NULL);
	delete carray;
	free(c2array);
	// debug heap end
	xappDestroy();
    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SHADEDPATH12));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SHADEDPATH12);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	registerRawInput();
	hInst = hInstance; // Store instance handle in our global variable

    // Create the main window. 
    string name = xapp().parameters["app"];
    if (name.length() > 0) {
		xapp().setRunningApp(name);
    }
    Log("++++ " << xapp().parameters["w"].c_str() << endl);
    bool isFullscreen = xapp().getBoolParam("fullscreen");
    Log("isFullscreen: " << isFullscreen << endl);
    int w = xapp().getIntParam("w", CW_USEDEFAULT);
    int h = xapp().getIntParam("h", CW_USEDEFAULT);
	xapp().ovrRendering = xapp().getBoolParam("ovr");
    Log("ovrRendering: " << xapp().ovrRendering << endl);

    int style;
    if (isFullscreen) {
	    style = 0;
    }
    else {
	    style = WS_OVERLAPPEDWINDOW;
    }
    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, w, h, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   xapp().setHWND(hWnd);
   xapp().init();

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_INPUT:
		handleRawInput(lParam, &xapp());
		return 0;
	case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
		// immediately validate all regions - drawing done entirely by X
		//ValidateRect(hWnd, nullptr);
		//return 0;
		{
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
