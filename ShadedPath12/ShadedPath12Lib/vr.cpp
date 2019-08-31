#include "stdafx.h"

VR::VR()
{
#if defined(_SVR_)
	m_rmat4DevicePose = new Matrix4[vr::k_unMaxTrackedDeviceCount];
	m_rTrackedDevicePose = new vr::TrackedDevicePose_t[vr::k_unMaxTrackedDeviceCount];
#endif

	Log("VR class initialized" << endl);
}

void VR::init(Pipeline* pipeline, DXGlobal* dxglobal) {
	if (this->pipeline != nullptr) return; // TODO fix multiple calls to this init()
	this->pipeline = pipeline;
	this->dxGlobal = dxglobal;
	pipeline->setVRImplementation(this);  //	TODO reenable for this class to work!!!
#if defined(_SVR_)
	// Loading the SteamVR Runtime
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None)
	{
		m_pHMD = NULL;
		char buf[1024];
		sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		Log(L"svr_Initialize failed: " << vr::VR_GetVRInitErrorAsEnglishDescription(eError) << endl);
		Error(L"svr_Initialize failed");
	}
	if (!vr::VRCompositor())
	{
		Log(L"VRCompositor not available" << endl);
		Error(L"VRCompositor not available");
	}
	// set async mode - should enable multi thread queue access
	vr::VRCompositor()->SetExplicitTimingMode(vr::EVRCompositorTimingMode::VRCompositorTimingMode_Explicit_ApplicationPerformsPostPresentHandoff);
#endif
}

#if !defined(_OVR_)
XMFLOAT4X4 VR::ident;
#endif

#define MAX_BONES 64

VR::~VR() {
#if defined(_SVR_)
	delete[] m_rmat4DevicePose;
	delete[] m_rTrackedDevicePose;
#endif
	if (!pipeline->isHMD()) return;
#if defined(_SVR_)
	vr::VR_Shutdown();
#endif
#if defined(_OVR_)
	int count;
	ovr_GetTextureSwapChainLength(session, textureSwapChain, &count);
	for (int i = 0; i < count; ++i)
	{
		texResource[i]->Release();
		//texRtv[i]->Release();
	}
	ovr_DestroyTextureSwapChain(session, textureSwapChain);
	ovr_Destroy(session);
	ovr_Shutdown();
#endif
}

void VR::init()
{
#if defined(_OVR_)
	ovrInitParams initParams = { ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL, 0, 0 };
	ovrResult result = ovr_Initialize(&initParams);
	if (OVR_FAILURE(result)) {
		ovrErrorInfo errorInfo;
		ovr_GetLastErrorInfo(&errorInfo);
		Log(L"ovr_Initialize failed: " << errorInfo.ErrorString << endl);
		Error(L"ovr_Initialize failed");
		return;
	}
	Log("LibOVR initialized ok!" << endl);
	result = ovr_Create(&session, &luid);
	if (OVR_FAILURE(result))
	{
		ovr_Shutdown();
		Error(L"No Oculus Rift detected. Cannot run in OVR mode without Oculus Rift device.");
		return;
	}

	desc = ovr_GetHmdDesc(session);
	resolution = desc.Resolution;
	// Start the sensor which provides the Rift’s pose and motion.
	/*	result = ovr_ConfigureTracking(session, ovrTrackingCap_Orientation |
	ovrTrackingCap_MagYawCorrection |
	ovrTrackingCap_Position, 0);
	if (OVR_FAILURE(result)) Error(L"Could not enable Oculus Rift Tracking. Cannot run in OVR mode without Oculus Rift tracking.");
	*/
	// Setup VR components, filling out description
	eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, desc.DefaultEyeFov[0]);
	eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, desc.DefaultEyeFov[1]);

	//ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);
	//ovr_RecenterTrackingOrigin(session);
	nextTracking();

	// tracking setup complete, now init rendering:

	// Configure Stereo settings.
	Sizei recommenedTex0Size = ovr_GetFovTextureSize(session, ovrEye_Left, desc.DefaultEyeFov[0], 1.0f);
	Sizei recommenedTex1Size = ovr_GetFovTextureSize(session, ovrEye_Right, desc.DefaultEyeFov[1], 1.0f);
	buffersize_width = recommenedTex0Size.w + recommenedTex1Size.w;
	buffersize_height = max(recommenedTex0Size.h, recommenedTex1Size.h);
	result = ovr_RequestBoundaryVisible(session, ovrTrue);
	if (OVR_FAILURE(result)) {
		Log(L"Oculus Boundary system inactive.");
	}
	else {
		Log(L"Oculus Boundary system activated!!");
	}
	// Initialization call
	if (ovr_PlatformInitializeWindows("1197980730287980") != ovrPlatformInitialize_Success)
	{
		// Exit.  Initialization failed which means either the oculus service isn’t on the machine or they’ve hacked their DLL
		ovrErrorInfo errorInfo;
		ovr_GetLastErrorInfo(&errorInfo);
		Log(L"ovr_PlatformInitializeWindows failed: " << errorInfo.ErrorString << endl);
		Error(L"ovr_PlatformInitializeWindows failed");
	}
	ovr_Entitlement_GetIsViewerEntitled();
#endif
}

void VR::initD3D()
{
#if defined(_OVR_)
	Sizei bufferSize;
	bufferSize.w = buffersize_width;
	bufferSize.h = buffersize_height;


	// xapp->d3d11Device.Get() will not work, we need a real D3D11 device

	//for (int i = 0; i < xapp->FrameCount; i++) {
	//	ID3D12Resource *resource = xapp->renderTargets[i].Get();
	//	D3D12_RESOURCE_DESC rDesc = resource->GetDesc();
	//	D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
	//	ThrowIfFailed(xapp->d3d11On12Device->CreateWrappedResource(
	//		resource,
	//		&d3d11Flags,
	//		D3D12_RESOURCE_STATE_RENDER_TARGET,
	//		D3D12_RESOURCE_STATE_PRESENT,
	//		IID_PPV_ARGS(&xapp->wrappedBackBuffers[i])
	//		));
	//	//xapp->d3d11On12Device->AcquireWrappedResources(xapp->wrappedBackBuffers[i].GetAddressOf(), 1);
	//}

	ovrTextureSwapChainDesc dsDesc = {};
	dsDesc.Type = ovrTexture_2D;
	dsDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	dsDesc.ArraySize = 1;
	dsDesc.Width = bufferSize.w;
	dsDesc.Height = bufferSize.h;
	dsDesc.MipLevels = 1;
	dsDesc.SampleCount = 1;
	dsDesc.StaticImage = ovrFalse;
	dsDesc.MiscFlags = ovrTextureMisc_DX_Typeless;//ovrTextureMisc_None;
	dsDesc.BindFlags = ovrTextureBind_DX_RenderTarget;

	/*	D3D11_TEXTURE2D_DESC dsDesc;
	dsDesc.Width = bufferSize.w;
	dsDesc.Height = bufferSize.h;
	dsDesc.MipLevels = 1;
	dsDesc.ArraySize = 1;
	dsDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	dsDesc.SampleDesc.Count = 1;
	dsDesc.SampleDesc.Quality = 0;
	dsDesc.Usage = D3D11_USAGE_DEFAULT;
	dsDesc.CPUAccessFlags = 0;
	dsDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	dsDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	*/
	if (ovr_CreateTextureSwapChainDX(session, xapp->commandQueue.Get()/*xapp->reald3d11Device.Get()*/, &dsDesc, &textureSwapChain) == ovrSuccess)
	{
		int count = 0;
		ovr_GetTextureSwapChainLength(session, textureSwapChain, &count);
		texRtv.resize(count);
		texResource.resize(count);
		// Create descriptor heaps.
		UINT rtvDescriptorSize;
		{
			// Describe and create a render target view (RTV) descriptor heap.

			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = count;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(xapp->device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvVRHeap)));
			rtvVRHeap->SetName(L"rtVRHeap_xapp");

			rtvDescriptorSize = xapp->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}
		for (int i = 0; i < count; ++i)
		{
			ID3D11Texture2D* tex = nullptr;
			ovr_GetTextureSwapChainBufferDX(session, textureSwapChain, i, IID_PPV_ARGS(&texResource[i]));
			//xapp->reald3d11Device.Get()->CreateRenderTargetView(tex, nullptr, &texRtv[i]);

			D3D12_RENDER_TARGET_VIEW_DESC rtvd = {};
			rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			rtvd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvVRHeap->GetCPUDescriptorHandleForHeapStart(), i, rtvDescriptorSize);
			texRtv[i] = rtvHandle;
			xapp->device->CreateRenderTargetView(texResource[i], /*nullptr*/&rtvd, texRtv[i]);

			//ComPtr<IDXGIResource> dxgires;
			//tex->QueryInterface<IDXGIResource>(&dxgires);
			////Log("dxgires = " << dxgires.GetAddressOf() << endl);
			//HANDLE shHandle;
			//dxgires->GetSharedHandle(&shHandle);
			////Log("shared handle = " << shHandle << endl);
			//xapp->d3d11Device->OpenSharedResource(shHandle, IID_PPV_ARGS(&xapp->wrappedTextures[i]));
			//tex->Release();
		}
	}
	// Initialize our single full screen Fov layer.
	layer.Header.Type = ovrLayerType_EyeFov;
	layer.Header.Flags = 0;
	layer.ColorTexture[0] = textureSwapChain;
	layer.ColorTexture[1] = nullptr;//textureSwapChain;
	layer.Fov[0] = eyeRenderDesc[0].Fov;
	layer.Fov[1] = eyeRenderDesc[1].Fov;
	layer.Viewport[0] = Recti(0, 0, bufferSize.w / 2, bufferSize.h);
	layer.Viewport[1] = Recti(bufferSize.w / 2, 0, bufferSize.w / 2, bufferSize.h);
	// ld.RenderPose and ld.SensorSampleTime are updated later per frame.
