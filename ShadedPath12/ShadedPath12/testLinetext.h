#pragma once
#include "xapp.h"
class TestLinetext :	public XAppBase
{
public:
	TestLinetext();
	virtual ~TestLinetext();

	void init();
	void update();
	void draw();
	void destroy();
	string getWindowTitle();

private:
	// Engine classes:
	GameTime gameTime;
	// used effects:
	Linetext textEffect;
	PostEffect postEffect;
	// other:
	LONGLONG startTime;
};

