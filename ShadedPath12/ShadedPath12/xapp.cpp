#include "stdafx.h"

XAppBase::XAppBase() {
	xapp = XApp::getInstance();
}

XAppBase::~XAppBase() {

}

void XAppBase::init() {

}

void XAppBase::update() {

}

void XAppBase::draw() {

}

void XAppBase::destroy() {

}

XApp::XApp() : /*camera(),*/ world(this) /*, vr(this)*/
{
	//camera.init();
	requestHeight = requestWidth = 0;
	mouseTodo = true;
	mouseDx = 0;
	mouseDy = 0;
	absFrameCount = 0;
	//appWindow.init(this); done during init()
	stats.init(this);
	//objectStore.xapp = this;
	//hud.setXApp(this);
}

XApp::~XApp()
{
}

void XApp::updateFrameStats() {
	absFrameCount++;
	LONGLONG old = gametime.getRealTime();
	gametime.advanceTime();
	double dt = gametime.getDeltaTime();
	if ((absFrameCount % 30) == 0) {
		// calculate fps every 30 frames
		LONGLONG now = gametime.getRealTime();
		double seconds = gametime.getSecondsBetween(old, now);
		fps = (int)(1 / seconds);
	}
}

void XApp::update() {
	//absFrameCount++;
	GetKeyboardState(key_state);
	LONGLONG old = gametime.getRealTime();
	gametime.advanceTime();
	double dt = gametime.getDeltaTime();
	if ((absFrameCount % 30) == 0) {
		// calculate fps every 30 frames
		LONGLONG now = gametime.getRealTime();
		double seconds = gametime.getSecondsBetween(old, now);
		fps = (int)(1 / seconds);
	}
	for (BYTE b = 1; b < 255; b++) {
		if (keyDown(b)) {
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
		;// camera.walk(dt);
	if (keyDown('S') || keyDown(VK_DOWN))
		;// camera.walk(-dt);
	if (keyDown('D') || keyDown(VK_RIGHT))
		;// camera.strafe(dt);
	if (keyDown('A') || keyDown(VK_LEFT))
		;// camera.strafe(-dt);

	;// camera.viewTransform();
	;// camera.projectionTransform();
	if (ovrRendering); // vr.nextTracking();
	if (mouseTodo && !ovrRendering) {
		mouseTodo = false;
		// mouse input
		float ROTATION_GAIN = 0.003f;
		float pitch = 0.0f; // camera.pitch;
		float yaw = 0.0f; // camera.yaw;
		XMFLOAT2 rotationDelta;
		rotationDelta.x = mouseDx * ROTATION_GAIN;   // scale for control sensitivity
		rotationDelta.y = mouseDy * ROTATION_GAIN;
		//Log(callnum++ << "mouse dx dy == " << mouseDx << " " << mouseDy);
		//Log(" delta x y == " << rotationDelta.x << " " << rotationDelta.y << "\n");

		// Update our orientation based on the command.
		pitch -= rotationDelta.y;
		yaw += rotationDelta.x;
		//Log("pich " << pitch);

		// Limit pitch to straight up or straight down.
		float limit = XM_PI / 2.0f - 0.01f;
		pitch = __max(-limit, pitch);
		pitch = __min(+limit, pitch);

		//Log(" " << pitch << endl);
		// Keep longitude in same range by wrapping.
		if (yaw > XM_PI)
		{
			yaw -= XM_PI * 2.0f;
		}
		else if (yaw < -XM_PI)
		{
			yaw += XM_PI * 2.0f;
		}
		//camera.pitch = pitch;
		//camera.yaw = yaw;
		//camera.apply_pitch_yaw();
		//camera.apply_yaw(camera.yaw);
	}
	XAppBase *app = getApp(appName);
	if (app != nullptr) {
		if (ovrRendering) {
			// currently we do nothing special for VR update: one single pass should be enough
			// different EyePos are then only generated for WVP generation before drawing
			// possible errors: Bounding boxes / vsibility calculations may be incorrect
			app->update();
		} else {
			app->update();
		}
	}
}

void XApp::importFrameFromRenderToApp()
{
	updateFrameStats();
	// Present the frame in app window, unless mirror is deactivated
	if (ovrMirror) {
		UINT frameIndex = appWindow.swapChain->GetCurrentBackBufferIndex();//getCurrentBackBufferIndex();
		lastPresentedFrame = frameIndex;
		appWindow.present();
	}
}

void XApp::draw() {
	//int frameIndex = getCurrentBackBufferIndex();
	if (ovrRendering) {
		//vr.startFrame();
	}
	//Log("draw " << frameIndex);
	int slot = threadState.waitForNextDrawSlot(app->draw_slot);
	//assert(slot == frameIndex);
	app->draw_slot = slot;
	app->absFrameCount = getAbsFrameCount();
	app->draw();
	//Log(" end " << frameIndex << " " << getAbsFrameCount() << endl);

	// Present the frame, if in VR this was already done by oculus SDK
	if (ovrMirror) {
		UINT frameIndex = appWindow.swapChain->GetCurrentBackBufferIndex();//getCurrentBackBufferIndex();
		lastPresentedFrame = frameIndex;
		appWindow.present();
	}
	if (!ovrRendering) {
		lastPresentedFrame = frameIndex;
		if (isShutdownMode()) {
			UINT n = shutdownAbsFrameStart;
		}
		//ThrowIfFailedWithDevice(swapChain->Present(0, 0), xapp().device.Get());
	}

	if (ovrRendering) {
		//vr.endFrame();
	}
	frameFinished();
}

#include "Initguid.h"
#include "DXGIDebug.h"

void XApp::destroy()
{
	// Wait for the GPU to be done with all resources.
	//WaitForGpu();
	app->destroy();
	appWindow.destroy();
	renderQueue.shutdown();
	workerQueue.shutdown();

	//Sleep(150);
	//CloseHandle(fenceEvent);
#ifdef _DEBUG
	//ThrowIfFailed(DXGIGetDebugInterface1(0, ));
	if (!disableDX12Debug) {
		typedef HRESULT(__stdcall *fPtr)(const IID&, void**);
		HMODULE hDll = GetModuleHandleW(L"dxgidebug.dll");
		fPtr DXGIGetDebugInterface = (fPtr)GetProcAddress(hDll, "DXGIGetDebugInterface");
		IDXGIDebug *pDxgiDebug;
		DXGIGetDebugInterface(__uuidof(IDXGIDebug), (void**)&pDxgiDebug);

		//pDxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
	}

#endif
}

void XApp::init()
{
	//// Basic initialization
	if (initialized) return;

	if (!ovrRendering) ovrMirror = false;
	if (ovrRendering);// vr.init();

	if (appName.length() == 0) {
		// no app name specified - just use first one from iterator
		auto it = appMap.begin();
		XAppBase *a = it->second;
		if (a != nullptr) {
			appName = it->first;
			Log("WARNING: xapp not specified, using this app: " << appName.c_str() << endl);
		}
	}
	//assert(appName.length() > 0);
	app = getApp(appName);
	if (app != nullptr) {
		//Log("initializing " << appName.c_str() << "\n");
		SetWindowText(getHWND(), string2wstring(app->getWindowTitle()));
	}
	else {
		Log("ERROR: xapp not available " << appName.c_str() << endl);
		// throw assertion error in debug mode
		assert(app != nullptr);
	}
#ifdef _DEBUG
	// Enable the D3D12 debug layer.
	{
		ComPtr<ID3D12Debug> debugController;
		ComPtr<ID3D12Debug1> debugController1;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			if (!disableDX12Debug) debugController->EnableDebugLayer();
			debugController->QueryInterface(IID_PPV_ARGS(&debugController1));
			if (debugController1) {
				debugController1->SetEnableGPUBasedValidation(true);
			} else {
				Log("WARNING: Could not enable GPU validation - ID3D12Debug1 controller not available" << endl);
			}
			HRESULT getAnalysis = DXGIGetDebugInterface1(0, __uuidof(pGraphicsAnalysis), reinterpret_cast<void**>(&pGraphicsAnalysis));
		} else {
			Log("WARNING: Could not get D3D12 debug interface - ID3D12Debug controller not available" << endl);
		}
	}
#endif

	//// Viewport and Scissor
/*	D3D12_RECT rect;
	if (GetWindowRect(getHWND(), &rect))
	{
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;
		viewport.MinDepth = 0.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;
		viewport.Width = static_cast<float>(width);
		viewport.Height = static_cast<float>(height);
		viewport.MaxDepth = 1.0f;

		scissorRect.left = 0;
		scissorRect.top = 0;
		scissorRect.right = static_cast<LONG>(width);
		scissorRect.bottom = static_cast<LONG>(height);
		vr.adaptViews(viewport, scissorRect);
	}
*/
	//// Pipeline 

	ComPtr<IDXGIFactory4> factory;
	UINT debugFlags = 0;
	if (!disableDX12Debug) {
		debugFlags = 0;
#ifdef _DEBUG
		debugFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	}

	ThrowIfFailed(CreateDXGIFactory2(debugFlags, IID_PPV_ARGS(&factory)));

	if (warp)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		// if this fails in debug run: enable win 10 dev mode
		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device)
			));
	}
	else {
		// if this fails in debug run: enable win 10 dev mode and/or disable d3d12 debug layer via command line parameter -disableDX12Debug 
		ThrowIfFailed(D3D12CreateDevice(
			nullptr,
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device)
			));
	}

	// disable auto alt-enter fullscreen switch (does leave an unresponsive window during debug sessions)
	ThrowIfFailed(factory->MakeWindowAssociation(getHWND(), DXGI_MWA_NO_ALT_ENTER));
	//IDXGIFactory4 *parentFactoryPtr = nullptr;
	//if (SUCCEEDED(swapChain->GetParent(__uuidof(IDXGIFactory4), (void **)&parentFactoryPtr))) {
	//	parentFactoryPtr->MakeWindowAssociation(getHWND(), DXGI_MWA_NO_ALT_ENTER);
	//	parentFactoryPtr->Release();
	//}

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	//ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));
	//commandQueue->SetName(L"commandQueue_xapp");

	calcBackbufferSizeAndAspectRatio();
	//camera.aspectRatio = aspectRatio;
	if (ovrRendering) {
		//camera.aspectRatio /= 2.0f;
	}

	//camera.projectionTransform();
	// Describe the swap chain.
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.BufferDesc.Width = backbufferWidth;
	swapChainDesc.BufferDesc.Height = backbufferHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = getHWND();
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;

	ComPtr<IDXGISwapChain> swapChain0; // we cannot use create IDXGISwapChain3 directly - create IDXGISwapChain, then call As() to map to IDXGISwapChain3
	//ThrowIfFailed(factory->CreateSwapChain(
	//	commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
	//	&swapChainDesc,
	//	&swapChain0
	//	));

	//ThrowIfFailed(swapChain0.As(&swapChain));
	////swapChain = nullptr;
	//frameIndex = xapp().swapChain->GetCurrentBackBufferIndex();

	if (ovrRendering) {
		//vr.initD3D();
	}

	// Create descriptor heaps.
	{
	}


	//// Assets

	// Create an empty root signature.
	{
	}
	// 11 on 12 device support
