#pragma once

namespace Colors {
	const XMFLOAT4 xm{ 1.0f, 0.0f, 1.0f, 1.0f };
	const XMFLOAT4 White = { 1.0f, 1.0f, 1.0f, 1.0f };
	const XMFLOAT4 Black = { 0.0f, 0.0f, 0.0f, 1.0f };
	const XMFLOAT4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	const XMFLOAT4 Green = { 0.0f, 1.0f, 0.0f, 1.0f };
	const XMFLOAT4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	const XMFLOAT4 Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
	const XMFLOAT4 Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
	const XMFLOAT4 Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };

	const XMFLOAT4 Silver = { 0.75f, 0.75f, 0.75f, 1.0f };
	const XMFLOAT4 LightSteelBlue = { 0.69f, 0.77f, 0.87f, 1.0f };
};

class XAppBase
{
public:
	XAppBase();
	~XAppBase();

	virtual void init();
	//virtual void resize();
	virtual void update();
	virtual void draw();
	virtual void destroy();
	virtual string getWindowTitle() = 0;

protected:
	string myClass;

};

class XApp
{
public:
	XApp();
	~XApp();

	string buildInfo = "Shaded Path 12 Engine Build 2016_02_06";

	void init();
	void resize();
	void update();
	void draw();
	void destroy();
	void report();
	void calcBackbufferSizeAndAspectRatio();
	// asset handling
	enum FileCategory { FX, TEXTURE, MESH, SOUND };
	wstring findFile(wstring filename, FileCategory cat);
	void readFile(wstring filename, vector<byte>& buffer, FileCategory cat);
	TextureStore textureStore;

	void registerApp(string name, XAppBase*);
	XAppBase* getApp(string appName);
	void setRunningApp(string appName);
	unordered_map<string, string> parameters;
	void parseCommandLine(string commandline);
	bool getBoolParam(string key, bool default_value = false);
	int getIntParam(string key, int default_value = 0);
	// last shader should call this directly after Present() call
	void frameFinished();
	// check if RTV still has to be cleared - take VR rendering into account
	bool rtvHasToBeCleared() {
		if (rtvCleared) return false;  // nothing to do - already cleared during this frame
		if (!ovrRendering || (ovrRendering && vr.isFirstEye())) {
			return true;
		}
		return false;
	};
	void handleRTVClearing(ID3D12GraphicsCommandList *commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle, D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle) {
		if (rtvHasToBeCleared()) {
			rtvCleared = true;
			const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
			commandList->ClearRenderTargetView(rtv_handle, clearColor, 0, nullptr);
			commandList->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		}
	};

	// query virtual key definitions (VK_) from Winuser.h
	bool keyDown(BYTE key);

	void setHWND(HWND h);
	void resize(int width, int height);
	HWND getHWND();
	// command line and parameters
	string commandline;
	HWND hwnd = 0;
	bool ovrRendering = false;   // use split screen ovr rendering
	bool warp; //use warp device to render (a.k.a. software rendering)
	// on some systems there is no debug version of DX 11 available,
	// then set -disableDX11Debug on command line to be able to let debug build run
	bool disableDX11Debug = false;
	// Graphics debugging does not like line shaders - it crashes on 2nd line shader initialization
	bool disableLineShaders = false;

	int requestWidth, requestHeight;
	// all drawing takes place in backbuffer - output to whatever size the window currently has is only last step
	unsigned int backbufferWidth = 0, backbufferHeight = 0;
	float aspectRatio;

	// 
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D11Device> d3d11Device;
	ComPtr<ID3D11DeviceContext> d3d11DeviceContext;
	ComPtr<ID3D11Device> reald3d11Device;
	ComPtr<ID3D11DeviceContext> reald3d11DeviceContext;
	ComPtr<ID3D11On12Device> d3d11On12Device;
	ComPtr<ID3D12RootSignature> rootSignature;
	static const UINT FrameCount = 3;
	ComPtr<IDXGISwapChain3> swapChain;
	ComPtr<ID3D12CommandQueue> commandQueue;
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;  // Resource Target View Heap
	UINT rtvDescriptorSize;
	ComPtr<ID3D12Resource> renderTargets[FrameCount];
	ComPtr<ID3D11Resource> wrappedBackBuffers[FrameCount];
	ComPtr<ID3D11Texture2D> wrappedTextures[FrameCount];
	ComPtr<ID3D12Resource> depthStencils[FrameCount];
	ComPtr<ID3D12DescriptorHeap> dsvHeaps[FrameCount];
	UINT lastPresentedFrame = 0;  // set directly before Present() to have 'old' frame number available
	BYTE key_state[256];
	int mouseDx;
	int mouseDy;
	bool mouseTodo;
	unsigned long framenum;
	int fps;
	bool anyKeyDown = false;

	// Other framework instances:
	World world;
	Camera camera;
	GameTime gametime;
	VR vr;
	IDXGraphicsAnalysis* pGraphicsAnalysis = nullptr; // check for nullpointer before using - only available during graphics diagnostics session
private:
	UINT frameIndex;
	bool rtvCleared = false; // shaders can ask if ClearRenderTargetView still has to be called (usually only first shader needs to)

	unordered_map<string, XAppBase *> appMap;
	bool initialized = false;
	string appName;

	XAppBase *app = nullptr;
};

// reference to global instance:
XApp& xapp();
void xappDestroy();