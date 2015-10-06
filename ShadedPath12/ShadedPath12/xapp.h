#pragma once
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

	void setHWND(HWND h);
	void resize(int width, int height);
	HWND getHWND();
	string commandline;
	HWND hwnd = 0;
	bool ovrRendering;   // use split screen ovr rendering
	int requestWidth, requestHeight;
	// all drawing takes place in backbuffer - output to whatever size the window currently has is only last step
	unsigned int backbufferWidth = 0, backbufferHeight = 0;
	float aspectRatio;
	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};

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