#endif
#if defined(_DEBUG)
	// SetStablePowerState requires Win10 to be in developer mode:
	// start settings app, then search for 'for developers settings', 
	// then enable it under developer features, developer mode
	//xapp->device->SetStablePowerState(true);
#endif
}

void VR::initFrame()
{
}

void VR::startFrame()
{
#if defined(_SVR_)
	//Util::logThreadInfo(L"VRCompositor()->SubmitExplicitTimingData()");
	vr::VRCompositor()->SubmitExplicitTimingData();
#endif
	curEye = EyeLeft;
}

void VR::endFrame()
{
}

void VR::prepareEyes(VR_Eyes* eyes)
{
	auto config = pipeline->getPipelineConfig();
	auto width = config.backbufferWidth;
	auto height = config.backbufferHeight;
	D3D12_VIEWPORT viewport;
	viewport.MinDepth = 0.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissorRect;
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = static_cast<LONG>(width);
	scissorRect.bottom = static_cast<LONG>(height);

	if (!pipeline->isVR()) {
		eyes->viewports[0] = viewport;
		eyes->viewports[1] = viewport;
		eyes->scissorRects[0] = scissorRect;
		eyes->scissorRects[1] = scissorRect;
	}
	else {
		eyes->viewports[EyeLeft] = viewport;
		eyes->scissorRects[EyeLeft] = scissorRect;
		eyes->viewports[EyeRight] = viewport;
		eyes->scissorRects[EyeRight] = scissorRect;
		eyes->viewports[EyeLeft].Width /= 2;
		eyes->scissorRects[EyeLeft].right /= 2;
		//scissorRects[EyeLeft].right--;
		eyes->viewports[EyeRight].Width /= 2;
		eyes->viewports[EyeRight].TopLeftX = eyes->viewports[EyeLeft].Width;
		eyes->scissorRects[EyeRight].left = eyes->scissorRects[EyeLeft].right;
	}
}

void VR::prepareViews(D3D12_VIEWPORT& viewport, D3D12_RECT& scissorRect)
{
	//if (!enabled) {
	//	viewports[EyeLeft] = viewport;
	//	scissorRects[EyeLeft] = scissorRect;
	//	viewports[EyeRight] = viewport;
	//	scissorRects[EyeRight] = scissorRect;
	//}
	//else {
	//	//orig_viewport = viewport;
	//	//orig_scissorRect = scissorRect;
	//	viewports[EyeLeft] = viewport;
	//	scissorRects[EyeLeft] = scissorRect;
	//	viewports[EyeRight] = viewport;
	//	scissorRects[EyeRight] = scissorRect;
	//	viewports[EyeLeft].Width /= 2;
	//	scissorRects[EyeLeft].right /= 2;
	//	//scissorRects[EyeLeft].right--;
	//	viewports[EyeRight].Width /= 2;
	//	viewports[EyeRight].TopLeftX = viewports[EyeLeft].Width;
	//	scissorRects[EyeRight].left = scissorRects[EyeLeft].right;
	//}
}

void VR::prepareDraw()
{
	//cam_pos = xapp->camera.pos;
	//cam_look = xapp->camera.look;
	//cam_up = xapp->camera.up;
	firstEye = true;
	curEye = EyeLeft;
}

void VR::endDraw() {
	//xapp->camera.pos = cam_pos;
	//xapp->camera.look = cam_look;
	//xapp->camera.up = cam_up;
	//xapp->camera.worldViewProjection();
}

void VR_Eyes::adjustEyeMatrix(XMMATRIX& m, Camera* cam, int eyeNum, VR* vr)
{
	// get updated eye pos
	//vrMode->nextTracking();
	viewOVR[eyeNum] = vr->getOVRViewMatrixByIndex(eyeNum);
	projOVR[eyeNum] = vr->getOVRProjectionMatrixByIndex(eyeNum);
	adjustedEyePos[eyeNum] = vr->getOVRAdjustedEyePosByIndex(eyeNum);

	// camera update
	//cam->projectionTransform(this, eyeNum);
	m = cam->worldViewProjection(projOVR[eyeNum], viewOVR[eyeNum]);
	// for disabled oculus rendering just copy regular camera wvp:
	if (vr->texResource.size() == 0) {
		//m = xapp().camera.worldViewProjection(); TODO removed - not necessary
	}
}

void VR::adjustEyeMatrix(XMMATRIX& m, Camera* cam) {
	if (cam == nullptr) {
		//xapp->camera.projectionTransform();  TODO removed - not necessary
		//m = xapp->camera.worldViewProjection();  TODO removed - not necessary
	}
	else {
		cam->projectionTransform();
		m = cam->worldViewProjection();
	}
	//Camera c2 = xapp->camera;
	//if (curEye == EyeLeft) {
	//	c2.pos.x = cam_pos.x - 0.5f;
	//	c2.pos.y = cam_pos.y + 0.3f;
	//}
	//else {
	//	c2.pos.x = cam_pos.x + 0.5f;
	//	c2.pos.y = cam_pos.y + 0.3f;
	//}
	//m = c2.worldViewProjection();
	//c2.apply_pitch_yaw();
	//if (curEye == EyeLeft) {
	//	xapp->camera.pos.x = cam_pos.x - 0.5f;
	//	xapp->camera.pos.y = cam_pos.x + 0.3f;
	//} else {
	//	xapp->camera.pos.x = cam_pos.x + 0.5f;
	//	xapp->camera.pos.y = cam_pos.x + 0.3f;
	//}
	//m = xapp->camera.worldViewProjection();
	//xapp->camera.apply_pitch_yaw();
}

void VR::nextEye() {
	if (curEye == EyeLeft) curEye = EyeRight;
	else curEye = EyeLeft;
	firstEye = false;
}

bool VR::isFirstEye() {
	return firstEye;
}

#if defined(_OVR_)

unsigned int VR::pack(const uint8_t* blend_indices)
{
	int pack = 0;
	for (int i = 3; i >= 0; i--) {
		pack <<= 8;
		pack |= (blend_indices[i]) & 0xff;
	}
	return pack;
}

void Matrix4fToXM(XMFLOAT4X4& xm, Matrix4f& m) {
	xm._11 = m.M[0][0];
	xm._12 = m.M[0][1];
	xm._13 = m.M[0][2];
	xm._14 = m.M[0][3];
	xm._21 = m.M[1][0];
	xm._22 = m.M[1][1];
	xm._23 = m.M[1][2];
	xm._24 = m.M[1][3];
	xm._31 = m.M[2][0];
	xm._32 = m.M[2][1];
	xm._33 = m.M[2][2];
	xm._34 = m.M[2][3];
	xm._41 = m.M[3][0];
	xm._42 = m.M[3][1];
	xm._43 = m.M[3][2];
	xm._44 = m.M[3][3];
}

void XMFLOAT4x4ToFloatArray(XMFLOAT4X4& xm, float* m) {
	m[0] = xm._11;
	m[1] = xm._12;
	m[2] = xm._13;
	m[3] = xm._14;
	m[4] = xm._21;
	m[5] = xm._22;
	m[6] = xm._23;
	m[7] = xm._24;
	m[8] = xm._31;
	m[9] = xm._32;
	m[10] = xm._33;
	m[11] = xm._34;
	m[12] = xm._41;
	m[13] = xm._42;
	m[14] = xm._43;
	m[15] = xm._44;
}

void VR::nextTracking()
{
#if defined(_DEBUG)
	// make sure we are only caled once per frame:
	static vector<bool> called;
	if (xapp->getFramenum() < 50000) {
		size_t framenum = (size_t)xapp->getFramenum();
		assert(called.size() <= framenum);
		called.push_back(true);
		assert(called.size() == framenum + 1);
	}
#endif

	// Get both eye poses simultaneously, with IPD offset already included. 
	ovrVector3f useHmdToEyeViewOffset[2] = { eyeRenderDesc[0].HmdToEyeOffset, eyeRenderDesc[1].HmdToEyeOffset };
	//ovrPosef temp_EyeRenderPose[2];
	double displayMidpointSeconds = ovr_GetPredictedDisplayTime(session, 0);
	ovrTrackingState ts = ovr_GetTrackingState(session, displayMidpointSeconds, false);
	ovr_CalcEyePoses(ts.HeadPose.ThePose, useHmdToEyeViewOffset, layer.RenderPose);
	//Log("eyePoses: " << layer.RenderPose[0].Position.x << " " << layer.RenderPose[1].Position.x << endl);
	ovrResult result;
	ovrBoundaryTestResult btest;
	ovrBool visible;
	result = ovr_GetBoundaryVisible(session, &visible);
	if (0) {
		Log("visible = " << (visible == ovrTrue) << endl);

		result = ovr_TestBoundary(session, ovrTrackedDevice_HMD, ovrBoundary_Outer, &btest);
		if (OVR_SUCCESS(result)) {
			//Log("boundary success");
			if (result == ovrSuccess) Log("success" << endl);
			if (result == ovrSuccess_BoundaryInvalid) Log("success boundary invalid" << endl);
			if (result == ovrSuccess_DeviceUnavailable) Log("success device unavailable" << endl);
		}
	}
	layer.Fov[0] = eyeRenderDesc[0].Fov;
	layer.Fov[1] = eyeRenderDesc[1].Fov;

	avatarInfo.trackingState = &ts;
	updateAvatar();
	// Render the two undistorted eye views into their render buffers.  
	for (int eye = 0; eye < 2; eye++)
	{
		ovrPosef* useEyePose = &EyeRenderPose[eye];
		float* useYaw = &YawAtRender[eye];
		float Yaw = XM_PI;
		*useEyePose = layer.RenderPose[eye];
		*useYaw = Yaw;

		// Get view and projection matrices (note near Z to reduce eye strain)
		Matrix4f rollPitchYaw = Matrix4f::RotationY(Yaw);
		Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(useEyePose->Orientation);
		// fix finalRollPitchYaw for LH coordinate system:
		Matrix4f s = Matrix4f::Scaling(1.0f, -1.0f, -1.0f);  // 1 1 -1
		finalRollPitchYaw = s * finalRollPitchYaw * s;

		Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
		Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));//0 0 1
		Vector3f Posf;
		Posf.x = xapp->camera.pos.x;
		Posf.y = xapp->camera.pos.y;
		Posf.z = xapp->camera.pos.z;
		Vector3f diff = rollPitchYaw.Transform(useEyePose->Position);
		//diff /= 10.0f;
		//diff.x = 0.0f;
		//diff.y = 0.0f;
		//diff.z = 0.0f;
		Vector3f shiftedEyePos;
		shiftedEyePos.x = Posf.x - diff.x;
		shiftedEyePos.y = Posf.y + diff.y;
		shiftedEyePos.z = Posf.z + diff.z;
		xapp->camera.look.x = finalForward.x;
		xapp->camera.look.y = finalForward.y;
		xapp->camera.look.z = finalForward.z;

		float nearz = xapp->camera.nearZ;
		float farz = xapp->camera.farZ;
		Matrix4f view = Matrix4f::LookAtLH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
		Matrix4f projO = ovrMatrix4f_Projection(eyeRenderDesc[eye].Fov, nearz, farz, ovrProjection_LeftHanded);
		Matrix4fToXM(this->viewOVR[eye], view.Transposed());
		Matrix4fToXM(this->projOVR[eye], projO.Transposed());
		this->adjustedEyePos[eye] = XMFLOAT3(shiftedEyePos.x, shiftedEyePos.y, shiftedEyePos.z);
	}
}