//	// 11 on 12 end

	//camera.ovrCamera = true;
	if (!ovrRendering); // camera.ovrCamera = false;
#if !defined(_OVR_)
	// vr camera does not make sense without ovr enabled
	//camera.ovrCamera = false;
#endif

	gametime.init(1); // init to real time
	//camera.setSpeed(1.0f);

	initPakFiles();
	dxmanager.init(this, XApp::FrameCount);
	renderControl.init(this);
	threadState.init();
	appWindow.init(this, factory);
	textureStore.init(this);

	app->init();
	initialized = true;
	app->update();
}

void XApp::startWorkerThreads()
{
	int numThreads = this->getMaxThreadCount();
	assert(0 < numThreads && numThreads < 10);
	// run render thread:
	workerThreads.add_t(Command::renderQueueTask, this);
	// run worker threads:
	for (int i = 0; i < numThreads; i++) {
		workerThreads.add_t(Command::task, this);
	}
	Log("Worker Threads started: " << numThreads << endl);
	//Log("main thread: " << ThreadInfo::thread_osid() << endl);
}

void XApp::initPakFiles()
{
	wstring binFile = findFile(L"texture01.pak", XApp::TEXTUREPAK, false);
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
	bfile.read((char*)&magic, 8);
	magic = _byteswap_uint64(magic);
	if (magic != 0x5350313250414B30L) {
		// magic "SP12PAK0" not found
		Log("pak file invalid: " << binFile << endl);
		return;
	}
	long long numEntries;
	bfile.read((char*)&numEntries, 8);
	if (numEntries > 30000) {
		Log("pak file invalid: contained number of textures: " << numEntries << endl);
		return;
	}
	int num = (int)numEntries;
	for (int i = 0; i < num; i++) {
		PakEntry pe;
		long long ll;
		bfile.read((char*)&ll, 8);
		pe.offset = (long)ll;
		bfile.read((char*)&ll, 8);
		pe.len = (long)ll;
		int name_len;
		bfile.read((char*)&name_len, 4);

		char *tex_name = new char[108 + 1];
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

PakEntry * XApp::findFileInPak(wstring filename)
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



void XApp::calcBackbufferSizeAndAspectRatio()
{
	// Full HD is default - should be overidden by specific devices like Rift
	backbufferHeight = 1080;
	backbufferWidth = 1920;
	if (ovrRendering) {
		//if (vr.getHeight() > 0) backbufferHeight = vr.getHeight();
		//if (vr.getWidth() > 0) backbufferWidth = vr.getWidth();
	}
	aspectRatio = static_cast<float>(backbufferWidth) / static_cast<float>(backbufferHeight);
	//viewport.MinDepth = 0.0f;
	//viewport.TopLeftX = 0.0f;
	//viewport.TopLeftY = 0.0f;
	//viewport.Width = static_cast<float>(backbufferWidth);
	//viewport.Height = static_cast<float>(backbufferHeight);
	//viewport.MaxDepth = 1.0f;

	//scissorRect.left = 0;
	//scissorRect.top = 0;
	//scissorRect.right = static_cast<LONG>(backbufferWidth);
	//scissorRect.bottom = static_cast<LONG>(backbufferHeight);
	//vr.prepareViews(viewport, scissorRect);
}

#if defined(DEBUG) || defined(_DEBUG)
#define FX_PATH L"..\\Debug\\"
#else
#define FX_PATH L"..\\Release\\"
#endif

#define TEXTURE_PATH L"..\\..\\data\\texture\\"
#define MESH_PATH L"..\\..\\data\\mesh\\"
#define SOUND_PATH L"..\\..\\data\\sound\\"

wstring XApp::findFile(wstring filename, FileCategory cat, bool errorIfNotFound, bool generateFilenameMode) {
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

void XApp::readFile(PakEntry * pakEntry, vector<byte>& buffer, FileCategory cat)
{
	Log("read file from pak: " << pakEntry->name.c_str() << endl);
	ifstream bfile(pakEntry->pakname.c_str(), ios::in | ios::binary);
	if (!bfile) {
		Error(L"failed re-opening pak file: " + pakEntry->pakname);
	} else {
		// position to start of file in pak:
		bfile.seekg(pakEntry->offset);
		buffer.resize(pakEntry->len);
		bfile.read((char*)&(buffer[0]), pakEntry->len);
		bfile.close();
	}
}

void XApp::readFile(wstring filename, vector<byte> &buffer, FileCategory cat) {
	//ofstream f("fx\\HERE");
	//f.put('c');
	//f.flush();
	//f.close();
	//if (filename.)
	filename = findFile(filename, cat);
	ifstream bfile(filename.c_str(), ios::in | ios::binary);
	if (!bfile) {
		Error(L"failed reading file: " + filename);
	} else {
		streampos start = bfile.tellg();
		bfile.seekg(0, std::ios::end);
		streampos len = bfile.tellg() - start;
		bfile.seekg(start);
		buffer.resize((SIZE_T)len);
		bfile.read((char*)&(buffer[0]), len);
		bfile.close();
	}

}

bool XApp::keyDown(BYTE key) {
	return (key_state[key] & 0x80) != 0;
}

void XApp::registerApp(string name, XAppBase *app)
{
	// check for class name and strip away the 'class ' part:
	if (name.find("class ") != string::npos) {
		name = name.substr(name.find_last_of(' '/*, name.size()*/) + 1);
	}
	appMap[name] = app;
	Log("xapp registered: " << name.c_str() << endl);
	//for_each(appMap.begin(), appMap.end(), [](auto element) {
	//	Log("xapp registered: " << element.first.c_str() << endl);
	//});
}

void XApp::parseCommandLine(string commandline) {
	vector<string> topics = split(commandline, ' ');
	for (string s : topics) {
		if (s.at(0) != '-') continue;
		s = s.substr(1);
		size_t eqPos = s.find('=');
		if (eqPos == string::npos) {
			// parse options without '='
			parameters[s] = "true";
		}
		else {
			// parse key=value options
			vector<string> kv = split(s, '=');
			if (kv.size() != 2) continue;
			parameters[kv[0]] = kv[1];
		}
		Log("|" << s.c_str() << "|" << endl);
	}
}

bool XApp::getBoolParam(string key, bool default_value) {
	string s = parameters[key];
	if (s.size() == 0) return default_value;
	if (s.compare("false") == 0) return false;
	return true;
}

int XApp::getIntParam(string key, int default_value) {
	string s = parameters[key];
	if (s.size() == 0) return default_value;
	istringstream buffer(s);
	int value;
	buffer >> value;
	return value;
}


XAppBase* XApp::getApp(string appName) {
	return appMap[appName];
}

void XApp::setRunningApp(string app) {
	appName = app;
}

void XApp::setHWND(HWND h) {
	hwnd = (HWND)h;
}

HWND XApp::getHWND() {
	return hwnd;
}

void XApp::resize(int width, int height) {
	// bail out if size didn't actually change, e.g. on minimize/maximize operation
	//if (width == xapp->requestWidth && height == xapp->requestHeight) return;

	requestWidth = width;
	requestHeight = height;
	resize();
}

void XApp::resize() {
}

void XApp::frameFinished() {
	rtvCleared = false;
}

// global instance:
static XApp *xappPtr = nullptr;

XApp* XApp::getInstance() {
	if (xappPtr == nullptr) {
		xappPtr = new XApp();
	}
	return xappPtr;
}

void xappDestroy() {
	delete xappPtr;
}

void XAppMultiBase::initAllApps()
{
	for ( auto app : apps)
	{
		app->init();
	}
}

void XApp::handleRTVClearing(ID3D12GraphicsCommandList * commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle, D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle, ID3D12Resource * resource)
{
	if (rtvHasToBeCleared()) {
		rtvCleared = true;
		ResourceStateHelper *resourceStateHelper = ResourceStateHelper::getResourceStateHelper();
		resourceStateHelper->toState(resource, D3D12_RESOURCE_STATE_RENDER_TARGET, commandList);
		commandList->ClearRenderTargetView(rtv_handle, clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		//resourceStateHelper->toState(resource, D3D12_RESOURCE_STATE_PRESENT, commandList);
	}
}

