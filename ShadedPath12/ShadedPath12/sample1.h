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
	void destroy();
	string getWindowTitle();

private:
	// Engine classes:
	GameTime gameTime;
	// used effects:
	CopyTextureEffect copyTextureEffect;
	ClearEffect clearEffect;
	//LinesEffect linesEffect;
	//Dotcross dotcrossEffect;
	//Linetext textEffect;
	//PostEffect postEffect;
	// other:
	LONGLONG startTime;
	int absFrameCountLine, fpsLine;  // indexes into the text array for Linetext effect
};