void VR::submitFrame()
{
	int frameIndex = xapp->getCurrentBackBufferIndex();

	// Increment to use next texture, just before writing
	int currentIndex;
	ovr_GetTextureSwapChainCurrentIndex(session, textureSwapChain, &currentIndex);
	//assert(currentIndex == frameIndex);
	xapp->lastPresentedFrame = frameIndex;
	//xapp->d3d11On12Device->AcquireWrappedResources(xapp->wrappedBackBuffers[frameIndex].GetAddressOf(), 1);
	//xapp->d3d11DeviceContext->CopyResource(xapp->wrappedTextures[currentIndex].Get(), xapp->wrappedBackBuffers[frameIndex].Get());
	//xapp->d3d11On12Device->ReleaseWrappedResources(xapp->wrappedBackBuffers[frameIndex].GetAddressOf(), 1);
	//xapp->d3d11DeviceContext->Flush();
	//xapp->device->
	//ovr_CommitTextureSwapChain(session, textureSwapChain);
	//ovr_CommitTextureSwapChain(session, textureSwapChain);
	/*	pTextureSet->CurrentIndex = (pTextureSet->CurrentIndex + 1) % pTextureSet->TextureCount;
	*/
	// Submit frame with one layer we have.
	ovrLayerHeader* layers = &layer.Header;
	ovrResult       result = ovrSuccess; //ovr_SubmitFrame(session, 0, nullptr, &layers, 1);
	bool isVisible = (result == ovrSuccess);
}


void VR::handleOVRMessages()
{
	handleAvatarMessages();
	ovrMessage* message = ovr_PopMessage();
	if (ovr_Message_IsError(message) != 0) {
		Log("OVR message error" << endl);
		auto errHandle = ovr_Message_GetError(message);
		const char* desc = ovr_Error_GetDisplayableMessage(errHandle);
		const char* tech = ovr_Error_GetMessage(errHandle);
		Log("error desc: " << desc << endl);
		Log("error tech: " << tech << endl);
	}
	if (!message) {
		return; // no new messages.
	}
	int messageType = ovr_Message_GetType(message);
	//Log("OVR message received: " << std::hex << messageType << " " << ovrMessageType_ToString((ovrMessageType)messageType) << endl);
	if (messageType == ovrMessage_Entitlement_GetIsViewerEntitled) {
		Log("Entitlement message " << endl);
		if (!ovr_Message_IsError(message))
		{
			// User is entitled.  Continue with normal game behaviour
			Log("your user is allowe to use this oculus app" << endl);
		}
		else
		{
			// User is NOT entitled.  Exit
			Log(L"WARNING: Your User is not allowed to use this Oculus application. You may continue but will not be able to use hands/controllers. Please consider registering register your app at Oculus.");
			Log("Attempting to run without Platform SDK." << endl);
			auto userId = ovr_GetLoggedInUserID();
			ovrAvatar_RequestAvatarSpecification(userId);
		}
	}
	else if (messageType == ovrMessage_User_GetLoggedInUser) {
		if (ovr_Message_IsError(message) != 0) {
			Log("message error");
		}
		else {
			auto user = ovr_Message_GetUser(message);
			auto userId = ovr_User_GetID(user);
			auto userName = ovr_User_GetOculusID(user);
			Log("current user: " << userName << " ID == " << userId << endl);
			ovrAvatar_RequestAvatarSpecification(userId);
			auto anotherUserId = ovr_GetLoggedInUserID();
			//Log("current user: " << userName << " ID == " << anotherUserId << endl);
		}
	}
	else {
		//  Handle other Platform SDK messages here.
	}
	ovr_FreeMessage(message);
}

void VR::handleAvatarMessages()
{
	static int loadingAssets = 0;
	//static const ovrAvatarMessage_AvatarSpecification *spec;
	static uint64_t userId;
	ovrAvatarMessage* message = ovrAvatarMessage_Pop();
	if (!message) {
		return; // no new messages.
	}
	const ovrAvatarMessage_AvatarSpecification* spec;
	auto messageType = ovrAvatarMessage_GetType(message);
	if (messageType == ovrAvatarMessageType_AvatarSpecification) {
		spec = ovrAvatarMessage_GetAvatarSpecification(message);
		userId = spec->oculusUserID;
		//Log("avatar spec: " << spec->avatarSpec << " ID == " << spec->oculusUserID << endl);
		// see mirror.cpp l 838
		// Create the avatar instance
		avatar = ovrAvatar_Create(spec->avatarSpec, ovrAvatarCapability_All);
		auto avatarControllerComponent = ovrAvatarPose_GetLeftHandComponent(avatar);  //ControllerComponent(avatar);
		auto avatarComponent = avatarControllerComponent->renderComponent;
		for (unsigned int i = 0; i < avatarComponent->renderPartCount; i++) {
			auto part = avatarComponent->renderParts[i];
			auto type = ovrAvatarRenderPart_GetType(part);
			Log("leftcontroller render part type: " << type << endl);
			if (type == ovrAvatarRenderPartType_SkinnedMeshRenderPBS) {
				auto skinnedMeshRenderPBS = ovrAvatarRenderPart_GetSkinnedMeshRenderPBS(part);
				if (skinnedMeshRenderPBS)
					Log(" left controller mesh asset id: " << std::hex << skinnedMeshRenderPBS->meshAssetID << endl);
			}
			if (type == ovrAvatarRenderPartType_SkinnedMeshRender) {
				auto skinnedMeshRender = ovrAvatarRenderPart_GetSkinnedMeshRender(part);
				if (skinnedMeshRender)
					Log(" left controller mesh asset id: " << std::hex << skinnedMeshRender->meshAssetID << endl);
			}
		}

		if (loadAvatarAssetsFromOculus) {
			// Trigger load operations for all of the assets referenced by the avatar
			uint32_t refCount = ovrAvatar_GetReferencedAssetCount(avatar);
			for (uint32_t i = 0; i < refCount; ++i)
			{
				ovrAvatarAssetID id = ovrAvatar_GetReferencedAsset(avatar, i);
				ovrAvatarAsset_BeginLoading(id);
				++loadingAssets;
			}
			//Log("Loading " << loadingAssets << " assets..." << endl);
		}
		else {
			gatherAvatarInfo(avatarInfo, avatar);
		}
	}
	else if (messageType == ovrAvatarMessageType_AssetLoaded) {
		auto assetmsg = ovrAvatarMessage_GetAssetLoaded(message);
		// Determine the type of the asset that got loaded
		ovrAvatarAssetType assetType = ovrAvatarAsset_GetType(assetmsg->asset);
		void* data = nullptr;

		// Call the appropriate loader function
		switch (assetType)
		{
		case ovrAvatarAssetType_Mesh:
		{
			const ovrAvatarMeshAssetData* assetdata = ovrAvatarAsset_GetMeshData(assetmsg->asset);
			//Log("vertices for " << assetmsg->assetID << " : " << assetdata->vertexCount << endl);
			writeOVRMesh(userId, assetmsg, assetdata);
		}
		break;
		case ovrAvatarAssetType_Texture:
		{
			const ovrAvatarTextureAssetData* assetdata = ovrAvatarAsset_GetTextureData(assetmsg->asset);
			//Log("vertices for " << assetmsg->assetID << " : " << assetdata->vertexCount << endl);
			writeOVRTexture(userId, assetmsg, assetdata);
		}
		//data = _loadTexture(ovrAvatarAsset_GetTextureData(assetmsg->asset));
		break;
		}

		// Store the data that we loaded for the asset in the asset map
		//_assetMap[message->assetID] = data;
		--loadingAssets;
		//Log("Loading " << loadingAssets << " assets..." << endl);
		if (loadingAssets == 0) {
			// all is loaded: gather Avatar info
			gatherAvatarInfo(avatarInfo, avatar);
		}
	}
	else {
		//  Handle other Platform SDK messages here.
	}
	ovrAvatarMessage_Free(message);
}

