#pragma once

#if defined(_OVR_)
#include "../../../OculusSDK/LibOVR/Include/OVR_CAPI.h"
#include "../../../OculusSDK/LibOVR/Include/OVR_CAPI_D3D.h"
#include "../../../OculusSDK/LibOVR/Include/Extras/OVR_Math.h"
#include "../../../OVRAvatarSDK/Include/OVR_Avatar.h"
//#include "../../../OVRPlatformSDK/Include/OVR_Types.h"
#define OVRPL_DISABLED
// OVR_Types.h cannot be found: include folder of platform SDk has to be added in compiler options
#include "OVR_Platform.h"
using namespace OVR;
#pragma comment(lib, "../../../OVRAvatarSDK/Windows/libovravatar.lib")
#pragma comment(lib, "../../../OVRPlatformSDK/Windows/LibOVRPlatform64_1.lib")
#if defined(_DEBUG)
#pragma comment(lib, "../../../OculusSDK/LibOVR/Lib/Windows/x64/Debug/VS2015/LibOVR.lib")
#else
#pragma comment(lib, "../../../OculusSDK/LibOVR/Lib/Windows/x64/Release/VS2015/LibOVR.lib")
#endif
#endif

enum EyePos { EyeLeft, EyeRight };

class XApp;
class VR;

class AvatarInfo {
public:
	wstring controllerLeftMeshFileName;
	wstring controllerLeftTextureFileName;
	string controllerLeftMeshId;
	ovrAvatarAssetID controllerLeftOvrMeshId;
	string controllerLeftTextureId;
	WorldObject controllerLeft;
	const ovrAvatarRenderPart_SkinnedMeshRenderPBS *controllerLeftRenderPart;
	ovrTrackingState *trackingState = nullptr;
	bool readyToRender = false;
};

// support class used in all effects (each effect has an instance)
// viewports and scissorRects are set by EffectBase::prepareDraw(), they have to be used by both VR ind non-VR rendering
class VR_Eyes {
public:
	void adjustEyeMatrix(XMMATRIX &m, Camera *cam, int eyeNum, VR* vr);
	D3D12_VIEWPORT *getViewportByIndex(int eyeNum) { return &viewports[eyeNum]; };
	D3D12_RECT *getScissorRectByIndex(int eyeNum) { return &scissorRects[eyeNum]; };

	// adjust the MVP matrix according to current eye position
	void adjustEyeMatrix(XMMATRIX &m, Camera *cam = nullptr);

	// get view matrix for current eye
	XMFLOAT4X4 getOVRViewMatrixByIndex(int eyeNum);
	// get projection matrix for current eye
	XMFLOAT4X4 getOVRProjectionMatrixByIndex(int eyeNum);

	D3D12_VIEWPORT viewports[2];
	D3D12_RECT scissorRects[2];
	XMFLOAT4X4 viewOVR[2], projOVR[2];
	XMFLOAT3 adjustedEyePos[2];
};

// global class  - only one instance  - used for global VR data and initialization
class VR {
public:
	VR(XApp *xapp);
	~VR();
	// basic OVR initialization, called at start of xapp.init()
	void init();
	// init d3d resources, needs d3d11 devices ready, called within xapp.init()
	void initD3D();
	void initFrame();
	void startFrame();
	void endFrame();
	EyePos getCurrentEye() { return curEye; };

	void prepareViews(D3D12_VIEWPORT &viewport, D3D12_RECT &scissorRect);
	D3D12_VIEWPORT *getViewport() { return &viewports[curEye]; };
	D3D12_RECT *getScissorRect() { return &scissorRects[curEye]; };
	D3D12_VIEWPORT *getViewportByIndex(int eyeNum) { return &viewports[eyeNum]; };
	D3D12_RECT *getScissorRectByIndex(int eyeNum) { return &scissorRects[eyeNum]; };
	// getHeight and getWidth should only be called after init()
	int getHeight() { return buffersize_height; };
	int getWidth() { return buffersize_width; };
	// prepare VR draw: save current camera pos/look/up, viewport and scissor rect
	void prepareDraw();
	// undo the chages to the camera made by prepareDraw
	void endDraw();
	// adjust the MVP matrix according to current eye position
	void adjustEyeMatrix(XMMATRIX &m, Camera *cam = nullptr);
	// move to 2nd eye
	void nextEye();
	// return if this run is for the first eye - some initializations are not needed for 2nd eye
	bool isFirstEye();
	// read HMD position and generate view parameters for both eyes
	void nextTracking();
	void submitFrame();

