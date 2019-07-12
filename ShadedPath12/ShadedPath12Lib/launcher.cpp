#include "stdafx.h"

void Launcher::init(HWND hwnd)
{
	app.init(hwnd);
}

void Launcher::start()
{
	app.start();
}

void Launcher::stop()
{
	app.stop();
}
