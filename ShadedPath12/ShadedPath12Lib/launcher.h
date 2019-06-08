#pragma once

/*
* Launch window based application
*/
class Launcher
{
public:
	void init(HWND hwnd);
	void start();
	void stop();
	Simple2dFrame simple2dFrame;
};

