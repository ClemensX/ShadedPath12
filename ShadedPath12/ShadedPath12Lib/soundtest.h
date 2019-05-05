#pragma once
#include "xapp.h"
class Soundtest :	public XAppBase
{
public:
	Soundtest();
	virtual ~Soundtest();

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
	WorldObjectEffect objectEffect;
	// other:
	WorldObject object;
	LONGLONG startTime;
	int framenumLine, fpsLine;  // indexes into the text array for Linetext effect
};

