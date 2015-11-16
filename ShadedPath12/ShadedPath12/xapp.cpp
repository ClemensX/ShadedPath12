#include "stdafx.h"
#include "xapp.h"

XAppBase::XAppBase() {
}

XAppBase::~XAppBase() {

}

void XAppBase::init() {

}

void XAppBase::update() {

}

void XAppBase::draw() {

}

void XAppBase::next() {

}

void XAppBase::destroy() {

}

XApp::XApp() : camera(world), world(this), vr(this)
{
	requestHeight = requestWidth = 0;
	mouseTodo = true;
	mouseDx = 0;
	mouseDy = 0;
	framenum = 0;
	//objectStore.xapp = this;
	//hud.setXApp(this);
}

XApp::~XApp()
{
}

void XApp::update() {
	static long callnum = 0;
	GetKeyboardState(key_state);
	LONGLONG old = gametime.getRealTime();
	gametime.advanceTime();
	float dt = gametime.getDeltaTime();
	if ((framenum % 30) == 0) {
		// calculate fps every 30 frames
		LONGLONG now = gametime.getRealTime();
		float seconds = gametime.getSecondsBetween(old, now);
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
		camera.walk(dt);
	if (keyDown('S') || keyDown(VK_DOWN))
		camera.walk(-dt);
	if (keyDown('D') || keyDown(VK_RIGHT))
		camera.strafe(dt);
	if (keyDown('A') || keyDown(VK_LEFT))
		camera.strafe(-dt);

	// Query the HMD for the current tracking state.
	camera.viewTransform();
	camera.projectionTransform();
	//if (ovrRendering) camera.recalcOVR(*this);
#if defined (ENABLE_OVR2)
	if (ovrRendering && false) {
		ovrTrackingState ts = ovrHmd_GetTrackingState(hmd, ovr_GetTimeInSeconds());
		if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked)) {
			//auto pose = ts.HeadPose;
			//Log(" head pose" << pose.ThePose.Orientation.x << " " << pose.ThePose.Orientation.y << " " << pose.ThePose.Orientation.z << endl);
			//Posef pose = ts.HeadPose;
			ovrPoseStatef pose = ts.HeadPose;// .ThePose;

											 // fake ovr
											 //pose.ThePose.Orientation.x = 0.1f;
											 //pose.ThePose.Orientation.y = 0.1f;
											 //pose.ThePose.Orientation.z = 0.1f;

											 // try new method:
			XMFLOAT4 af = XMFLOAT4(1.0f, 2.0f, 0.0f, 0.0f);
			XMFLOAT4 bf = XMFLOAT4(4.5f, 1.5f, 0.0f, 0.0f);
			XMVECTOR a = XMLoadFloat4(&af);
			XMVECTOR b = XMLoadFloat4(&bf);
			//XMVECTOR proj = XMVector3ProjectOnVector(a, b);
			XMVECTOR proj = XMVector3ReflecttOnVector(a, b);
			XMFLOAT4 p;
			XMStoreFloat4(&p, proj);
			//Log(" vec " << p.x << " " << p.y << endl);

			//Log(" head pose" << pose.ThePose.Orientation.x << " " << pose.ThePose.Orientation.y << " " << pose.ThePose.Orientation.z << endl);
			//float yaw, eyePitch, eyeRoll;
			//pose.ThePose.Orientation.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &eyePitch, &eyeRoll);
			XMFLOAT4 lookQ = XMFLOAT4(pose.ThePose.Orientation.x, pose.ThePose.Orientation.y, pose.ThePose.Orientation.z, pose.ThePose.Orientation.w);
			XMVECTOR lookQV = XMLoadFloat4(&lookQ);
			//lookQV = XMQuaternionNormalize(lookQV);  //TODO do we need to normalize here?
			XMMATRIX lookM = XMMatrixRotationQuaternion(lookQV);
			// fix orientation of reversed coords:
			////lookM = XMMatrixScaling(1.0f, -1.0f, 1.0f) * lookM;
			lookM = (XMMatrixScaling(-1.0f, -1.0f, -1.0f) * lookM) * XMMatrixScaling(-1.0f, -1.0f, -1.0f);
			//lookM = XMMatrixTranspose(lookM);

			//const XMFLOAT3* camPos = &world.path.getPos(PATHID_CAM_INTRO, xapp->gametime.getRealTime(), 0);
			//camera.pos
			XMFLOAT4 pos(camera.pos.x, camera.pos.y, camera.pos.z, 0.0f);
			XMFLOAT4 target(0.0f, 0.0f, 100.0f, 1.0f);
			//XMFLOAT4 target(camera.pos.x, camera.pos.y, camera.pos.z + 100.0f, 1.0f);
			XMVECTOR targetV = XMLoadFloat4(&target);
			XMVECTOR axis = targetV;
			targetV = XMVector3Transform(targetV, lookM);
			targetV = XMVector3ReflecttOnVector(targetV, axis);
			XMFLOAT4 up(0.0f, 1.0f, 0.0f, 0.0f);
			XMFLOAT4 xmpos, xmtarget, xmup;
			xmpos = XMFLOAT4(pos);
			XMStoreFloat4(&xmtarget, targetV);
			// adjust lookat:
			xmtarget.x += camera.pos.x;
			xmtarget.y += camera.pos.y;
			xmtarget.z += camera.pos.z;

			//xmtarget.y *= -1.0f;
			//xmtarget.x *= -1.0f;
			// adjust up vector:
			XMVECTOR upPoseV = XMLoadFloat4(&lookQ);
			XMMATRIX upM = XMMatrixRotationQuaternion(lookQV);
			XMVECTOR upV = XMLoadFloat4(&up);
			axis = upV;
			upV = XMVector3Transform(upV, upM);
			upV = XMVector3ReflecttOnVector(upV, axis);
			XMStoreFloat4(&xmup, upV);
			//xmup.y *= -1.0f;

			xmup = XMFLOAT4(up);
			camera.lookAt(xmpos, xmtarget, xmup);
			camera.viewTransform();
			// now do it the oculus way:
			// Get both eye poses simultaneously, with IPD offset already included. 
			ovrVector3f useHmdToEyeViewOffset[2] = { EyeRenderDesc[0].HmdToEyeViewOffset, EyeRenderDesc[1].HmdToEyeViewOffset };
			ovrPosef temp_EyeRenderPose[2];
			ovrHmd_GetEyePoses(hmd, 0, useHmdToEyeViewOffset, temp_EyeRenderPose, &ts);

			ovrPosef    * useEyePose = &EyeRenderPose[0];
			float       * useYaw = &YawAtRender[0];
			float Yaw = XM_PI;
			*useEyePose = temp_EyeRenderPose[0];
			*useYaw = Yaw;

			// Get view and projection matrices (note near Z to reduce eye strain)
			Matrix4f rollPitchYaw = Matrix4f::RotationY(Yaw);
			Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(useEyePose->Orientation);
			Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
			Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
			Vector3f Pos;
			Pos.x = camera.pos.x;
			Pos.y = camera.pos.y;
			Pos.z = camera.pos.z;
			Vector3f shiftedEyePos = Pos + rollPitchYaw.Transform(useEyePose->Position);

			Matrix4f view = Matrix4f::LookAtLH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
			Matrix4f projO = ovrMatrix4f_Projection(EyeRenderDesc[0].Fov, 0.2f, 1000.0f, true);
			float f = projO.M[0][0];
			XMFLOAT4X4 xmProjO;
			xmProjO._11 = projO.M[0][0];
			xmProjO._12 = projO.M[0][1];
			xmProjO._13 = projO.M[0][2];
			xmProjO._14 = projO.M[0][3];
			xmProjO._21 = projO.M[1][0];
			xmProjO._22 = projO.M[1][1];
			xmProjO._23 = projO.M[1][2];
			xmProjO._24 = projO.M[1][3];
			xmProjO._31 = projO.M[2][0];
			xmProjO._32 = projO.M[2][1];
			xmProjO._33 = projO.M[2][2];
			xmProjO._34 = projO.M[2][3];
			xmProjO._41 = projO.M[3][0];
			xmProjO._42 = projO.M[3][1];
			xmProjO._43 = projO.M[3][2];
			xmProjO._44 = projO.M[3][3];
			XMMATRIX xmProjOM = XMLoadFloat4x4(&xmProjO);
			camera.projection = xmProjO;
		}
	}
