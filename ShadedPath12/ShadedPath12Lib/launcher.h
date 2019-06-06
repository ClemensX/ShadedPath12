#pragma once

/*
* Launch window based application
*/
class Launcher
{
public:
	void init();
	void initWindow(HWND hwnd);
	void start();
	void stop();
	Simple2dFrame simple2dFrame;
};

