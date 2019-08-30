class Pipeline;
class Frame;
class TextureStore;
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
	// return true if there is a windows and swap chain available for output
	bool isOutputWindowAvailable() { return swapChain != nullptr; };
	DXGlobalParam config;
	ComPtr<IDXGIFactory4> factory;
	IDXGraphicsAnalysis* pGraphicsAnalysis = nullptr; // check for nullpointer before using - only available during graphics diagnostics session
	ComPtr<ID3D12Device4> device;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<IDXGISwapChain3> swapChain;
	ResourceStateHelper* resourceStateHelper = ResourceStateHelper::getResourceStateHelper();
	float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
	static void createSyncPoint(FrameDataGeneral* f, ComPtr<ID3D12CommandQueue> queue);
	static void waitForSyncPoint(FrameDataGeneral* f);
	// create sync point and wit for completion
	void waitGPU(FrameDataGeneral* res, ComPtr<ID3D12CommandQueue> queue);
	void destroy(Pipeline* pipeline);

	// render methods:

	// copy background render texture to foreground window (possibly from different slots)
	// and present() the swap chain
	void present2Window(Pipeline* pipeline, Frame *frame);
	// prepare comeras by copying global cameras to frame resources
	void prepareCameras(Frame* frame, Pipeline* pipeline, const Camera* cleft, const Camera* cright = nullptr);
	// preapre for next frame draw: wait for GPU to finish and reset command list and allocator
	void waitAndReset(FrameDataGeneral* fd);
	// clear render target to background color
	void clearRenderTexture(FrameDataGeneral* fd);
	void prepare2DRendering(Frame* frame, Pipeline* pipeline, FrameDataD2D* fd2d);
	void end2DRendering(Frame* frame, Pipeline* pipeline, FrameDataD2D* fd2d);
	// copy texture from GPU mem to CPU mem and export it as BMP file
	// this is slow. not intended to be called every frame
	void copyTextureToCPUAndExport(Frame* frame, Pipeline* pipeline, string filename);
	// texture store to be used by effects - has to be set from application during startup init
	void setTextureStore(TextureStore* textureStore) { this->textureStore = textureStore; };
	TextureStore* getTextureStore() { return this->textureStore; };
	void submitVR(Frame* frame, Pipeline* pipeline, FrameDataGeneral *fdg);
	VR2* vr = nullptr;
	// PIX instrumentation:
	void startStatisticsDraw(FrameDataGeneral* fd);
	void endStatisticsDraw(FrameDataGeneral* fd);

private:
	TextureStore* textureStore = nullptr;
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
	ComPtr<ID3D12DescriptorHeap> rtvHeapRenderTexture;  // Resource Target View Heap
	UINT rtvDescriptorSizeRenderTexture;
	ComPtr<ID3D12Resource> renderTargetRenderTexture;
	ComPtr<ID3D12Resource> depthStencilRenderTexture;
	ComPtr<ID3D12DescriptorHeap> dsvHeapRenderTexture;
	ComPtr<ID3D12CommandAllocator> commandAllocatorRenderTexture;
	ComPtr<ID3D12GraphicsCommandList> commandListRenderTexture;
	ComPtr<ID3D12PipelineState> pipelineStateRenderTexture;
	ComPtr<ID3D12RootSignature> rootSignatureRenderTexture;
	HANDLE fenceEventRenderTexture;
	ComPtr<ID3D12Fence> fenceRenderTexture;
	UINT64 fenceValueRenderTexture;
	ComPtr<ID3D11Resource> wrappedDx12Resource;
	VR_Eyes eyes;
	Camera leftCam;
	Camera rightCam;
};