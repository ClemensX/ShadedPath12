#include "pch.h"

SingleQueue::SingleQueue()
{
	state = Undefined;
}

void SingleQueue::sync()
{
	state = SyncRequest;
}

void SingleQueue::push(WorkerCommand * cmd)
{
	//assert(state == Synced);
}

WorkerCommand * SingleQueue::pop()
{
	std::cerr << "[          ] pop " << std::endl;
	if (in_shutdown) {
		throw "WorkerQueue shutdown in pop";
	}
	return nullptr;
}

void SingleQueue::endCommand(WorkerCommand * workerCommand)
{
}
