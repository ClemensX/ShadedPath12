class Pipeline;
struct FrameDataGeneral;
struct FrameDataD2D;
// global DirectX parameters
struct DXGlobalParam{
	bool ovrRendering = false;   // use split screen ovr rendering
	bool ovrMirror = true;       // if ovrRendering() then this flag indicates if mirroring to app window should occur, always false for non-ovr rendering
	bool warp = false;           //use warp device to render (a.k.a. software rendering)
	// on some systems there is no debug version of DX12 available,
	// then set -disableDX12Debug on command line to be able to let debug build run
	bool disableDX12Debug = false;
	// on some systems there is no debug version of DX11 available,
	// then set -disableDX11Debug on command line to be able to let debug build run
	bool disableDX11Debug = false;
	// Graphics debugging does not like line shaders - it crashes on 2nd line shader initialization
	bool disableLineShaders = false;
};

/*
 * Global DirectX resources
 * Base Initialization of devices etc.
 * Consider as constant after initialization.
*/

class DXGlobal {
public:
	void init();
	void initFrameBufferResources(FrameDataGeneral* fd, FrameDataD2D* fd_d2d, int frameBufferNumber, Pipeline* pipeline);
	void initSwapChain(Pipeline* pipeline, HWND hwnd);
	DXGlobalParam config;
	ComPtr<IDXGIFactory4> factory;
	IDXGraphicsAnalysis* pGraphicsAnalysis = nullptr; // check for nullpointer before using - only available during graphics diagnostics session
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<IDXGISwapChain3> swapChain;
	ResourceStateHelper* resourceStateHelper = ResourceStateHelper::getResourceStateHelper();
};

// Frame data unrelated to a specific effect that needs to be unique for each slot
struct FrameDataGeneral {
	ComPtr<ID3D11DeviceContext> deviceContext11; // cannot use D3D11 DeviceContext in multi-thread code
	ComPtr<ID3D11Device> device11;
	ComPtr<ID3D11On12Device> device11On12;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;  // Resource Target View Heap
	UINT rtvDescriptorSize;
	ComPtr<ID3D12Resource> renderTarget;
	ComPtr<ID3D12Resource> depthStencil;
	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	HANDLE fenceEvent;
	ComPtr<ID3D12Fence> fence;
	UINT64 fenceValue;
};