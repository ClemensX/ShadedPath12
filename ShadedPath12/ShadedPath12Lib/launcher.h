#pragma once

/*
* Launch window. Specify which app to run here.
*/
class Launcher
{
public:
	void init(HWND hwnd);
	void start();
	void stop();
	//Simple2dFrame app;
	BillboardApp app;
};

