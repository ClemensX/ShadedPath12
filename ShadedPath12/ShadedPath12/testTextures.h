#pragma once
#include "xapp.h"
class TestTextures :	public XAppBase
{
public:
	TestTextures();
	virtual ~TestTextures();

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
	Linetext textEffect;
	PostEffect postEffect;
	// other:
	LONGLONG startTime;
	int framenumLine, fpsLine;  // indexes into the text array for Linetext effect
};

