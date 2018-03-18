#include "pch.h"

SingleQueue::SingleQueue()
{
	state = Undefined;
	renderCommandsPerPhase[0] = 0; // no render commands for initial phase
}

void SingleQueue::sync()
{
	unique_lock<mutex> lock(monitorMutex);
	state = SyncRequest;
	stateUpdate();
}

void SingleQueue::push(WorkerCommand * cmd)
{
	unique_lock<mutex> lock(monitorMutex);
}

WorkerCommand * SingleQueue::pop()
{
	unique_lock<mutex> lock(monitorMutex);
	std::cerr << "[          ] pop " << std::endl;
	if (in_shutdown) {
		throw "WorkerQueue shutdown in pop";
	}
	return nullptr;
}

void SingleQueue::endCommand(WorkerCommand * workerCommand)
{
	unique_lock<mutex> lock(monitorMutex);
}

void SingleQueue::stateUpdate()
{
	// we should be locked via parent method
	assert(renderCommandsPerPhase.count(renderPhase) == 1);  // make sure we have an entry for this phase
	int phaseCommands = renderCommandsPerPhase[renderPhase];
	if (state == SyncRequest) {
		// finish all commands of this phase, just queue all new commands
		if (phaseCommands == finishedThisPhase) {
			state = Synced;
		}
	}
}