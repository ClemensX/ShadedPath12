#include "stdafx.h"
//#include "effectbase.h"

void EffectBase::createSyncPoint(FrameResourceSimple &f, ComPtr<ID3D12CommandQueue> queue)
{
	UINT64 threadFenceValue = InterlockedIncrement(&f.fenceValue);
	ThrowIfFailed(queue->Signal(f.fence.Get(), threadFenceValue));
	ThrowIfFailed(f.fence->SetEventOnCompletion(threadFenceValue, f.fenceEvent));
}

void EffectBase::waitForSyncPoint(FrameResourceSimple & f)
{
	//	int frameIndex = xapp->getCurrentBackBufferIndex();
	UINT64 completed = f.fence->GetCompletedValue();
	//Log("ev start " << frameIndex << " " << completed << " " << f.fenceValue << endl);
	if (completed == -1) {
		Error(L"fence.GetCompletedValue breakdown");
	}
	if (completed > 100000) {
		//Log("ev MAX " << completed << " " << f.fenceValue << endl);
	}
	if (completed <= f.fenceValue)
	{
		WaitForSingleObject(f.fenceEvent, INFINITE);
	}
	else {
		//Log("ev " << completed << " " << f.fenceValue << endl);
	}
}

void EffectBase::createConstantBuffer(size_t s, wchar_t * name)
{
	UINT cbvSize = calcConstantBufferSize((UINT)s);
	//ThrowIfFailed(xapp->device->CreateCommittedResource(
	//	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
	//	D3D12_HEAP_FLAG_NONE, // do not set - dx12 does this automatically depending on resource type
	//	&CD3DX12_RESOURCE_DESC::Buffer(cbvSize),
	//	D3D12_RESOURCE_STATE_GENERIC_READ,
	//	nullptr,
	//	IID_PPV_ARGS(&cbvResource)));
	//cbvResource.Get()->SetName(name);
	////Log("GPU virtual: " <<  cbvResource->GetGPUVirtualAddress(); << endl);
	//ThrowIfFailed(cbvResource->Map(0, nullptr, reinterpret_cast<void**>(&cbvGPUDest)));
}


D3D12_GPU_VIRTUAL_ADDRESS EffectBase::getCBVVirtualAddress(int frame, int thread, UINT objectIndex, int eyeNum)
{
	// TODO correction for OVR mode
	//assert(XApp::FrameCount*thread + frame <= singleCBVResources);
	UINT64 plus = slotSize * objectIndex;
	if (xapp->ovrRendering) {
		plus *= 2; // adjust for two eyes
	}
	if (xapp->ovrRendering && eyeNum == 1) {
		plus += slotSize;
	}
	//UINT64 va = singleCBVResources[XApp::FrameCount*thread + frame]->GetGPUVirtualAddress() + plus;
	//Log("va " << va << endl);
	return singleCBVResources[XApp::FrameCount*thread + frame]->GetGPUVirtualAddress() + plus;
}

UINT8* EffectBase::getCBVUploadAddress(int frame, int thread, UINT objectIndex, int eyeNum)
{
	// TODO: slotsize has to be object specific?
	//assert(XApp::FrameCount*thread + frame <= singleCBVResources.size());
	UINT8* mem = singleCBV_GPUDests[XApp::FrameCount*thread + frame];
	if (mem == nullptr) {
		return this->cbvGPUDest;
	}
	UINT64 plus = slotSize * objectIndex;
	if (xapp->ovrRendering) {
		plus *= 2; // adjust for two eyes
	}
	if (xapp->ovrRendering && eyeNum == 1) {
		plus += slotSize;
	}
	//Log("vup " << (mem + plus) << endl);
	return  mem + plus;
}

UINT8* EffectBase::getMemUploadAddress(int frame, int thread, UINT objectIndex, int eyeNum)
{
	// TODO: slotsize has to be object specific?
	//assert(XApp::FrameCount*thread + frame <= singleCBVResources.size());
	UINT8* mem = (UINT8 *)singleMemResources[XApp::FrameCount*thread + frame];
	//if (mem == nullptr) {
	//	return this->cbvGPUDest;
	//}
	UINT64 plus = slotSize * objectIndex;
	if (xapp->ovrRendering) {
		plus *= 2; // adjust for two eyes
	}
	if (xapp->ovrRendering && eyeNum == 1) {
		plus += slotSize;
	}
	//Log("vup " << (mem + plus) << endl);
	return  mem + plus;
}

void EffectBase::waitForWorkerThreads()
{
	if (workerThreads.size() > 0) {
		// we still have threads running
		for (auto& t : workerThreads) {
			if (t.joinable()) {
				t.join();
			}
		}
		// all threads finished - remove from list
		workerThreads.clear();
	}
}

//void EffectBase::prepareDraw(VR *vr)
//{
//	vr_eyes.viewports[0] = *vr->getViewportByIndex(0);
//	vr_eyes.viewports[1] = *vr->getViewportByIndex(1);
//	vr_eyes.scissorRects[0] = *vr->getScissorRectByIndex(0);
//	vr_eyes.scissorRects[1] = *vr->getScissorRectByIndex(1);
//}

EffectBase::~EffectBase()
{
	waitForWorkerThreads();
	for (auto m : singleMemResources) {
		delete[] m;
	}
}


// simple clear effect

void ClearEffect::initFrameResource(EffectFrameResource * effectFrameResource, int frameIndex)
{
}

void ClearEffect::init()
{
	xapp = XApp::getInstance();
	EffectBase::initFrameResources();
}

void ClearEffect::draw()
{
}

// global effect

void GlobalEffect::initFrameResource(EffectFrameResource * effectFrameResource, int frameIndex)
{
	// create depth/stencil buffer and render texture
	// Describe and create a depth stencil view (DSV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(xapp->device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&effectFrameResource->dsvHeap)));
 
	// depth stencil
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	// create the depth/stencil texture
	xapp->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, xapp->backbufferWidth, xapp->backbufferHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, xapp->clearColor),
		IID_PPV_ARGS(&effectFrameResource->depthStencil)
	);
	xapp->device->CreateDepthStencilView(effectFrameResource->depthStencil.Get(), &depthStencilDesc, effectFrameResource->dsvHeap->GetCPUDescriptorHandleForHeapStart());
	NAME_D3D12_OBJECT_SUFF(effectFrameResource->depthStencil, frameIndex);

	// Describe and create a render target view (RTV) descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 1;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(xapp->device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&effectFrameResource->rtvHeap)));
	NAME_D3D12_OBJECT_SUFF(effectFrameResource->rtvHeap, frameIndex);
	effectFrameResource->rtvDescriptorSize = xapp->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// create the depth/stencil texture
	xapp->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, xapp->backbufferWidth, xapp->backbufferHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, xapp->clearColor),
		IID_PPV_ARGS(&effectFrameResource->renderTarget)
	);

	// create the render target view from the heap desc and render texture:
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(effectFrameResource->rtvHeap->GetCPUDescriptorHandleForHeapStart());
	xapp->device->CreateRenderTargetView(effectFrameResource->renderTarget.Get(), nullptr, rtvHandle);
	rtvHandle.Offset(1, effectFrameResource->rtvDescriptorSize);
}

void GlobalEffect::init()
{
	xapp = XApp::getInstance();
	EffectBase::initFrameResources();
}

void GlobalEffect::draw()
{
}