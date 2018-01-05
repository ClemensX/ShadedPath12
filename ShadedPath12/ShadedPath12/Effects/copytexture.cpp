#include "stdafx.h"

void WorkerCopyTextureCommand::perform()
{
	//Log("perform() copy texture command" << endl);
	TextureInfo *tex = xapp->textureStore.getTexture(textureName);
	assert(tex->available);
	auto res = frameResource;
	auto dxmanager = xapp->dxmanager;
	resourceStateHelper->addOrKeep(res->renderTarget.Get(), D3D12_RESOURCE_STATE_COMMON);
	resourceStateHelper->addOrKeep(tex->texSRV.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// wait until GPU has finished with previous commandList
	//Sleep(30);
	//dxmanager->waitGPU(res, commandQueue); done in applicationwindow
	ID3D12GraphicsCommandList *commandList = res->commandList.Get();
	ThrowIfFailed(res->commandAllocator->Reset());
	ThrowIfFailed(commandList->Reset(res->commandAllocator.Get(), res->pipelineState.Get()));

	CD3DX12_TEXTURE_COPY_LOCATION src(tex->texSRV.Get(), 0);
	CD3DX12_TEXTURE_COPY_LOCATION dest(res->renderTarget.Get(), 0);
	CD3DX12_BOX box(0, 0, 512, 512);

	//commandList->CopyResource(renderTargets[frameNum].Get(), HouseTex->texSRV.Get());
	resourceStateHelper->toState(res->renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, commandList);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(res->rtvHeap->GetCPUDescriptorHandleForHeapStart(), 0, res->rtvDescriptorSize);
	commandList->ClearRenderTargetView(rtvHandle, xapp->clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(res->dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	resourceStateHelper->toState(tex->texSRV.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, commandList);
	resourceStateHelper->toState(res->renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, commandList);
	commandList->CopyTextureRegion(&dest, 0, 0, 0, &src, &box);
	resourceStateHelper->toState(tex->texSRV.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, commandList);
	resourceStateHelper->toState(res->renderTarget.Get(), D3D12_RESOURCE_STATE_COMMON, commandList);
	ThrowIfFailed(commandList->Close());
	RenderCommand rc;
	rc.commandList = commandList;
	xapp->renderQueue.push(rc);
	Log(" render queue size: " << xapp->renderQueue.size() << " frame " << res->frameNum << endl);
}

void CopyTextureEffect::init()
{
	xapp = XApp::getInstance();
	assert(xapp->inInitPhase() == true);
	setThreadCount(xapp->getMaxThreadCount());
}

void CopyTextureEffect::setThreadCount(int max)
{
	assert(xapp != nullptr);
	assert(xapp->inInitPhase() == true);
	worker.resize(max);
}

void CopyTextureEffect::draw(string texName)
{
	assert(xapp->inInitPhase() == false);
	// get ref to current command: (here just the frame number, may be more complicated in other effects)
	int index = xapp->getCurrentBackBufferIndex();
	WorkerCopyTextureCommand *c = &worker.at(index);
	c->type = CommandType::WorkerCopyTexture;
	c->textureName = texName;
	c->xapp = xapp;
	c->resourceStateHelper = xapp->appWindow.resourceStateHelper;
	c->frameResource = xapp->appWindow.getCurrentFrameResource();
	//c->commandDetails = c;
	xapp->workerQueue.push(c);
}

