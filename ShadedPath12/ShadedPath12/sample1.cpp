#include "stdafx.h"
#include "sample1.h"


Sample1::Sample1() : XAppBase()
{
	myClass = string(typeid(*this).name());
	xapp().registerApp(myClass, this);
}


Sample1::~Sample1()
{
}

string Sample1::getWindowTitle() {
	return "DX12 Sample";
}

void Sample1::init()
{
}

void Sample1::update()
{
}

void Sample1::draw()
{
}

static Sample1 sample1;