#endif
	if (mouseTodo /*&& !ovrRendering*/) {
		mouseTodo = false;
		// mouse input
		float ROTATION_GAIN = 0.003f;
		float pitch = camera.pitch;
		float yaw = camera.yaw;
		XMFLOAT2 rotationDelta;
		rotationDelta.x = mouseDx * ROTATION_GAIN;   // scale for control sensitivity
		rotationDelta.y = mouseDy * ROTATION_GAIN;
		//Log(callnum++ << "mouse dx dy == " << mouseDx << " " << mouseDy);
		//Log(" delta x y == " << rotationDelta.x << " " << rotationDelta.y << "\n");

		// Update our orientation based on the command.
		pitch -= rotationDelta.y;
		yaw += rotationDelta.x;

		// Limit pitch to straight up or straight down.
		float limit = XM_PI / 2.0f - 0.01f;
		pitch = __max(-limit, pitch);
		pitch = __min(+limit, pitch);

		// Keep longitude in same range by wrapping.
		if (yaw > XM_PI)
		{
			yaw -= XM_PI * 2.0f;
		}
		else if (yaw < -XM_PI)
		{
			yaw += XM_PI * 2.0f;
		}
		camera.pitch = pitch;
		camera.yaw = yaw;
		camera.apply_pitch_yaw();
		//camera.apply_yaw(camera.yaw);
	}
	XAppBase *app = getApp(appName);
	if (app != nullptr) {
		if (ovrRendering) {
			// currently we do nothing special for VR update: one single pass shouls be enough
			// different EyePos are then only generated for WVP generation before drawing
			// possible errors: Bounding boxes / vsibility calculations may be incorrect
			app->update();
		} else {
			app->update();
		}
	}
}