void VR::computeWorldPose(const ovrAvatarSkinnedMeshPose& localPose, XMMATRIX worldPose[])
{
	for (int i = 0; i < localPose.jointCount; ++i)
	{
		XMFLOAT4X4 local4;
		calculateBindMatrix(&localPose.jointTransform[i], &local4);
		XMMATRIX local = XMLoadFloat4x4(&local4);
		int parentIndex = localPose.jointParents[i];
		if (parentIndex < 0)
		{
			worldPose[i] = local;
		}
		else
		{
			worldPose[i] = local * worldPose[parentIndex]/* * local*/;
			//if (i == 4 && debugComponent) // left hand
			//{
			//	//Log("C");
			//}
		}
	}
}

void VR::calculateBindMatrix(const ovrAvatarTransform* t, XMFLOAT4X4* bind4)
{
	XMVECTOR zeroRotationOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR scale, rotationQuaternion, translation;
	if (t->scale.x != t->scale.y || t->scale.y != t->scale.z) {
		//Log("WARNING: non-uniform scale encountered!!" << endl);
	}
	XMFLOAT3 scaleF = XMFLOAT3(t->scale.x, t->scale.y, t->scale.z);
	scale = XMLoadFloat3(&scaleF);
	rotationQuaternion = XMVectorSet(t->orientation.x, t->orientation.y, t->orientation.z, t->orientation.w);
	translation = XMVectorSet(t->position.x, t->position.y, t->position.z, 0.0f);
	rotationQuaternion = XMQuaternionNormalize(rotationQuaternion);
	XMMATRIX bind = XMMatrixAffineTransformation(scale, zeroRotationOrigin, rotationQuaternion, translation);
	XMMATRIX fixScale = XMMatrixScaling(1.0f, 1.0f, -1.0f);
	bind = fixScale * bind * fixScale;
	XMStoreFloat4x4(bind4, bind);
}

void VR::writeOVRMesh(const uint64_t userId, const ovrAvatarMessage_AssetLoaded* assetmsg, const ovrAvatarMeshAssetData* assetdata)
{
	//Log("write avatar mesh (user id / mesh id / vertex count): " << std::hex << userId << " / " << assetmsg->assetID << " / " << assetdata->vertexCount << endl);
	wstring filename = getMeshFileName(assetmsg->assetID);
	wstring binFile = xapp->findFileForCreation(filename, XApp::MESH);
	ofstream bfile(binFile, ios::out | ios::trunc | ios::binary);  // create and delete old content
	assert(bfile);
	int mode = 0; // no bone data
				  // change to bone mode if joints are found:
	if (assetdata->skinnedBindPose.jointCount > 0) mode = 1;
	bfile.write((char*)& mode, 4);
	if (mode == 1) {
		int numAniClips = 1;  // only one fake clip for ovr mode
		bfile.write((char*)& numAniClips, 4);
		//aniClips = new AnimationClip[numAniClips];
		for (int i = 0; i < numAniClips; i++) {
			string clip_name("non_keyframe");
			int numAnimationNameLength = (int)clip_name.length();
			bfile.write((char*)& numAnimationNameLength, 4);
			bfile.write(clip_name.c_str(), numAnimationNameLength);
			//int zero = 0;
			//bfile.write((char*)&zero, 1);
			int numJoints = assetdata->skinnedBindPose.jointCount;
			//debugComponent = std::string(assetdata->skinnedBindPose.jointNames[0]).compare("hands:l_hand_world") ? false : true;
			bfile.write((char*)& numJoints, 4);
			XMMATRIX worldPose[MAX_BONES];  // actually the bind pose, used to calc inverse bind pose matrices
			computeWorldPose(assetdata->skinnedBindPose, &worldPose[0]);
			for (int j = 0; j < numJoints; j++) {
				// parent:
				int parentId = assetdata->skinnedBindPose.jointParents[j];
				bfile.write((char*)& parentId, 4);
				// calculate inverse bind matrices:
				// inverses are stored exactly like in OpenGL
				XMMATRIX bind = worldPose[j];
				XMVECTOR determinant = XMMatrixDeterminant(bind);
				XMMATRIX inverse = XMMatrixInverse(&determinant, bind);
				XMFLOAT4X4 inv;
				XMStoreFloat4x4(&inv, inverse);
				float f[16];
				XMFLOAT4x4ToFloatArray(inv, f);
				for (int n = 0; n < 16; n++) {
					bfile.write((char*)& f[n], 4);
				}
				// zero keyframes:
				int keyframes = 0;
				bfile.write((char*)& keyframes, 4);
			}
		}
	}
	// vertices
	ovrAvatarMeshVertex_* vbuf = (ovrAvatarMeshVertex_*)assetdata->vertexBuffer;
	uint16_t* ibuf = (uint16_t*)assetdata->indexBuffer;
	int numVerts = assetdata->vertexCount;
	if (true) {
		// fix for left handed system
		// flip z coordinate on all vertices, change triangle ordering
		// pose matrices are converted during rendering/update, see VR.calculateBindMatrix()

		for (size_t i = 0; i < assetdata->vertexCount; i++) {
			ovrAvatarMeshVertex_ v = vbuf[i];
			v.z *= -1.0f;
			v.nz *= -1.0f;
			v.tz *= -1.0f;
			vbuf[i] = v;
		}
		for (size_t i = 0; i < assetdata->indexCount; i += 3) {
			auto save = ibuf[i + 1];
			ibuf[i + 1] = ibuf[i + 2];
			ibuf[i + 2] = save;
		}
	}
	numVerts *= 3;
	bfile.write((char*)& numVerts, 4);
	for (size_t i = 0; i < assetdata->vertexCount; i++) {
		ovrAvatarMeshVertex_ v = vbuf[i];
		//assert(v.u <= 1.0f);
		//assert(v.u >= 0.0f);
		//assert(v.v <= 1.0f);
		//assert(v.v >= 0.0f);
		bfile.write((char*)& v.x, 4);
		bfile.write((char*)& v.y, 4);
		bfile.write((char*)& v.z, 4);
	}

	// texture coords
	//int numTex = (numVerts * 2) / 3;
	for (size_t i = 0; i < assetdata->vertexCount; i++) {
		ovrAvatarMeshVertex_ v = vbuf[i];
		//v.u = 1.0f - v.u;
		//v.v = 1.0f - v.v;
		bfile.write((char*)& v.u, 4);
		bfile.write((char*)& v.v, 4);
	}

	// normals
	for (size_t i = 0; i < assetdata->vertexCount; i++) {
		ovrAvatarMeshVertex_ v = vbuf[i];
		bfile.write((char*)& v.nx, 4);
		bfile.write((char*)& v.ny, 4);
		bfile.write((char*)& v.nz, 4);
	}

	if (mode == 1) {
		// we have skinned animation weights
		// for all vertices we have one int packed with the indices of the 4 bones influencing it
		for (size_t i = 0; i < assetdata->vertexCount; i++) {
			ovrAvatarMeshVertex_ v = vbuf[i];
			unsigned int packed = pack(&v.blendIndices[0]);
			bfile.write((char*)& packed, 4);
		}
		// for each bonePack we have the 4 weights:
		for (size_t i = 0; i < assetdata->vertexCount; i++) {
			ovrAvatarMeshVertex_ v = vbuf[i];
			unsigned int packed = pack(&v.blendIndices[0]);
			bfile.write((char*)& v.blendWeights[0], 4);
			bfile.write((char*)& v.blendWeights[1], 4);
			bfile.write((char*)& v.blendWeights[2], 4);
			bfile.write((char*)& v.blendWeights[3], 4);
		}
	}
	// vertices index buffer
	int numIndex = assetdata->indexCount;
	bfile.write((char*)& numIndex, 4);
	//const uint16_t* ibuf = assetdata->indexBuffer;
	for (size_t i = 0; i < assetdata->indexCount; i++) {
		uint16_t n = ibuf[i];
		int nl = n;
		bfile.write((char*)& nl, 4);
	}

	// animations
	int numAnimationNameLength = 0;
	bfile.write((char*)& numAnimationNameLength, 4);
	if (numAnimationNameLength != 0) {
		// TODO
	}

	bfile.close();
	Mesh mesh;
	//meshes[id] = mesh;
	//loader.loadBinaryAsset(binFile, &meshes[id], scale);
	//meshes[id].createVertexAndIndexBuffer(this->objectEffect);
}

