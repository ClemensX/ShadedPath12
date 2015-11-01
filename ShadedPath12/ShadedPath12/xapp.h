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
	virtual void next();
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

	void init();
	void resize();
	void update();
	void draw();
	void destroy();
	void report();
	void calcBackbufferSizeAndAspectRatio();
	void registerApp(string name, XAppBase*);
	XAppBase* getApp(string appName);
	void setRunningApp(string appName);
	unordered_map<string, string> parameters;
	void parseCommandLine(string commandline);
	bool getBoolParam(string key, bool default_value = false);
	int getIntParam(string key, int default_value = 0);

	// query virtual key definitions (VK_) from Winuser.h
	bool keyDown(BYTE key);

	void setHWND(HWND h);
	void resize(int width, int height);
	HWND getHWND();
	// command line and parameters
	string commandline;
	HWND hwnd = 0;
	bool ovrRendering;   // use split screen ovr rendering
	bool warp; //use warp device to render (a.k.a. software rendering)

	int requestWidth, requestHeight;
	// all drawing takes place in backbuffer - output to whatever size the window currently has is only last step
	unsigned int backbufferWidth = 0, backbufferHeight = 0;
	float aspectRatio;

	// 
	ComPtr<ID3D12Device> device;
	//UINT getFrameIndex() { return frameIndex; };
	//auto getCommandAllocator() { return commandAllocators[frameIndex]; };
	//auto getPipelineState() { return pipelineState; };
	ComPtr<ID3D12RootSignature> rootSignature;
	static const UINT FrameCount = 3;
	ComPtr<IDXGISwapChain3> swapChain;
	ComPtr<ID3D12CommandQueue> commandQueue;
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;  // Resource Target View Heap
	UINT rtvDescriptorSize;
	ComPtr<ID3D12Resource> renderTargets[FrameCount];
	UINT lastPresentedFrame = -100;  // set directly before Present() to have 'old' frame number available
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
private:

	// Pipeline objects
	ComPtr<ID3D12CommandAllocator> commandAllocators[FrameCount];
	ComPtr<ID3D12GraphicsCommandList> commandList;
	//ComPtr<ID3D11Resource> m_wrappedBackBuffers[FrameCount];
	//ComPtr<ID2D1Bitmap1> m_d2dRenderTargets[FrameCount];
	ComPtr<ID3D12PipelineState> pipelineState;

	// App resources
	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	// Synchronization objects.
	UINT frameIndex;
	HANDLE fenceEvent;
	ComPtr<ID3D12Fence> fence;
	UINT64 fenceValues[FrameCount];

	unordered_map<string, XAppBase *> appMap;
	bool initialized = false;
	string appName;

	void PopulateCommandList();
	void WaitForGpu();
	void MoveToNextFrame();
	XAppBase *app = nullptr;
};

// reference to global instance:
XApp& xapp();
void xappDestroy();