void XApp::draw() {
	if (ovrRendering) {
		vr.startFrame();
	}
	app->draw();

	// Present the frame.
	lastPresentedFrame = swapChain->GetCurrentBackBufferIndex();
	ThrowIfFailedWithDevice(swapChain->Present(0, 0), xapp().device.Get());

	app->next();
	//MoveToNextFrame();
	if (ovrRendering) {
		vr.endFrame();
	}
}

#include "Initguid.h"
#include "DXGIDebug.h"

void XApp::destroy()
{
	// Wait for the GPU to be done with all resources.
	//WaitForGpu();
	app->destroy();

	Sleep(150);
	//CloseHandle(fenceEvent);
#ifdef _DEBUG
	//ThrowIfFailed(DXGIGetDebugInterface1(0, ));
	typedef HRESULT(__stdcall *fPtr)(const IID&, void**);
	HMODULE hDll = GetModuleHandleW(L"dxgidebug.dll");
	fPtr DXGIGetDebugInterface = (fPtr)GetProcAddress(hDll, "DXGIGetDebugInterface");
	IDXGIDebug *pDxgiDebug;
	DXGIGetDebugInterface(__uuidof(IDXGIDebug), (void**)&pDxgiDebug);

	//pDxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);

#endif
}

void XApp::init()
{
	//// Basic initialization
	if (initialized) return;

	initialized = true;
	camera.ovrCamera = true;
	if (!ovrRendering) camera.ovrCamera = false;

	gametime.init(1); // init to real time
	camera.setSpeed(1.0f);

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
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
		HRESULT getAnalysis = DXGIGetDebugInterface1(0, __uuidof(pGraphicsAnalysis), reinterpret_cast<void**>(&pGraphicsAnalysis));
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
	ThrowIfFailed(CreateDXGIFactory2(0
#ifdef _DEBUG
		| DXGI_CREATE_FACTORY_DEBUG
#endif
		, IID_PPV_ARGS(&factory)));

	if (warp)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device)
			));
	}
	else {
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

	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));
	commandQueue->SetName(L"commandQueue_xapp");

	calcBackbufferSizeAndAspectRatio();
	camera.aspectRatio = aspectRatio;
	if (ovrRendering) {
		camera.aspectRatio /= 2.0f;
	}
	camera.projectionTransform();
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
	ThrowIfFailed(factory->CreateSwapChain(
		commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		&swapChainDesc,
		&swapChain0
		));

	ThrowIfFailed(swapChain0.As(&swapChain));
	frameIndex = swapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));
		rtvHeap->SetName(L"rtvHeap_xapp");

		rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
			device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
			wstringstream s;
			s << L"renderTarget_xapp[" << n << "]";
			renderTargets[n]->SetName(s.str().c_str());
			rtvHandle.Offset(1, rtvDescriptorSize);
			//ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[n])));
		}
	}

	//// Assets

	// Create an empty root signature.
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
	}

	app->init();
	app->update();
}

void XApp::calcBackbufferSizeAndAspectRatio()
{
	// Full HD is default - should be overidden by specific devices like Rift
	backbufferHeight = 1080;
	backbufferWidth = 1920;
	aspectRatio = static_cast<float>(backbufferWidth) / static_cast<float>(backbufferHeight);
	viewport.MinDepth = 0.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(backbufferWidth);
	viewport.Height = static_cast<float>(backbufferHeight);
	viewport.MaxDepth = 1.0f;

	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = static_cast<LONG>(backbufferWidth);
	scissorRect.bottom = static_cast<LONG>(backbufferHeight);
	vr.prepareViews(viewport, scissorRect);
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
	for_each(appMap.begin(), appMap.end(), [](auto element) {
		Log("xapp registered: " << element.first.c_str() << endl);
	});
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

// global instance:
static XApp *xappPtr = nullptr;

XApp& xapp() {
	if (xappPtr == nullptr) {
		xappPtr = new XApp();
	}
	return *xappPtr;
}

void xappDestroy() {
	delete xappPtr;
}
