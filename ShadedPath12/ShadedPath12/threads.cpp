#include "stdafx.h"

void Command::task(XApp * xapp)
{
	//Log("execute command t = " << this_thread::get_id() << " xapp = " << xapp << endl);
	Log("execute command t = " << ThreadInfo::thread_osid() << " xapp = " << xapp << endl);
	try {
		//Sleep(5000);
		WorkerQueue &worker = xapp->workerQueue;
		bool cont = true;
		while (cont) {
			if (xapp->isShutdownMode()) break;
			WorkerCommand *command = worker.pop();
			if (command->isValidSequence()) {
				command->perform();
			} else {
				assert(false);
				// we are out of order: reinsert this command into the queue
				//Log("reinserting worker command to queue " << command.absFrameCount << endl);
				xapp->pushedBackWorkerCommands++;
				worker.push(command);
			}
		}
	}
	catch (char *s) {
		Log("task finshing due to exception: " << s << endl);
	}
	Log("task finshed" << endl);
}

void Command::renderQueueTask(XApp * xapp)
{
	//Log("execute render queue command t = " << this_thread::get_id() << " xapp = " << xapp << endl);
	Log("execute render queue command t = " << ThreadInfo::thread_osid() << " xapp = " << xapp << endl);
	try {
		RenderQueue &render = xapp->renderQueue;
		bool cont = true;
		while (cont) {
			if (xapp->isShutdownMode()) break;
			RenderCommand command = render.pop();
			ID3D12CommandList* ppCommandLists[] = { command.commandList };
			unsigned int render_command_frame = command.frameIndex;
			//assert(current_frame == render_command_frame);
			//Log("render queue t = " << this_thread::get_id() << " frame: " << current_frame << " render command frame: " << render_command_frame << endl);
			UINT presentCount;
			xapp->appWindow.swapChain->GetLastPresentCount(&presentCount);
			//Log("Render command abs frame " << command.absFrameCount << " last present frame = " << presentCount << " render queue t = " << ThreadInfo::thread_osid() << " render command frame: " << render_command_frame << endl);
			// only render if swapchain and command list operate on same frame
			if (true) {
				//xapp->dxmanager.waitGPU(*command.frameResource, xapp->appWindow.commandQueue);
				auto swapChainIndex = xapp->appWindow.swapChain->GetCurrentBackBufferIndex();
				if (presentCount + 1 < command.absFrameCount) {
					// we are out of order: reinsert this render command into the queue
					//Log("reinserting render frame to queue " << command.absFrameCount << endl);
					xapp->renderQueue.push(command);
					continue;
				}
				assert(presentCount + 1 == command.absFrameCount); // we cannot present out of order - fix render queue
				//assert(render_command_frame == swapChainIndex);
				xapp->appWindow.commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
				//Log("present swap chain " << xapp->appWindow.swapChain->GetCurrentBackBufferIndex() << endl);
				ThrowIfFailedWithDevice(xapp->appWindow.swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING), xapp->device.Get());
				//Log("render queue t = " << ThreadInfo::thread_osid() << " render command frame: " << render_command_frame << " presented " << endl);
				// unlock this slot to enable next draw call
				xapp->threadState.freeDrawSlot(render_command_frame);
			}
			//command->perform();
		}
	}
	catch (char *s) {
		Log("task finshing due to exception: " << s << endl);
	}
	Log("task finshed" << endl);
}

bool WorkerCommand::isValidSequence()
{
	if (this->xapp->workerThreadStates[this->draw_slot] == this->requiredThreadState) {
		return true;
	}
	return false;
}

void WorkerCommand::addPushedBackCount()
{
	xapp->pushedBackWorkerCommands++;
}

void WorkerQueue::push(WorkerCommand * workerCommand)
{
}

WorkerCommand * WorkerQueue::pop()
{
	return nullptr;
}