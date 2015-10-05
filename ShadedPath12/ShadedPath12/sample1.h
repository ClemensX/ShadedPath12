#pragma once
#include "xapp.h"
class Sample1 :	public XAppBase
{
public:
	Sample1();
	virtual ~Sample1();

	void init();
	void update();
	void draw();
	void next();
	void destroy();
	string getWindowTitle();

private:
	// Engine classes:
	GameTime gameTime;
	// used effects:
	LinesEffect linesEffect;
	// other:
	LONGLONG startTime;
};