#include "dds.h"
void VR::writeOVRTexture(const uint64_t userId, const ovrAvatarMessage_AssetLoaded* assetmsg, const ovrAvatarTextureAssetData* data)
{
	// Load the image data
	wstring filename = getTextureFileName(assetmsg->assetID);
	wstring binFile = xapp->findFileForCreation(filename, XApp::TEXTURE);
	ofstream bfile(binFile, ios::out | ios::trunc | ios::binary);  // create and delete old content
	assert(bfile);
	uint32_t dwMagicNumber = DDS_MAGIC;
	bfile.write((const char*)& dwMagicNumber, 4);
	DDS_HEADER header = { 0 };
	bool ok = false;

	//bfile.write((char*)&mode, 4);
	switch (data->format)
	{

		// Handle uncompressed image data
	case ovrAvatarTextureFormat_RGB24:
		//Log("format RGB24 " << data->sizeX << " * " << data->sizeY << " (" << data->textureDataSize << " bytes)" << endl);
		header.size = 0x7c;
		header.flags = 0x2100f;
		header.height = data->sizeY;
		header.width = data->sizeX;
		//header.pitchOrLinearSize = max(1, ((header.width + 3) / 4)) * blockSize;//0x40000;
		header.pitchOrLinearSize = header.width * 4;
		header.depth = 1;
		header.mipMapCount = data->mipCount;
		header.caps = 0x401008;

		header.ddspf.size = 0x20;
		header.ddspf.flags = 0x41;
		header.ddspf.fourCC = 0x0;
		header.ddspf.RGBBitCount = 0x20;
		header.ddspf.RBitMask = 0x00ff0000;
		header.ddspf.GBitMask = 0x0000ff00;
		header.ddspf.BBitMask = 0x000000ff;
		header.ddspf.ABitMask = 0xff000000;

		bfile.write((const char*)& header, sizeof(header));
		ok = true;
		for (uint32_t level = 0, offset = 0, width = data->sizeX, height = data->sizeY; level < data->mipCount; ++level)
		{
			//glTexImage2D(GL_TEXTURE_2D, level, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data->textureData + offset);
			//convert to rgb32 while saving:
			const unsigned char alpha = 0xff;
			for (uint32_t rgb_index = 0; rgb_index < width * height * 3; rgb_index += 3) {
				const char* mem = ((const char*)data->textureData) + offset + rgb_index;
				bfile.write(mem, 3);
				bfile.write((const char*)& alpha, 1);
			}
			offset += width * height * 3;
			width /= 2;
			height /= 2;
		}
		break;

		// Handle compressed image data
	case ovrAvatarTextureFormat_DXT1:
	case ovrAvatarTextureFormat_DXT5:
		//GLenum glFormat;
		int blockSize;
		if (data->format == ovrAvatarTextureFormat_DXT1)
		{
			blockSize = 8;
			//Log("format DXT1 " << data->sizeX << " * " << data->sizeY << " (" << data->textureDataSize << " bytes)" << endl);
			header.ddspf.fourCC = 0x31545844;
			//glFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		}
		else
		{
			blockSize = 16;
			//Log("format DXT5 " << std::hex << data->sizeX << " * " << data->sizeY << " (" << data->textureDataSize << " bytes)" << endl);
			header.ddspf.fourCC = 0x35545844;
		}

		header.size = 0x7c;
		header.flags = 0xa1007;
		header.height = data->sizeY;
		header.width = data->sizeX;
		header.pitchOrLinearSize = blockSize * max(1, ((header.width + 3) / 4)) * max(1, ((header.height + 3) / 4));
		header.depth = 1;
		header.mipMapCount = data->mipCount;
		header.caps = 0x401008;

		header.ddspf.size = 0x20;
		header.ddspf.flags = 0x04;

		bfile.write((const char*)& header, sizeof(header));
		ok = true;
		//glFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		//Log(" sizes: ");
		int total = 0;
		for (uint32_t level = 0, offset = 0, width = data->sizeX, height = data->sizeY; level < data->mipCount; ++level)
		{
			//int levelSize = blockSize * (width / 4) * (height / 4);
			int levelSize = blockSize * max(1, ((width + 3) / 4)) * max(1, ((height + 3) / 4));
			//Log(std::hex << levelSize << " ");
			total += levelSize;
			const char* mem = ((const char*)data->textureData) + offset;
			bfile.write(mem, levelSize);
			offset += levelSize;
			width /= 2;
			height /= 2;
		}
		//Log(endl << std::hex << "total: " << total << endl);
		break;
	}
	bfile.close();
	if (!ok) {
		ofstream bfile(binFile, ios::out | ios::trunc | ios::binary);  // create and delete old content
		bfile.close();
	}
}

void VR::loadAvatarFromOculus(bool reloadAssets)
{
	this->loadAvatarAssetsFromOculus = reloadAssets;
	ovrAvatar_Initialize("1197980730287980");
	// get current user:
	ovr_User_GetLoggedInUser();
}

void VR::loadAvatarDefault()
{
	loadAvatarFromOculus(false);
}

void VR::gatherAvatarComponentInfo(AvatarPartInfo& avatarPartInfo, const ovrAvatarHandComponent* component)
{
	auto avatarRenderComponent = component->renderComponent;
	//Log("controller has " << avatarRenderComponent->renderPartCount << " render parts" << endl);
	assert(1 == avatarRenderComponent->renderPartCount);
	for (unsigned int i = 0; i < avatarRenderComponent->renderPartCount; i++) {
		auto part = avatarRenderComponent->renderParts[i];
		auto type = ovrAvatarRenderPart_GetType(part);
		//Log("leftcontroller render part type: " << type << endl);
		if (type == ovrAvatarRenderPartType_SkinnedMeshRender) {
			auto skinnedMeshRender = ovrAvatarRenderPart_GetSkinnedMeshRender(part);
			if (skinnedMeshRender) {
				avatarPartInfo.textureFileName = L"hand_color.dds";//getTextureFileName(skinnedMeshRenderPBS->albedoTextureAssetID);
				avatarPartInfo.meshFileName = getMeshFileName(skinnedMeshRender->meshAssetID);
				avatarPartInfo.textureId = "white";//getTextureId(skinnedMeshRenderPBS->albedoTextureAssetID);
				avatarPartInfo.meshId = getMeshId(skinnedMeshRender->meshAssetID);
				avatarPartInfo.ovrMeshId = skinnedMeshRender->meshAssetID;
				avatarPartInfo.renderPart = skinnedMeshRender;
			}
		}
	}
}

void VR::gatherAvatarComponentInfo(AvatarPartInfo& avatarPartInfo, const ovrAvatarControllerComponent* component)
{
	auto avatarRenderComponent = component->renderComponent;
	//Log("controller has " << avatarRenderComponent->renderPartCount << " render parts" << endl);
	assert(1 == avatarRenderComponent->renderPartCount);
	for (unsigned int i = 0; i < avatarRenderComponent->renderPartCount; i++) {
		auto part = avatarRenderComponent->renderParts[i];
		auto type = ovrAvatarRenderPart_GetType(part);
		//Log("leftcontroller render part type: " << type << endl);
		if (type == ovrAvatarRenderPartType_SkinnedMeshRenderPBS) {
			auto skinnedMeshRenderPBS = ovrAvatarRenderPart_GetSkinnedMeshRenderPBS(part);
			if (skinnedMeshRenderPBS) {
				//Log("skinnedMeshRenderPBS left controller mesh asset id: " << std::hex << skinnedMeshRenderPBS->meshAssetID << endl);
				//Log("surface texture: " << std::hex << skinnedMeshRenderPBS->surfaceTextureAssetID << endl);
				//Log("albedo texture: " << std::hex << skinnedMeshRenderPBS->albedoTextureAssetID << endl);
				avatarPartInfo.textureFileName = getTextureFileName(skinnedMeshRenderPBS->albedoTextureAssetID);
				avatarPartInfo.meshFileName = getMeshFileName(skinnedMeshRenderPBS->meshAssetID);
				avatarPartInfo.textureId = getTextureId(skinnedMeshRenderPBS->albedoTextureAssetID);
				avatarPartInfo.meshId = getMeshId(skinnedMeshRenderPBS->meshAssetID);
				avatarPartInfo.ovrMeshId = skinnedMeshRenderPBS->meshAssetID;
				avatarPartInfo.renderPartPBS = skinnedMeshRenderPBS;
			}
		}
	}
}

void VR::gatherAvatarInfo(AvatarInfo& avatarInfo, ovrAvatar* avatar)
{
	// left controller:
	auto avatarControllerComponent = ovrAvatarPose_GetLeftControllerComponent(avatar);
	gatherAvatarComponentInfo(avatarInfo.controllerLeft, avatarControllerComponent);
	// right controller:
	avatarControllerComponent = ovrAvatarPose_GetRightControllerComponent(avatar);
	gatherAvatarComponentInfo(avatarInfo.controllerRight, avatarControllerComponent);
	// left hand:
	auto avatarHandComponent = ovrAvatarPose_GetLeftHandComponent(avatar);
	gatherAvatarComponentInfo(avatarInfo.handLeft, avatarHandComponent);
	// right hand:
	avatarHandComponent = ovrAvatarPose_GetRightHandComponent(avatar);
	gatherAvatarComponentInfo(avatarInfo.handRight, avatarHandComponent);

	// prepare assets:
	TextureInfo* ti = xapp->textureStore.getTexture(avatarInfo.controllerLeft.textureId);
	if (ti->id.length() == 0) {
		xapp->textureStore.loadTexture(avatarInfo.controllerLeft.textureFileName, avatarInfo.controllerLeft.textureId);
		ti = xapp->textureStore.getTexture(avatarInfo.controllerLeft.textureId);
		assert(ti);
	}
	xapp->objectStore.loadObject(avatarInfo.controllerLeft.meshFileName, avatarInfo.controllerLeft.meshId, 1.0f);
	xapp->objectStore.addObject(avatarInfo.controllerLeft.o, avatarInfo.controllerLeft.meshId, XMFLOAT3(0.0f, -0.4f, -0.2f), ti);

	ti = xapp->textureStore.getTexture(avatarInfo.controllerRight.textureId);
	if (ti->id.length() == 0) {
		xapp->textureStore.loadTexture(avatarInfo.controllerRight.textureFileName, avatarInfo.controllerRight.textureId);
		ti = xapp->textureStore.getTexture(avatarInfo.controllerRight.textureId);
		assert(ti);
	}
	xapp->objectStore.loadObject(avatarInfo.controllerRight.meshFileName, avatarInfo.controllerRight.meshId, 1.0f);
	xapp->objectStore.addObject(avatarInfo.controllerRight.o, avatarInfo.controllerRight.meshId, XMFLOAT3(0.4f, -0.4f, -0.2f), ti);


	ti = xapp->textureStore.getTexture(avatarInfo.handLeft.textureId);
	if (ti->id.length() == 0) {
		xapp->textureStore.loadTexture(avatarInfo.handLeft.textureFileName, avatarInfo.handLeft.textureId);
		ti = xapp->textureStore.getTexture(avatarInfo.handLeft.textureId);
		assert(ti);
	}
	xapp->objectStore.loadObject(avatarInfo.handLeft.meshFileName, avatarInfo.handLeft.meshId, 1.0f);
	xapp->objectStore.addObject(avatarInfo.handLeft.o, avatarInfo.handLeft.meshId, XMFLOAT3(0.0f, -0.4f, -0.2f), ti);

	ti = xapp->textureStore.getTexture(avatarInfo.handRight.textureId);
	if (ti->id.length() == 0) {
		xapp->textureStore.loadTexture(avatarInfo.handRight.textureFileName, avatarInfo.handRight.textureId);
		ti = xapp->textureStore.getTexture(avatarInfo.handRight.textureId);
		assert(ti);
	}
	xapp->objectStore.loadObject(avatarInfo.handRight.meshFileName, avatarInfo.handRight.meshId, 1.0f);
	xapp->objectStore.addObject(avatarInfo.handRight.o, avatarInfo.handRight.meshId, XMFLOAT3(0.4f, -0.4f, -0.2f), ti);

	avatarInfo.controllerLeft.o.material.ambient = XMFLOAT4(1, 1, 1, 1);
	// controller, shiny:
	//avatarInfo.controllerLeft.o.material.specExp = 20.0f;
	//avatarInfo.controllerLeft.o.material.specIntensity = 700.0f;
	// hands:
	avatarInfo.controllerLeft.o.material.specExp = 10.0f;
	avatarInfo.controllerLeft.o.material.specIntensity = 70.0f;
	avatarInfo.controllerLeft.o.setAction("non_keyframe");

	avatarInfo.controllerRight.o.material.ambient = XMFLOAT4(1, 1, 1, 1);
	// hands:
	avatarInfo.controllerRight.o.material.specExp = 10.0f;
	avatarInfo.controllerRight.o.material.specIntensity = 70.0f;
	avatarInfo.controllerRight.o.setAction("non_keyframe");

	avatarInfo.handLeft.o.material.ambient = XMFLOAT4(1, 1, 1, 1);
	// hands:
	avatarInfo.handLeft.o.material.specExp = 10.0f;
	avatarInfo.handLeft.o.material.specIntensity = 70.0f;
	avatarInfo.handLeft.o.setAction("non_keyframe");

	avatarInfo.handRight.o.material.ambient = XMFLOAT4(1, 1, 1, 1);
	// hands:
	avatarInfo.handRight.o.material.specExp = 10.0f;
	avatarInfo.handRight.o.material.specIntensity = 70.0f;
	avatarInfo.handRight.o.setAction("non_keyframe");
	//avatarInfo.handRight.o.drawNormals = true;
	avatarInfo.readyToRender = true;
}


CD3DX12_CPU_DESCRIPTOR_HANDLE VR::getRTVHandle(int frameIndex) {
	UINT rtvDescriptorSize;
	rtvDescriptorSize = xapp->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvVRHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);
	return rtvHandle;
};

