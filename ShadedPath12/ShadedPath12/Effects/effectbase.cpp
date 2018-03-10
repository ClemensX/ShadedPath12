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

void EffectBase::init(GlobalEffect * globalEffect)
{
	assert(globalEffect != nullptr);
	globalEffect->addNeededCommandSlots(this->neededCommandSlots());
}

// simple clear effect, should be first effect called by app

void ClearEffect::initFrameResource(EffectFrameResource * effectFrameResource, int frameIndex)
{
}

void ClearEffect::init(GlobalEffect *globalEffect)
{
	EffectBase::init(globalEffect);
	this->globalEffect = globalEffect;
	this->xapp = XApp::getInstance();
	this->dxmanager = &xapp->dxmanager;
	this->resourceStateHelper = ResourceStateHelper::getResourceStateHelper();
	assert(xapp->inInitPhase() == true);
	worker.resize(xapp->getMaxThreadCount() < 3 ? 3 : xapp->getMaxThreadCount());
}

void ClearEffect::draw() {
	assert(globalEffect->isInitPhaseEnded());
	// get index and abs frame number
	int index = xapp->getCurrentApp()->draw_slot;//xapp->getCurrentBackBufferIndex();
	long long absFrameCount = xapp->getCurrentApp()->absFrameCount;
	WorkerClearCommand *c = &worker.at(index);
	c->draw_slot = index;
	c->absFrameCount = absFrameCount;
	//c->type = CommandType::WorkerCopyTexture; do not set - still working? TODO
	//c->textureName = texName;
	c->xapp = xapp;
	c->resourceStateHelper = resourceStateHelper;
	// reuse frame resource from GlobEffect, not our own un-initialized one
	c->effectFrameResource = globalEffect->getFrameResource(index);
	c->requiredThreadState = WorkerThreadState::InitFrame;
	assert(index == c->effectFrameResource->frameIndex);
	xapp->workerQueue.push(c);

}

void WorkerClearCommand::perform()
{
	//Log("clear.perform() " << draw_slot << endl);
	// can't do wait if no command list has been executed: comment out for now
	xapp->dxmanager.waitGPU(*effectFrameResource, xapp->appWindow.commandQueue);
	auto res = this->effectFrameResource;
	ID3D12GraphicsCommandList *commandList = res->commandList.Get();
	ThrowIfFailed(res->commandAllocator->Reset());
	ThrowIfFailed(commandList->Reset(res->commandAllocator.Get(), res->pipelineState.Get()));

	float red[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
	float green[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
	float blue[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
	float *clearColor;
	switch (res->frameIndex)
	{
	case 0:
		clearColor = red;
		break;
	case 1:
		clearColor = green;
		break;
	default:
		clearColor = blue;
		break;
	}

	// get rid of debug layer warning CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE
	//clearColor = xapp->clearColor;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(res->rtvHeap->GetCPUDescriptorHandleForHeapStart(), 0, res->rtvDescriptorSize);
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(res->dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	xapp->workerThreadStates[res->frameIndex] = WorkerThreadState::Render;
	Log("clear.perform() " << draw_slot << endl);
}

// global effect, should be last effect called by app
// wil copy render texture to app window

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
		&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0),
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

	// create the render target texture
	xapp->device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, xapp->backbufferWidth, xapp->backbufferHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, xapp->clearColor),
		IID_PPV_ARGS(&effectFrameResource->renderTarget)
	);

	resourceStateHelper->add(effectFrameResource->renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	// create the render target view from the heap desc and render texture:
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(effectFrameResource->rtvHeap->GetCPUDescriptorHandleForHeapStart());
	xapp->device->CreateRenderTargetView(effectFrameResource->renderTarget.Get(), nullptr, rtvHandle);
	rtvHandle.Offset(1, effectFrameResource->rtvDescriptorSize);
	dxmanager->createPSO(*effectFrameResource, frameIndex);
	effectFrameResource->frameIndex = frameIndex;
	xapp->workerThreadStates[frameIndex] = WorkerThreadState::InitFrame;
}


void GlobalEffect::init()
{
	this->xapp = XApp::getInstance();
	this->dxmanager = &xapp->dxmanager;
	this->resourceStateHelper = ResourceStateHelper::getResourceStateHelper();
	EffectBase::initFrameResources();
	assert(xapp->inInitPhase() == true);
	setThreadCount(xapp->getMaxThreadCount());
}

void GlobalEffect::endInitPhase()
{
	initPhaseEnded = true;
	assert(xapp->workerThreads.size() == 0); // init phase not properly ended, start threads after init phase
	xapp->workerQueue.init(xapp->getMaxThreadCount(), xapp->FrameCount, getNeededCommandSlots());
	xapp->startWorkerThreads();
}

void GlobalEffect::setThreadCount(int max)
{
	assert(xapp != nullptr);
	assert(xapp->inInitPhase() == true);
	worker.resize(max < 3 ? 3 : max);
}

// copy finished frame to app window
void GlobalEffect::draw()
{
	assert(initPhaseEnded);
	// get index and abs frame number
	int index = xapp->getCurrentApp()->draw_slot;//xapp->getCurrentBackBufferIndex();
	long long absFrameCount = xapp->getCurrentApp()->absFrameCount;
	WorkerGlobalCopyTextureCommand *c = &worker.at(index);
	c->draw_slot = index;
	c->absFrameCount = absFrameCount;
	//c->type = CommandType::WorkerCopyTexture; do not set - still working? TODO
	//c->textureName = texName;
	c->xapp = xapp;
	c->resourceStateHelper = resourceStateHelper;
	c->appFrameResource = xapp->appWindow.getFrameResource(index);
	c->effectFrameResource = getFrameResource(index);
	assert(index == c->appFrameResource->frameIndex);
	assert(index == c->effectFrameResource->frameIndex);
	c->requiredThreadState = WorkerThreadState::Render;

	//c->commandDetails = c;
	xapp->workerQueue.push(c);
}


void WorkerGlobalCopyTextureCommand::perform()
{
	auto res = this->effectFrameResource;
	Log(" copy texture command begin, frame = " << res->frameIndex << endl);
	ID3D12GraphicsCommandList *commandList = res->commandList.Get();
	resourceStateHelper->toState(appFrameResource->renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, commandList);
	resourceStateHelper->toState(res->renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, commandList);

	//commandList->CopyResource(appFrameResource->renderTarget.Get(), res->renderTarget.Get());
	//
	CD3DX12_TEXTURE_COPY_LOCATION src(res->renderTarget.Get(), 0);
	CD3DX12_TEXTURE_COPY_LOCATION dest(appFrameResource->renderTarget.Get(), 0);
	//CD3DX12_BOX box(0, 0, 512, 512);

	commandList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);

	resourceStateHelper->toState(appFrameResource->renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, commandList);
	resourceStateHelper->toState(res->renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, commandList);
	//

	//Log("copy.perform() " << draw_slot << " abs " << this->absFrameCount << endl);
	assert(xapp->workerThreadStates[res->frameIndex] == WorkerThreadState::Render);
	ThrowIfFailed(commandList->Close());
	RenderCommand rc;
	rc.commandList = commandList;
	rc.writesToSwapchain = true;
	rc.frameIndex = res->frameIndex;
	rc.absFrameCount = this->absFrameCount;
	rc.frameResource = nullptr;//res;
	//Log(" copy texture command finished, frame = " << res->frameIndex << endl);
	//Log("push..");
	xapp->renderQueue.push(rc);
	//Log("done" << endl);
	xapp->workerThreadStates[res->frameIndex] = WorkerThreadState::InitFrame;
}