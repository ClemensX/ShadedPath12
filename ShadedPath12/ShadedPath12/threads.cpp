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
				worker.endCommand(command);
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
	unique_lock<mutex> lock(monitorMutex);
	if (in_shutdown) {
		throw "WorkerQueue shutdown in push";
	}
	// we should never run out of free slots, so we can push without wait
	assert(freeSlots.size() > 0);
	int free_slot = freeSlots.front();
	freeSlots.pop_front();
	int frameIndex = workerCommand->draw_slot;
	QueueFrameState &state = qframeStates[frameIndex];
	if (state.state == InitFrame) {
		// wait for next initFrame command
		// store incoming render commands
		// ? wait until last finalize finished?
		if (workerCommand->requiredThreadState == InitFrame) {
			assert(state.initSlots.size() == 0);
			state.initSlots.push_front(free_slot);
			commands[free_slot] = workerCommand;
			state.absFrameCount = workerCommand->absFrameCount;
			cond.notify_one();
			return;
		} else if (workerCommand->requiredThreadState == Render) {
			// during init phase we have to store Render Commands for later execution
			state.renderSlots.push_front(free_slot);
			commands[free_slot] = workerCommand;
			return;
		}
	}
	// if we reached here we could not handle the push request
	assert(false);
}

// find oldest init slot
int WorkerQueue::handleInitSlot()
{
	long long oldest = -1;
	// find oldest valid entry first:
	for (auto&& state : qframeStates)
	{
		if (state.state == InitFrame) {
			if (state.initSlots.size() > 0) {
				// we have an init slot
				if (oldest < 0 || (state.absFrameCount < oldest)) {
					oldest = state.absFrameCount;
				}
			}
		}
	}
	if (oldest < 0) return -1;
	for (auto&& state : qframeStates)
	{
		if (state.state == InitFrame) {
			if (state.initSlots.size() > 0 && state.absFrameCount == oldest) {
				int found = state.initSlots.front();
				state.working.push_front(found);
				state.initSlots.pop_front();
				return found;
			}
		}
	}
	return -1;
}

// find oldest render slot
int WorkerQueue::handleRenderSlot()
{
	for (auto&& state : qframeStates)
	{
		if (state.state == Render) {
			if (state.renderSlots.size() > 0) {
				// render slot available
				int found = state.renderSlots.front();
				state.working.push_front(found);
				state.renderSlots.pop_front();
				return found;
			}
		}
	}
	return -1;
}

bool WorkerQueue::isSlotAvailable()
{
	for (auto&& state : qframeStates)
	{
		if (state.state == Render) {
			if (state.renderSlots.size() > 0) {
				return true;
			}
		}
		if (state.state == InitFrame) {
			if (state.initSlots.size() > 0) {
				return true;
			}
		}
	}
	return false;
}

WorkerCommand * WorkerQueue::pop()
{
	unique_lock<mutex> lock(monitorMutex);
	bool valid = true;
	WorkerCommand *workerCommand;
	do {
		while (!isSlotAvailable()) {
			cond.wait(lock);
			if (in_shutdown) {
				throw "WorkerQueue shutdown in pop";
			}
		}
		// look for next command, depending on state of the frames
		int found = handleRenderSlot();
		if (found < 0) {
			found = handleInitSlot();
		}
		if (found >= 0) {
			//assert(myqueue.empty() == false);
			workerCommand = commands[found];//myqueue.front();
			workerCommand->commandIndex = found;
			//myqueue.pop();
			if (!workerCommand->isValidSequence()) {
				valid = false;
				// TODO this is practically active wait - use backup queue or else...
				//myqueue.push(workerCommand);
				workerCommand->addPushedBackCount();
			} else {
				valid = true;
			}
		} else {
			valid = false;
		}
	} while (!valid);
	cond.notify_one();
	return workerCommand;
}

void WorkerQueue::endCommand(WorkerCommand * workerCommand)
{
	unique_lock<mutex> lock(monitorMutex);
	int frameIndex = workerCommand->draw_slot;
	QueueFrameState &state = qframeStates[frameIndex];
	if (state.state == InitFrame) {
		// initFrame ended: switch to Render state
		assert(workerCommand->requiredThreadState == InitFrame); // nothing else should end during init phase
		if (workerCommand->requiredThreadState == InitFrame) {
			assert(state.working.size() == 1 && state.working.front() == workerCommand->commandIndex);
			int found = state.working.front();
			state.working.pop_front();
			freeSlots.push_front(found);
			state.state = Render;
			cond.notify_one();
			return;
		}
	} else if (state.state = Render) {
		assert(workerCommand->requiredThreadState == Render); // currently nothing else should end during init phase
		if (workerCommand->requiredThreadState == Render) {
			// remove this render slot:
			int slot = workerCommand->commandIndex;
			state.working.remove(slot);
			freeSlots.push_front(slot);
			state.state = InitFrame;
			cond.notify_one();
			return;
		}
	}

	assert(false);
}