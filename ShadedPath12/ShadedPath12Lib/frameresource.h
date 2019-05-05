
enum WorkerThreadState {
	// no regular render commands will be executed, thy will be put back to queue instead
	// only ClearEffect will be allowed
	InitFrame,
	Render,
	FinishFrame
};

/*
 * FrameResource - Base class for all frame resources.
 * All Thread and Frame related data goes here.
 */
class FrameResource {
public:
	HANDLE fenceEvent;
	ComPtr<ID3D12Fence> fence;
	UINT64 fenceValue;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;  // Resource Target View Heap
	UINT rtvDescriptorSize;
	ComPtr<ID3D12Resource> renderTarget;
	ComPtr<ID3D12Resource> depthStencil;
	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	unsigned int frameIndex;
//	atomic<WorkerThreadState> workerThreadState;
};

/*
* AppWindowFrameResource.
* All resources needed for app window.
*/
class AppWindowFrameResource : public FrameResource {
public:
};

/*
 * AppWindowFrameResource.
 * All resources needed for app window.
 */
class EffectFrameResource : public FrameResource {
public:
};