XMFLOAT4X4 VR::getOVRViewMatrix() {
	return viewOVR[curEye];
};
XMFLOAT4X4 VR::getOVRViewMatrixByIndex(int eyeNum) {
	return viewOVR[eyeNum];
};


// get projection matrix for current eye
XMFLOAT4X4 VR::getOVRProjectionMatrix() {
	return projOVR[curEye];
};

// get projection matrix for current eye
XMFLOAT4X4 VR::getOVRProjectionMatrixByIndex(int eyeNum) {
	return projOVR[eyeNum];
};

XMFLOAT3 VR::getOVRAdjustedEyePosByIndex(int eyeNum) {
	return adjustedEyePos[eyeNum];
};


// drawing
static void _ovrAvatarHandInputStateFromOvr(const ovrAvatarTransform& transform, const ovrInputState& inputState, ovrHandType hand, ovrAvatarHandInputState* state)
{
	state->transform = transform;
	state->buttonMask = 0;
	state->touchMask = 0;
	state->joystickX = inputState.Thumbstick[hand].x;
	state->joystickY = inputState.Thumbstick[hand].y;
	state->indexTrigger = inputState.IndexTrigger[hand];
	state->handTrigger = inputState.HandTrigger[hand];
	state->isActive = false;
	if (hand == ovrHand_Left)
	{
		if (inputState.Buttons & ovrButton_X) state->buttonMask |= ovrAvatarButton_One;
		if (inputState.Buttons & ovrButton_Y) state->buttonMask |= ovrAvatarButton_Two;
		if (inputState.Buttons & ovrButton_Enter) state->buttonMask |= ovrAvatarButton_Three;
		if (inputState.Buttons & ovrButton_LThumb) state->buttonMask |= ovrAvatarButton_Joystick;
		if (inputState.Touches & ovrTouch_X) state->touchMask |= ovrAvatarTouch_One;
		if (inputState.Touches & ovrTouch_Y) state->touchMask |= ovrAvatarTouch_Two;
		if (inputState.Touches & ovrTouch_LThumb) state->touchMask |= ovrAvatarTouch_Joystick;
		if (inputState.Touches & ovrTouch_LThumbRest) state->touchMask |= ovrAvatarTouch_ThumbRest;
		if (inputState.Touches & ovrTouch_LIndexTrigger) state->touchMask |= ovrAvatarTouch_Index;
		if (inputState.Touches & ovrTouch_LIndexPointing) state->touchMask |= ovrAvatarTouch_Pointing;
		if (inputState.Touches & ovrTouch_LThumbUp) state->touchMask |= ovrAvatarTouch_ThumbUp;
		state->isActive = (inputState.ControllerType & ovrControllerType_LTouch) != 0;
	}
	else if (hand == ovrHand_Right)
	{
		if (inputState.Buttons & ovrButton_A) state->buttonMask |= ovrAvatarButton_One;
		if (inputState.Buttons & ovrButton_B) state->buttonMask |= ovrAvatarButton_Two;
		if (inputState.Buttons & ovrButton_Home) state->buttonMask |= ovrAvatarButton_Three;
		if (inputState.Buttons & ovrButton_RThumb) state->buttonMask |= ovrAvatarButton_Joystick;
		if (inputState.Touches & ovrTouch_A) state->touchMask |= ovrAvatarTouch_One;
		if (inputState.Touches & ovrTouch_B) state->touchMask |= ovrAvatarTouch_Two;
		if (inputState.Touches & ovrTouch_RThumb) state->touchMask |= ovrAvatarTouch_Joystick;
		if (inputState.Touches & ovrTouch_RThumbRest) state->touchMask |= ovrAvatarTouch_ThumbRest;
		if (inputState.Touches & ovrTouch_RIndexTrigger) state->touchMask |= ovrAvatarTouch_Index;
		if (inputState.Touches & ovrTouch_RIndexPointing) state->touchMask |= ovrAvatarTouch_Pointing;
		if (inputState.Touches & ovrTouch_RThumbUp) state->touchMask |= ovrAvatarTouch_ThumbUp;
		state->isActive = (inputState.ControllerType & ovrControllerType_RTouch) != 0;
	}
}

static void _ovrAvatarTransformFromPOS(const ovrVector3f& position, const ovrQuatf& orientation, const ovrVector3f& scale, ovrAvatarTransform* target) {
	target->position.x = position.x;
	target->position.y = position.y;
	target->position.z = position.z;
	target->orientation.x = orientation.x;
	target->orientation.y = orientation.y;
	target->orientation.z = orientation.z;
	target->orientation.w = orientation.w;
	target->scale.x = scale.x;
	target->scale.y = scale.y;
	target->scale.z = scale.z;
}

