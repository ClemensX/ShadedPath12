#include "stdafx.h"

void Command::task(XApp * xapp)
{
	Log("execute command  xapp = " << xapp << endl);
	try {
		//Sleep(5000);
		WorkerQueue &worker = xapp->workerQueue;
		RenderQueue &render = xapp->renderQueue;
		bool cont = true;
		while (cont) {
			if (xapp->isShutdownMode()) break;
			WorkerCommand *command = worker.pop();
			command->perform();
		}
	}
	catch (char *s) {
		Log("task finshing due to exception: " << s << endl);
	}
	Log("task finshed" << endl);
}

void Command::renderQueueTask(XApp * xapp)
{
	Log("execute render queue command  xapp = " << xapp << endl);
	try {
		RenderQueue &render = xapp->renderQueue;
		bool cont = true;
		while (cont) {
			if (xapp->isShutdownMode()) break;
			RenderCommand command = render.pop();
			ID3D12CommandList* ppCommandLists[] = { command.commandList };
			unsigned int current_frame = xapp->getCurrentBackBufferIndex();
			unsigned int render_command_frame = command.frameNum;
			//assert(current_frame == render_command_frame);
			Log("render queue frame: " << xapp->getCurrentBackBufferIndex() << endl);
			// only render if swapchain and command list operate on same frame
			if (current_frame == render_command_frame) {
				//xapp->dxmanager.waitGPU(*command.frameResource, xapp->appWindow.commandQueue);
				xapp->appWindow.commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
				ThrowIfFailedWithDevice(xapp->appWindow.swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING), xapp->device.Get());
			}
			//command->perform();
		}
	}
	catch (char *s) {
		Log("task finshing due to exception: " << s << endl);
	}
	Log("task finshed" << endl);
}