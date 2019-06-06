#include "stdafx.h"

void Launcher::init()
{
	simple2dFrame.init();
}

void Launcher::initWindow(HWND hwnd)
{
	simple2dFrame.initWindow(hwnd);
}

void Launcher::start()
{
	simple2dFrame.start();
}

void Launcher::stop()
{
	simple2dFrame.stop();
}
