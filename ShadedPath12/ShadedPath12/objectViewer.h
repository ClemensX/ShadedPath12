#pragma once
#include "xapp.h"
class ObjectViewer :	public XAppBase
{
public:
	ObjectViewer();
	virtual ~ObjectViewer();

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
	Billboard billboardEffect;
	WorldObjectEffect objectEffect;
	// other:
	LONGLONG startTime;
	int framenumLine, fpsLine;  // indexes into the text array for Linetext effect
	bool textureFullFrameTest = false;
	WorldObject object;
};

