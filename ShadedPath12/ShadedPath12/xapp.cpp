#include "stdafx.h"
#include "xapp.h"

XAppBase::XAppBase() {
}

XAppBase::~XAppBase() {

}

void XAppBase::init() {

}

void XAppBase::update() {

}

void XAppBase::draw() {

}

XApp::XApp()
{
}

XApp::~XApp()
{
}

void XApp::registerApp(string name, XAppBase *app)
{
	appMap[name] = app;
	for_each(appMap.begin(), appMap.end(), [](auto element) {
		Log("xapp registered: " << element.first.c_str() << endl);
	});
}


// global instance:
static XApp *xappPtr = nullptr;

XApp& xapp() {
	if (xappPtr == nullptr) {
		xappPtr = new XApp();
	}
	return *xappPtr;
}

void xappDestroy() {
	delete xappPtr;
}
