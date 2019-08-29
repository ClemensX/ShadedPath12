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
#if defined(_SVR_)
// you have to copy openvr/bin/win64/openvr_api.dll to repos/ShadedPath12/ShadedPath12/x64/Debug or /Release
#include "../../../openvr/headers/openvr.h"
#pragma comment(lib, "../../../openvr/lib/win64/openvr_api.lib")
#endif
enum EyePos { EyeLeft, EyeRight };

class VR;
class Pipeline;
class Frame;
class DXGlobal;
struct FrameDataGeneral;

class AvatarPartInfo {
public:
	wstring meshFileName;
	wstring textureFileName;
	string meshId;
	string textureId;
#if defined(_OVR_)
	WorldObject o;
	ovrAvatarAssetID ovrMeshId;
	const ovrAvatarRenderPart_SkinnedMeshRenderPBS *renderPartPBS;
	const ovrAvatarRenderPart_SkinnedMeshRender *renderPart;
#endif
};

class AvatarInfo {
public:
	AvatarPartInfo controllerLeft;
	AvatarPartInfo handLeft;
	AvatarPartInfo controllerRight;
	AvatarPartInfo handRight;
	bool readyToRender = false;
#if defined(_OVR_)
	ovrTrackingState *trackingState = nullptr;
#endif
};

// support class used in all effects (each effect has an instance)
// viewports and scissorRects are set by EffectBase::prepareDraw(), they have to be used by both VR ind non-VR rendering
class VR_Eyes {
public:
	void adjustEyeMatrix(XMMATRIX &m, Camera *cam, int eyeNum, VR* vr);
	D3D12_VIEWPORT *getViewportByIndex(int eyeNum) { return &viewports[eyeNum]; };
	D3D12_RECT *getScissorRectByIndex(int eyeNum) { return &scissorRects[eyeNum]; };

	// adjust the MVP matrix according to current eye position
	void adjustEyeMatrix(XMMATRIX &m, Camera *cam = nullptr); // TODO probably not needed

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
	VR();
	void init(Pipeline *pipeline, DXGlobal*);
	~VR();
	// basic OVR initialization, called at start of xapp.init()
	void init();
	// init d3d resources, needs d3d11 devices ready, called within xapp.init()
	void initD3D();
	void initFrame();
	void startFrame();
	void endFrame();
	EyePos getCurrentEye() { return curEye; };

	void prepareEyes(VR_Eyes* eyes);
	void prepareViews(D3D12_VIEWPORT &viewport, D3D12_RECT &scissorRect);
	//D3D12_VIEWPORT *getViewport() { return &viewports[curEye]; };
	//D3D12_RECT *getScissorRect() { return &scissorRects[curEye]; };
	//D3D12_VIEWPORT *getViewportByIndex(int eyeNum) { return &viewports[eyeNum]; };
	//D3D12_RECT *getScissorRectByIndex(int eyeNum) { return &scissorRects[eyeNum]; };
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
	void submitFrame(Frame* frame, Pipeline* pipeline, FrameDataGeneral *fdg);

	// ovr message queue, has to be called from application update()
	void handleOVRMessages();
	// load avatar data (mesh, bones and textures) from Oculus
	void loadAvatarFromOculus(bool reloadAssets = true);
	void loadAvatarDefault();
	void drawController(bool isLeft);
	void drawHand(bool isLeft);
	// if all assets of an avatar have been loaded, gather all the info needed for rendering:
#if defined(_SVR_)
	vr::IVRSystem* m_pHMD;
	vr::IVRRenderModels* m_pRenderModels;
	std::string m_strDriver;
	std::string m_strDisplay;
	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	Matrix4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];
	bool m_rbShowTrackedDevice[vr::k_unMaxTrackedDeviceCount];
	Matrix4 GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
	Matrix4 GetHMDMatrixPoseEye(vr::Hmd_Eye nEye);
	Matrix4 GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye);
	void UpdateHMDMatrixPose(Camera * cam = nullptr);

	Matrix4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t& matPose);
	void SetupCameras();
