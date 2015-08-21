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

void XApp::init()
{
	if (!initialized) {
		initialized = true;
	}
	if (appName.length() == 0) {
		// no app name specified - just use first one from iterator
		auto it = appMap.begin();
		XAppBase *a = it->second;
		if (a != nullptr) {
			appName = it->first;
			Log("WARNING: xapp not specified, using this app: " << appName.c_str() << "\n");
		}
	}
	//assert(appName.length() > 0);
	XAppBase *app = getApp(appName);
	if (app != nullptr) {
		//Log("initializing " << appName.c_str() << "\n");
		SetWindowText(getHWND(), string2wstring(app->getWindowTitle()));
		app->init();
	}
	else {
		Log("ERROR: xapp not available " << appName.c_str() << "\n");
		// throw assertion error in debug mode
		assert(app != nullptr);
	}
}

void XApp::registerApp(string name, XAppBase *app)
{
	// check for class name and strip away the 'class ' part:
	if (name.find("class ") != string::npos) {
		name = name.substr(name.find_last_of(' '/*, name.size()*/) + 1);
	}
	appMap[name] = app;
	for_each(appMap.begin(), appMap.end(), [](auto element) {
		Log("xapp registered: " << element.first.c_str() << endl);
	});
}

void XApp::parseCommandLine(string commandline) {
	vector<string> topics = split(commandline, ' ');
	for (string s : topics) {
		if (s.at(0) != '-') continue;
		s = s.substr(1);
		size_t eqPos = s.find('=');
		if (eqPos < 0) {
			// parse options without '='
			parameters[s] = "true";
		}
		else {
			// parse key=value options
			vector<string> kv = split(s, '=');
			if (kv.size() != 2) continue;
			parameters[kv[0]] = kv[1];
		}
		Log("|" << s.c_str() << "|" << endl);
	}
}

bool XApp::getBoolParam(string key, bool default_value) {
	string s = parameters[key];
	if (s.size() == 0) return default_value;
	if (s.compare("false") == 0) return false;
	return true;
}

int XApp::getIntParam(string key, int default_value) {
	string s = parameters[key];
	if (s.size() == 0) return default_value;
	istringstream buffer(s);
	int value;
	buffer >> value;
	return value;
}


XAppBase* XApp::getApp(string appName) {
	return appMap[appName];
}

void XApp::setRunningApp(string app) {
	appName = app;
}

void XApp::setHWND(HWND h) {
	hwnd = (HWND)h;
}

HWND XApp::getHWND() {
	return hwnd;
}

void XApp::resize(int width, int height) {
	// bail out if size didn't actually change, e.g. on minimize/maximize operation
	//if (width == xapp->requestWidth && height == xapp->requestHeight) return;

	requestWidth = width;
	requestHeight = height;
	resize();
}

void XApp::resize() {
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
