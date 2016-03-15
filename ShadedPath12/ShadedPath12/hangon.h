#pragma once
#include "xapp.h"
class HangOn :	public XAppBase
{
public:
	HangOn();
	virtual ~HangOn();

	void init();
	// fixed number of stars, randomly anywhere in the world, but only eith at least the provided height
	void initStarfield(int num, float minHeight);
	void initMeteorField();
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

