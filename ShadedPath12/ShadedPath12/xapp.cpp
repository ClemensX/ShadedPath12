#include "stdafx.h"
#include "xapp.h"

XAppBase::XAppBase() {
	//Log("XAppBase registers " << this->getWindowTitle().c_str() << endl);
	Log("XAppBase registers " << myClass.c_str() << endl);
}

XAppBase::~XAppBase() {

}

void XAppBase::init() {

}

void XAppBase::update() {

}

void XAppBase::draw() {

}
