#pragma once

/*
* Application window is responsible for drawing to window frame
* swap chain is hold here, rest of engine is independent of app window
*/
class ApplicationWindow
{
public:
	void init(XApp *xapp);
	void present();
private:
	XApp * xapp = nullptr;
};