private:
	ResourceStateHelper* resourceStateHelper = ResourceStateHelper::getResourceStateHelper();
	int m_iTrackedControllerCount;
	int m_iTrackedControllerCount_Last;
	int m_iValidPoseCount;
	int m_iValidPoseCount_Last;
	bool m_bShowCubes;

	std::string m_strPoseClasses;                            // what classes we saw poses for this frame
	char m_rDevClassChar[vr::k_unMaxTrackedDeviceCount];   // for each device, a character representing its class

	int m_iSceneVolumeWidth;
	int m_iSceneVolumeHeight;
	int m_iSceneVolumeDepth;
	float m_fScaleSpacing;
	float m_fScale;

	int m_iSceneVolumeInit;                                  // if you want something other than the default 20x20x20

	float m_fNearClip = 0.1f;
	float m_fFarClip = 300.0f;
	Matrix4 m_mat4HMDPose;
	Matrix4 m_mat4eyePosLeft;
	Matrix4 m_mat4eyePosRight;

	Matrix4 m_mat4ProjectionCenter;
	Matrix4 m_mat4ProjectionLeft;
	Matrix4 m_mat4ProjectionRight;
public:
#endif
#if defined(_OVR_)
	void gatherAvatarComponentInfo(AvatarPartInfo &avatarPartInfo, const ovrAvatarControllerComponent *component);
	void gatherAvatarComponentInfo(AvatarPartInfo &avatarPartInfo, const ovrAvatarHandComponent *component);
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
	// get view matrix for current eye
	XMFLOAT4X4 getOVRViewMatrix();
	XMFLOAT4X4 getOVRViewMatrixByIndex(int eyeNum);
	// get projection matrix for current eye
	XMFLOAT4X4 getOVRProjectionMatrix();
	XMFLOAT4X4 getOVRProjectionMatrixByIndex(int eyeNum);
	XMFLOAT3 getOVRAdjustedEyePosByIndex(int eyeNum);
	int getCurrentFrameBufferIndex() {
		int currentIndex;
		ovr_GetTextureSwapChainCurrentIndex(session, textureSwapChain, &currentIndex);
		return currentIndex;
	};
	ovrAvatar *avatar = nullptr;
#else
	// just return identity matrix if ovr not enabled
	static XMFLOAT4X4 ident;
	// get view matrix for current eye
	XMFLOAT4X4 getOVRViewMatrix() { return ident; };
	// get projection matrix for current eye
	XMFLOAT4X4 getOVRProjectionMatrix() { return ident; };
	XMFLOAT4X4 getOVRViewMatrixByIndex(int eyeNum) { return ident; };
	XMFLOAT4X4 getOVRProjectionMatrixByIndex(int eyeNum) { return ident; };
	XMFLOAT3 getOVRAdjustedEyePosByIndex(int eyeNum) { return XMFLOAT3(0, 0, 0); };
	int getCurrentFrameBufferIndex(); // TODO probably not needed
#endif

	bool enabled = false;  // default: VR is off, switch on by command line option -vrMode
	std::vector<ID3D12Resource*> texResource;
	ComPtr<ID3D12DescriptorHeap> rtvVRHeap;  // Resource Target View Heap
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> texRtv;

	CD3DX12_CPU_DESCRIPTOR_HANDLE getRTVHandle(int frameIndex);
	AvatarInfo avatarInfo;
protected:
	EyePos curEye = EyeLeft;
private:
	bool loadAvatarAssetsFromOculus = false; // true triggers loading all meshes and texures and saving to local files
	void updateAvatar();
	void handleAvatarMessages();
	unsigned int pack(const uint8_t *blend_indices);

	//D3D12_VIEWPORT viewports[2];
	//D3D12_RECT scissorRects[2];

	XMFLOAT4 cam_look, cam_up, cam_pos;
	bool firstEye = false;
	int buffersize_width = 0;
	int buffersize_height = 0;
	Pipeline* pipeline = nullptr;
	DXGlobal* dxGlobal = nullptr;

#if defined(_OVR_)
	void writeOVRMesh(const uint64_t userId, const ovrAvatarMessage_AssetLoaded *assetmsg, const ovrAvatarMeshAssetData *assetdata);
	void writeOVRTexture(const uint64_t userId, const ovrAvatarMessage_AssetLoaded *assetmsg, const ovrAvatarTextureAssetData *assetdata);
	// calculate bind matrix from oculus avatar transform, including transition from OpenGL right hand bind matrices to DirectX left hand system
	void calculateBindMatrix(const ovrAvatarTransform *t, XMFLOAT4X4 *inv);
	// compute 'world' pose from current pose matrices and stored inverse bind matrices.
	// inverse matrices are from the mesh binary file
	// world actually means zero position and no rotation, vertices still have to be applied to WVP projection
	void computeWorldPose(const ovrAvatarSkinnedMeshPose& localPose, XMMATRIX worldPose[]);
	bool debugComponent = false; // set to true for specific components only (like left hand) for easier debugging/logging
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