void VR::updateAvatar()
{
	if (!avatarInfo.readyToRender) return;
	ovrInputState touchState;
	ovr_GetInputState(session, ovrControllerType_Active, &touchState);
	ovrAvatarHandInputState inputStateLeft, inputStateRight;
	ovrAvatarTransform left, right;
	// left
	auto leftP = avatarInfo.trackingState->HandPoses[ovrHand_Left].ThePose.Position;
	auto leftQ = avatarInfo.trackingState->HandPoses[ovrHand_Left].ThePose.Orientation;
	_ovrAvatarTransformFromPOS(leftP, leftQ, Vector3f(1.0f), &left);
	_ovrAvatarHandInputStateFromOvr(left, touchState, ovrHand_Left, &inputStateLeft);
	// right
	auto rightP = avatarInfo.trackingState->HandPoses[ovrHand_Right].ThePose.Position;
	auto rightQ = avatarInfo.trackingState->HandPoses[ovrHand_Right].ThePose.Orientation;
	_ovrAvatarTransformFromPOS(rightP, rightQ, Vector3f(1.0f), &right);
	_ovrAvatarHandInputStateFromOvr(right, touchState, ovrHand_Right, &inputStateRight);

	ovrAvatarPose_UpdateHands(avatar, inputStateLeft, inputStateRight);
	float deltaTimeInSeconds = (float)xapp->gametime.getDeltaTime();
	ovrAvatarPose_Finalize(avatar, deltaTimeInSeconds);
	//Log("L touch " << inputStateLeft.transform.position.x << " " << inputStateLeft.handTrigger << endl);
	//Log(" leftp " << leftP.x << " " << leftP.y << " " << leftP.z << endl);
	ovrAvatarTransform finalPose = left;
	finalPose.position.x += xapp->camera.pos.x;
	finalPose.position.y += xapp->camera.pos.y;
	finalPose.position.z = xapp->camera.pos.z - left.position.z;
	avatarInfo.controllerLeft.o.pos() = XMFLOAT3(finalPose.position.x, finalPose.position.y, finalPose.position.z);
	// save quaternion and do correction for left handed system:
	avatarInfo.controllerLeft.o.quaternion.x = -finalPose.orientation.x;
	avatarInfo.controllerLeft.o.quaternion.y = -finalPose.orientation.y;
	avatarInfo.controllerLeft.o.quaternion.z = finalPose.orientation.z;
	avatarInfo.controllerLeft.o.quaternion.w = finalPose.orientation.w;
	avatarInfo.controllerLeft.o.useQuaternionRotation = true;

	avatarInfo.handLeft.o.pos() = XMFLOAT3(finalPose.position.x, finalPose.position.y, finalPose.position.z);
	// save quaternion and do correction for left handed system:
	avatarInfo.handLeft.o.quaternion.x = -finalPose.orientation.x;
	avatarInfo.handLeft.o.quaternion.y = -finalPose.orientation.y;
	avatarInfo.handLeft.o.quaternion.z = finalPose.orientation.z;
	avatarInfo.handLeft.o.quaternion.w = finalPose.orientation.w;
	avatarInfo.handLeft.o.useQuaternionRotation = true;

	finalPose = right;
	finalPose.position.x += xapp->camera.pos.x;
	finalPose.position.y += xapp->camera.pos.y;
	finalPose.position.z = xapp->camera.pos.z - right.position.z;
	avatarInfo.controllerRight.o.pos() = XMFLOAT3(finalPose.position.x, finalPose.position.y, finalPose.position.z);
	// save quaternion and do correction for left handed system:
	avatarInfo.controllerRight.o.quaternion.x = -finalPose.orientation.x;
	avatarInfo.controllerRight.o.quaternion.y = -finalPose.orientation.y;
	avatarInfo.controllerRight.o.quaternion.z = finalPose.orientation.z;
	avatarInfo.controllerRight.o.quaternion.w = finalPose.orientation.w;
	avatarInfo.controllerRight.o.useQuaternionRotation = true;

	avatarInfo.handRight.o.pos() = XMFLOAT3(finalPose.position.x, finalPose.position.y, finalPose.position.z);
	// save quaternion and do correction for left handed system:
	avatarInfo.handRight.o.quaternion.x = -finalPose.orientation.x;
	avatarInfo.handRight.o.quaternion.y = -finalPose.orientation.y;
	avatarInfo.handRight.o.quaternion.z = finalPose.orientation.z;
	avatarInfo.handRight.o.quaternion.w = finalPose.orientation.w;
	avatarInfo.handRight.o.useQuaternionRotation = true;
}

void VR::drawController(bool isLeft)
{
	if (!avatarInfo.readyToRender) return;

	auto* mesh = isLeft ? avatarInfo.controllerLeft.renderPartPBS : avatarInfo.controllerRight.renderPartPBS;
	assert(mesh);
	XMMATRIX skinnedPoses[MAX_BONES];  // current world pose matrices
	computeWorldPose(mesh->skinnedPose, skinnedPoses);
	for (int i = 0; i < mesh->skinnedPose.jointCount; ++i)
	{
		if (isLeft)
			xapp->world.path.updateBindPose(i, avatarInfo.controllerLeft.o.pathDescBone, &skinnedPoses[i]);
		else
			xapp->world.path.updateBindPose(i, avatarInfo.controllerRight.o.pathDescBone, &skinnedPoses[i]);
		// inv * pose will be done later
	}
	if (isLeft) {
		avatarInfo.controllerLeft.o.update();
		avatarInfo.controllerLeft.o.draw();
	}
	else {
		avatarInfo.controllerRight.o.update();
		avatarInfo.controllerRight.o.draw();
	}
}

void VR::drawHand(bool isLeft)
{
	if (!avatarInfo.readyToRender) return;

	auto* mesh = isLeft ? avatarInfo.handLeft.renderPart : avatarInfo.handRight.renderPart;
	assert(mesh);
	XMMATRIX skinnedPoses[MAX_BONES];  // current world pose matrices
	computeWorldPose(mesh->skinnedPose, skinnedPoses);
	for (int i = 0; i < mesh->skinnedPose.jointCount; ++i)
	{
		if (isLeft)
			xapp->world.path.updateBindPose(i, avatarInfo.handLeft.o.pathDescBone, &skinnedPoses[i]);
		else
			xapp->world.path.updateBindPose(i, avatarInfo.handRight.o.pathDescBone, &skinnedPoses[i]);
		// inv * pose will be done later
	}
	if (isLeft) {
		avatarInfo.handLeft.o.update();
		avatarInfo.handLeft.o.draw();
	}
	else {
		avatarInfo.handRight.o.update();
		avatarInfo.handRight.o.draw();
	}
}

#else

void VR::handleOVRMessages()
{
}

void VR::loadAvatarDefault()
{
}

void VR::loadAvatarFromOculus(bool reloadAssets)
{
}

void VR::nextTracking()
{
}

void VR::submitFrame(Frame* frame, Pipeline* pipeline, FrameDataGeneral* fdg)
{
#if defined(_SVR_)
	//vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);
	vr::VRCompositor()->PostPresentHandoff();

	// uv min upper left, uvmax lower right
	vr::VRTextureBounds_t bounds;
	bounds.uMin = 0.0f;
	bounds.uMax = 0.5f;
	bounds.vMin = 0.0f;
	bounds.vMax = 1.0f;

	vr::D3D12TextureData_t d3d12LeftEyeTexture = { fdg->renderTargetRenderTexture.Get(), dxGlobal->commandQueue.Get(), 0 };
	vr::Texture_t leftEyeTexture = { (void*)& d3d12LeftEyeTexture, vr::TextureType_DirectX12, vr::ColorSpace_Gamma };
	vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture, &bounds, vr::Submit_Default);

	bounds.uMin = 0.5f;
	bounds.uMax = 1.0f;

	vr::D3D12TextureData_t d3d12RightEyeTexture = { fdg->renderTargetRenderTexture.Get(), dxGlobal->commandQueue.Get(), 0 };
	vr::Texture_t rightEyeTexture = { (void*)& d3d12RightEyeTexture, vr::TextureType_DirectX12, vr::ColorSpace_Gamma };
#endif
	//Util::logThreadInfo(wstring(L"VRCompositor()->Submit"));
#if defined(_SVR_)
	vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture, &bounds, vr::Submit_Default);
#endif	
}

int VR::getCurrentFrameBufferIndex() {
	return 0; // xapp->getCurrentBackBufferIndex(); TODO probably not needed
};

void VR::drawController(bool isLeft)
{
}

void VR::drawHand(bool isLeft)
{
}
#endif
/*
 pose is returned as HmdMatrix34_t (3 lines, 4 rows matrix)
 Axes: left   x: m[0][0] - m[2][0]
 Axes: up     y: m[0][1] - m[2][1]
 Axes: foward z: m[0][2] - m[2][2]
 Translation:    m[0][3] - m[2][3]

 apply camera position:
   m[0][3] = x
   m[1][3] = y
   m[0][3] = x
*/
#if defined(_SVR_)
void VR::UpdateHMDMatrixPose(Camera* cam)
{
	if (!m_pHMD)
		return;

	//Log("WaitGetPoses()" << endl);
	vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

	m_iValidPoseCount = 0;
	m_strPoseClasses = "";
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
	{
		if (m_rTrackedDevicePose[nDevice].bPoseIsValid)
		{
			m_iValidPoseCount++;
			m_rmat4DevicePose[nDevice] = ConvertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
			if (m_rDevClassChar[nDevice] == 0)
			{
				switch (m_pHMD->GetTrackedDeviceClass(nDevice))
				{
				case vr::TrackedDeviceClass_Controller:        m_rDevClassChar[nDevice] = 'C'; break;
				case vr::TrackedDeviceClass_HMD:               m_rDevClassChar[nDevice] = 'H'; break;
				case vr::TrackedDeviceClass_Invalid:           m_rDevClassChar[nDevice] = 'I'; break;
				case vr::TrackedDeviceClass_GenericTracker:    m_rDevClassChar[nDevice] = 'G'; break;
				case vr::TrackedDeviceClass_TrackingReference: m_rDevClassChar[nDevice] = 'T'; break;
				default:                                       m_rDevClassChar[nDevice] = '?'; break;
				}
			}
			m_strPoseClasses += m_rDevClassChar[nDevice];
		}
	}

	if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		// adjust position to camera:
		if (cam != nullptr) {
			m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m[0][3] += cam->pos.x;
			m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m[1][3] += cam->pos.y;
			m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m[2][3] -= cam->pos.z;
			m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd] = ConvertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);
			// end
		}
		m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd];
		m_mat4HMDPose.invert();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Converts a SteamVR matrix to our local matrix class
//-----------------------------------------------------------------------------
Matrix4 VR::ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t& matPose)
{
	Matrix4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);
	return matrixObj;
}

//-----------------------------------------------------------------------------
// Purpose: Gets a Current View Projection Matrix with respect to nEye,
//          which may be an Eye_Left or an Eye_Right.
//-----------------------------------------------------------------------------
Matrix4 VR::GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye)
{
	Matrix4 matMVP;
	if (nEye == vr::Eye_Left)
	{
		matMVP = m_mat4ProjectionLeft * m_mat4eyePosLeft * m_mat4HMDPose;
	}
	else if (nEye == vr::Eye_Right)
	{
		matMVP = m_mat4ProjectionRight * m_mat4eyePosRight * m_mat4HMDPose;
	}
	matMVP.transpose();
	return matMVP;
}

