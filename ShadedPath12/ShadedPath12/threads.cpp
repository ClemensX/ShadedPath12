#include "stdafx.h"

void Command::task(XApp * xapp)
{
	Log("execute command " << this << " xapp = " << xapp << endl);
	Sleep(5000);
	Log("task finshed" << endl);
}