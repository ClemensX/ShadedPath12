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