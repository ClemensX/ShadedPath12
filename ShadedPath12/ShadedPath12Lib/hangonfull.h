#pragma once
#include "xapp.h"
class HangOnFull : public XAppBase, XAppMultiBase
{
public:
	HangOnFull();
	virtual ~HangOnFull();

	void init();
	void update();
	void draw();
	void destroy();
	string getWindowTitle();

	void initScenes();

private:
	// all apps used by this multi app:
	XAppBase *logo, *hangon;
};

