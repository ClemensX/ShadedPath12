#include "pch.h"

SingleQueue::SingleQueue()
{
	state = Undefined;
	//renderCommandsPerPhase[0] = 0; // no render commands for initial phase
}

void SingleQueue::setPlan(RenderPlan plan)
{
	this->plan = plan;
}

void SingleQueue::start()
{
	this->renderPhase = 0;
	this->finishedThisPhase = 0;
}

//void SingleQueue::sync()
//{
//	unique_lock<mutex> lock(monitorMutex);
//	state = SyncRequest;
//	stateUpdate();
//}

void SingleQueue::push(WorkerCommand * cmd, int phase)
{
	unique_lock<mutex> lock(monitorMutex);
	cmd->renderPhase = phase;
	// make sure command was added for a render phase:
	assert(plan.stepRef(phase).type == PlanRender);
	this->queuedCommands.emplace_back(cmd->commandIndex);
	cond.notify_one();
}

WorkerCommand * SingleQueue::pop()
{
	unique_lock<mutex> lock(monitorMutex);
	while (!isSlotAvailable()) {
		LogF("pop wait " << in_shutdown << endl);
		if (!in_shutdown) {
			cond.wait(lock);
			LogF("pop waking" << endl);
		}
		if (in_shutdown) {
			LogF("queue shutdown" << endl);
			throw "WorkerQueue shutdown in pop";
		}
	}
	int found = queuedCommands.front();
	running.push_front(found);
	queuedCommands.pop_front();
	LogF("pop() " << found << endl);
	return commands.at(found);
}

void SingleQueue::addCommandSlot(WorkerCommand * workerCommand)
{
	unique_lock<mutex> lock(monitorMutex);
	int curSize = (int) this->commands.size();
	workerCommand->commandIndex = curSize;
	this->commands.push_back(workerCommand);
	this->freeSlots.push_front(curSize);
}

void SingleQueue::endCommand(WorkerCommand * workerCommand)
{
	unique_lock<mutex> lock(monitorMutex);
}

void SingleQueue::stateUpdate()
{
	// we should be locked via parent method
	//assert(renderCommandsPerPhase.count(renderPhase) == 1);  // make sure we have an entry for this phase
	//int phaseCommands = renderCommandsPerPhase[renderPhase];
	//if (state == SyncRequest) {
	//	// finish all commands of this phase, just queue all new commands
	//	if (phaseCommands == finishedThisPhase) {
	//		state = Synced;
	//	}
	//}
}

bool SingleQueue::isSlotAvailable()
{
	if (queuedCommands.size() <= 0) {
		return false;
	}
	return true;
}
