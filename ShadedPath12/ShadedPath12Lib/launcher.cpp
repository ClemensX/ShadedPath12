#include "stdafx.h"

void Launcher::init(HWND hwnd)
{
	simple2dFrame.init(hwnd);
}

void Launcher::start()
{
	simple2dFrame.start();
}

void Launcher::stop()
{
	simple2dFrame.stop();
}