	// ovr message queue, has to be called from application update()
	void handleOVRMessages();
	// load avatar data (mesh, bones and textures) from Oculus
	void loadAvatar();
	void drawLeftController();
	// if all assets of an avatar have been loaded, gather all the info needed for rendering:
	void gatherAvatarInfo(AvatarInfo &avatarInfo, ovrAvatar *avatar);
	wstring getTextureFileName(ovrAvatarAssetID id) {
		wstringstream sss;
		sss << std::hex << "ovr_" << id << ".dds";
		return sss.str();
	}
	string getTextureId(ovrAvatarAssetID id) {
		stringstream sss;
		sss << std::hex << id;
		return sss.str();
	}
	wstring getMeshFileName(ovrAvatarAssetID id) {
		wstringstream sss;
		sss << std::hex << "ovr_" << id << ".b";
		return sss.str();
	}
	string getMeshId(ovrAvatarAssetID id) {
		stringstream sss;
		sss << std::hex << id;
		return sss.str();
	}
#if defined(_OVR_)
	// get view matrix for current eye
	XMFLOAT4X4 getOVRViewMatrix();
	XMFLOAT4X4 getOVRViewMatrixByIndex(int eyeNum);
	// get projection matrix for current eye
	XMFLOAT4X4 getOVRProjectionMatrix();
	XMFLOAT4X4 getOVRProjectionMatrixByIndex(int eyeNum);
	XMFLOAT3 getOVRAdjustedEyePosByIndex(int eyeNum);
#else
	// just return identity matrix if ovr not enabled
	static XMFLOAT4X4 ident;
	// get view matrix for current eye
	XMFLOAT4X4 getOVRViewMatrix() { return ident; };
	// get projection matrix for current eye
	XMFLOAT4X4 getOVRProjectionMatrix() { return ident; };
#endif

	bool enabled = false;  // default: VR is off, switch on by command line option -vr
	std::vector<ID3D12Resource*> texResource;
	ComPtr<ID3D12DescriptorHeap> rtvVRHeap;  // Resource Target View Heap
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> texRtv;

	CD3DX12_CPU_DESCRIPTOR_HANDLE getRTVHandle(int frameIndex);
	int getCurrentFrameBufferIndex() {
		int currentIndex;
		ovr_GetTextureSwapChainCurrentIndex(session, textureSwapChain, &currentIndex);
		return currentIndex;
	};
	ovrAvatar *avatar = nullptr;
	AvatarInfo avatarInfo;
protected:
	EyePos curEye = EyeLeft;
private:
	void updateAvatar();
	void handleAvatarMessages();
	void writeOVRMesh(const uint64_t userId, const ovrAvatarMessage_AssetLoaded *assetmsg, const ovrAvatarMeshAssetData *assetdata);
	void writeOVRTexture(const uint64_t userId, const ovrAvatarMessage_AssetLoaded *assetmsg, const ovrAvatarTextureAssetData *assetdata);
	void calculateInverseBindMatrix(const ovrAvatarTransform *t, XMFLOAT4X4 *inv);
	unsigned int pack(const uint8_t *blend_indices);

	D3D12_VIEWPORT viewports[2];
	D3D12_RECT scissorRects[2];
	XApp* xapp;
	XMFLOAT4 cam_look, cam_up, cam_pos;
	bool firstEye = false;
	int buffersize_width = 0;
	int buffersize_height = 0;

#if defined(_OVR_)
	ovrHmdDesc desc;
	ovrSizei resolution;
	ovrSession session;
	ovrGraphicsLuid luid;
	ovrEyeRenderDesc eyeRenderDesc[2];
	ovrPosef         EyeRenderPose[2];     // Useful to remember where the rendered eye originated
	float            YawAtRender[2];       // Useful to remember where the rendered eye originated
	XMFLOAT4X4 viewOVR[2], projOVR[2];
	XMFLOAT3 adjustedEyePos[2];            // store fixed camera pos for usage in effect (aka roomscale)
	//ovrSwapTextureSet *      pTextureSet = 0;
	ovrTextureSwapChain textureSwapChain = 0;
	//std::vector<ID3D11RenderTargetView*> texRtv;
	ovrLayerEyeFov layer;

#endif
};
