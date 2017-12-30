#pragma once

/*
* Application window is responsible for drawing to window frame
* swap chain is hold here, rest of engine is independent of app window
*/
class ApplicationWindow
{
public:
	void init(XApp *xapp, ComPtr<IDXGIFactory4> &factory);
	void present();
	UINT GetCurrentBackBufferIndex();
	void destroy();
	ComPtr<ID3D12CommandQueue> commandQueue;
private:
	XApp * xapp = nullptr;
	DXManager *dxmanager = nullptr;
	static const UINT FrameCount = 3;
	ComPtr<IDXGISwapChain3> swapChain;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;  // Resource Target View Heap
	UINT rtvDescriptorSize;
	ComPtr<ID3D12Resource> renderTargets[FrameCount];
	//ComPtr<ID3D11Resource> wrappedBackBuffers[FrameCount];
	//ComPtr<ID3D11Texture2D> wrappedTextures[FrameCount];
	ComPtr<ID3D12Resource> depthStencils[FrameCount];
	ComPtr<ID3D12DescriptorHeap> dsvHeaps[FrameCount];
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	FrameResource updateFrameData;
};

