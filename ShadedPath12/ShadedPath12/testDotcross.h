#pragma once
#include "xapp.h"
class TestDotcross :	public XAppBase
{
public:
	TestDotcross();
	virtual ~TestDotcross();

	void init();
	void update();
	void draw();
	void destroy();
	string getWindowTitle();

private:
	// Engine classes:
	GameTime gameTime;
	// used effects:
	LinesEffect linesEffect;
	Dotcross dotcrossEffect;
	PostEffect postEffect;
	// other:
	LONGLONG startTime;
};

