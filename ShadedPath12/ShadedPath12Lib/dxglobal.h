class Pipeline;
struct FrameDataGeneral;
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
	void initFrameBufferResources(FrameDataGeneral* fd);
	void initSwapChain(Pipeline *pipeline);
	DXGlobalParam config;
	ComPtr<IDXGIFactory4> factory;
	IDXGraphicsAnalysis* pGraphicsAnalysis = nullptr; // check for nullpointer before using - only available during graphics diagnostics session
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<IDXGISwapChain3> swapChain;
	ComPtr<ID3D11Device> device11;
	//ComPtr<ID3D11DeviceContext> deviceContext11;
	ComPtr<ID3D11On12Device> device11On12;
	ComPtr<ID2D1Factory3> d2dFactory;
	ComPtr<ID2D1Device2> d2dDevice;
	ComPtr<ID2D1DeviceContext2> d2dDeviceContext;
	ComPtr<IDWriteFactory> dWriteFactory;
};

// Frame data unrelated to a specific effect that needs to be unique for each slot
struct FrameDataGeneral {
	ComPtr<ID3D11DeviceContext> deviceContext11; // cannot use D3D11 DeviceContext in multi-thread code
};