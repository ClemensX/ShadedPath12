#include "stdafx.h"

void Command::task(XApp * xapp)
{
	Log("execute command " << this << " xapp = " << xapp << endl);
	//Sleep(5000);
	WorkerQueue &worker = xapp->workerQueue;
	RenderQueue &render = xapp->renderQueue;
	bool cont = true;
	while (cont) {
		if (xapp->isShutdownMode()) break;
		WorkerCommand command = worker.pop();
		//Log("task popped");
		//Log("task finished");
	}
	Log("task finshed" << endl);
}