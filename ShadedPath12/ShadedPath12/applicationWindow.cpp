#include "stdafx.h"

void ApplicationWindow::init(XApp *xapp) {
	this->xapp = xapp;
}

void ApplicationWindow::present() {
	assert(xapp);
	Log("app window present()" << endl);
	//Log("xapp device" << xapp << " " << xapp->device << endl);
	//Log("xapp device" << xapp << endl);
}