//-----------------------------------------------------------------------------
// Purpose: Gets an HMDMatrixPoseEye with respect to nEye.
//-----------------------------------------------------------------------------
Matrix4 VR::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{
	if (!m_pHMD)
		return Matrix4();

	vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);
	Matrix4 matrixObj(
		matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
		matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
		matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
		matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f
	);

	return matrixObj.invert();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void VR::SetupCameras()
{
	m_mat4ProjectionLeft = GetHMDMatrixProjectionEye(vr::Eye_Left);
	m_mat4ProjectionRight = GetHMDMatrixProjectionEye(vr::Eye_Right);
	m_mat4eyePosLeft = GetHMDMatrixPoseEye(vr::Eye_Left);
	m_mat4eyePosRight = GetHMDMatrixPoseEye(vr::Eye_Right);
}

//-----------------------------------------------------------------------------
// Purpose: Gets a Matrix Projection Eye with respect to nEye.
//-----------------------------------------------------------------------------
Matrix4 VR::GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye)
{
	if (!m_pHMD)
		return Matrix4();

	vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, m_fNearClip, m_fFarClip);

	return Matrix4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
}


#endif

// VR2 test
VR2::VR2()
{
	//m_rmat4DevicePose = new Matrix4[vr::k_unMaxTrackedDeviceCount];
	//m_rTrackedDevicePose = new vr::TrackedDevicePose_t[vr::k_unMaxTrackedDeviceCount];
}

void VR2::init(Pipeline* pipeline, DXGlobal* dxglobal) {
	this->pipeline = pipeline;
	this->dxGlobal = dxglobal;
	//pipeline->setVRImplementation(this); //	TODO reenable for this class to work!!!
	Log("size of VR2: " << sizeof(VR2) << endl);
	Log("size of VR2: " << sizeof(*this) << endl);
#if defined(_SVR_)
	// Loading the SteamVR Runtime
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None)
	{
		m_pHMD = NULL;
		char buf[1024];
		sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		Log(L"svr_Initialize failed: " << vr::VR_GetVRInitErrorAsEnglishDescription(eError) << endl);
		Error(L"svr_Initialize failed");
	}
	if (!vr::VRCompositor())
	{
		Log(L"VRCompositor not available" << endl);
		Error(L"VRCompositor not available");
	}
	// set async mode - should enable multi thread queue access
	vr::VRCompositor()->SetExplicitTimingMode(vr::EVRCompositorTimingMode::VRCompositorTimingMode_Explicit_ApplicationPerformsPostPresentHandoff);
#endif
}

VR2::~VR2() {
	if (!pipeline->isHMD()) return;
#if defined(_SVR_)
	//vr::VRCompositor()->CompositorQuit();
	vr::VR_Shutdown();
#endif
}

void VR2::init()
{
	//pipeline->setVRImplementation(this);
}

void VR2::initD3D()
{
}

void VR2::initFrame()
{
}

void VR2::startFrame()
{
#if defined(_SVR_)
	//Util::logThreadInfo(L"VRCompositor()->SubmitExplicitTimingData()");
	vr::VRCompositor()->SubmitExplicitTimingData();
#endif
	curEye = EyeLeft;
}

void VR2::endFrame()
{
}

void VR2::prepareEyes(VR_Eyes* eyes)
{
	auto config = pipeline->getPipelineConfig();
	auto width = config.backbufferWidth;
	auto height = config.backbufferHeight;
	D3D12_VIEWPORT viewport;
	viewport.MinDepth = 0.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = static_cast<float>(width);
	viewport.Height = static_cast<float>(height);
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissorRect;
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = static_cast<LONG>(width);
	scissorRect.bottom = static_cast<LONG>(height);

	if (!pipeline->isVR()) {
		eyes->viewports[0] = viewport;
		eyes->viewports[1] = viewport;
		eyes->scissorRects[0] = scissorRect;
		eyes->scissorRects[1] = scissorRect;
	}
	else {
		eyes->viewports[EyeLeft] = viewport;
		eyes->scissorRects[EyeLeft] = scissorRect;
		eyes->viewports[EyeRight] = viewport;
		eyes->scissorRects[EyeRight] = scissorRect;
		eyes->viewports[EyeLeft].Width /= 2;
		eyes->scissorRects[EyeLeft].right /= 2;
		//scissorRects[EyeLeft].right--;
		eyes->viewports[EyeRight].Width /= 2;
		eyes->viewports[EyeRight].TopLeftX = eyes->viewports[EyeLeft].Width;
		eyes->scissorRects[EyeRight].left = eyes->scissorRects[EyeLeft].right;
	}
}

void VR2::prepareViews(D3D12_VIEWPORT& viewport, D3D12_RECT& scissorRect)
{
}

void VR2::prepareDraw()
{
}

void VR2::endDraw() {
}


void VR2::adjustEyeMatrix(XMMATRIX& m, Camera* cam) {
}

void VR2::nextEye() {
}

bool VR2::isFirstEye() {
	return firstEye;
}
void VR2::handleOVRMessages()
{
}

void VR2::loadAvatarDefault()
{
}

void VR2::loadAvatarFromOculus(bool reloadAssets)
{
}

void VR2::nextTracking()
{
}

void VR2::submitFrame(Frame* frame, Pipeline* pipeline, FrameDataGeneral* fdg)
{
#if defined(_SVR_)
	//vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);
	vr::VRCompositor()->PostPresentHandoff();

	// uv min upper left, uvmax lower right
	vr::VRTextureBounds_t bounds;
	bounds.uMin = 0.0f;
	bounds.uMax = 0.5f;
	bounds.vMin = 0.0f;
	bounds.vMax = 1.0f;

	vr::D3D12TextureData_t d3d12LeftEyeTexture = { fdg->renderTargetRenderTexture.Get(), dxGlobal->commandQueue.Get(), 0 };
	vr::Texture_t leftEyeTexture = { (void*)& d3d12LeftEyeTexture, vr::TextureType_DirectX12, vr::ColorSpace_Gamma };
	vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture, &bounds, vr::Submit_Default);

	bounds.uMin = 0.5f;
	bounds.uMax = 1.0f;

	vr::D3D12TextureData_t d3d12RightEyeTexture = { fdg->renderTargetRenderTexture.Get(), dxGlobal->commandQueue.Get(), 0 };
	vr::Texture_t rightEyeTexture = { (void*)& d3d12RightEyeTexture, vr::TextureType_DirectX12, vr::ColorSpace_Gamma };
#endif
	//Util::logThreadInfo(wstring(L"VRCompositor()->Submit"));
#if defined(_SVR_)
	vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture, &bounds, vr::Submit_Default);
#endif	
}

int VR2::getCurrentFrameBufferIndex() {
	return 0; // xapp->getCurrentBackBufferIndex(); TODO probably not needed
};

void VR2::drawController(bool isLeft)
{
}

void VR2::drawHand(bool isLeft)
{
}
/*
 pose is returned as HmdMatrix34_t (3 lines, 4 rows matrix)
 Axes: left   x: m[0][0] - m[2][0]
 Axes: up     y: m[0][1] - m[2][1]
 Axes: foward z: m[0][2] - m[2][2]
 Translation:    m[0][3] - m[2][3]

 apply camera position:
   m[0][3] = x
   m[1][3] = y
   m[0][3] = x
*/

#if defined(_SVR_)
void VR2::UpdateHMDMatrixPose(Camera* cam)
{
	if (!m_pHMD)
		return;

	//Log("WaitGetPoses()" << endl);
	vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);
/*
	m_iValidPoseCount = 0;
	m_strPoseClasses = "";
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
	{
		if (m_rTrackedDevicePose[nDevice].bPoseIsValid)
		{
			m_iValidPoseCount++;
			m_rmat4DevicePose[nDevice] = ConvertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
			if (m_rDevClassChar[nDevice] == 0)
			{
				switch (m_pHMD->GetTrackedDeviceClass(nDevice))
				{
				case vr::TrackedDeviceClass_Controller:        m_rDevClassChar[nDevice] = 'C'; break;
				case vr::TrackedDeviceClass_HMD:               m_rDevClassChar[nDevice] = 'H'; break;
				case vr::TrackedDeviceClass_Invalid:           m_rDevClassChar[nDevice] = 'I'; break;
				case vr::TrackedDeviceClass_GenericTracker:    m_rDevClassChar[nDevice] = 'G'; break;
				case vr::TrackedDeviceClass_TrackingReference: m_rDevClassChar[nDevice] = 'T'; break;
				default:                                       m_rDevClassChar[nDevice] = '?'; break;
				}
			}
			m_strPoseClasses += m_rDevClassChar[nDevice];
		}
	}

	if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		// adjust position to camera:
		if (cam != nullptr) {
			static float adder = 0.0f;
			adder += 0.10f;
			m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m[0][3] = cam->pos.x;
			m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m[1][3] = cam->pos.y;
			m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m[2][3] = -cam->pos.z;
			m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd] = ConvertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);
			// end
		}
		m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd];
		m_mat4HMDPose.invert();
	}
*/
}

//-----------------------------------------------------------------------------
// Purpose: Converts a SteamVR matrix to our local matrix class
//-----------------------------------------------------------------------------
Matrix4 VR2::ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t& matPose)
{
	return Matrix4();
}

//-----------------------------------------------------------------------------
// Purpose: Gets a Current View Projection Matrix with respect to nEye,
//          which may be an Eye_Left or an Eye_Right.
//-----------------------------------------------------------------------------
Matrix4 VR2::GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye)
{
	return Matrix4();
}

//-----------------------------------------------------------------------------
// Purpose: Gets an HMDMatrixPoseEye with respect to nEye.
//-----------------------------------------------------------------------------
Matrix4 VR2::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{
	return Matrix4();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void VR2::SetupCameras()
{
}

//-----------------------------------------------------------------------------
// Purpose: Gets a Matrix Projection Eye with respect to nEye.
//-----------------------------------------------------------------------------
Matrix4 VR2::GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye)
{
	return Matrix4();
}

#endif
