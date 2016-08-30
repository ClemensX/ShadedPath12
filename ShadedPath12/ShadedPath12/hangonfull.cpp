#include "stdafx.h"
#include "hangonfull.h"

static HangOnFull hangOnFull_instance;

HangOnFull::HangOnFull()
{
	myClass = string(typeid(*this).name());
	xapp().registerApp(myClass, this);
}

HangOnFull::~HangOnFull()
{
}

void HangOnFull::init()
{
	initScenes();
}

void HangOnFull::update()
{
	logo->update();
}

void HangOnFull::draw()
{
	logo->draw();
}

void HangOnFull::destroy()
{
}

string HangOnFull::getWindowTitle()
{
	return "Hang On";
}

void HangOnFull::initScenes()
{
	Camera b = xapp().camera;
	b.pos.z = -4000.0f;
	for (long i = 0; i < 100000; i++)
	{
		Camera old = xapp().camera;
		xapp().camera = b;
		xapp().camera.pos.z -= 1.0f;
		xapp().camera = old;
	}
	logo = xapp().getApp("Logo");
	hangon = xapp().getApp("HangOn");
	this->apps.push_back(logo);
	this->apps.push_back(hangon);
	this->initAllApps();
}