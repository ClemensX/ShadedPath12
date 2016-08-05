#pragma once
#include "xapp.h"
class Logo : public XAppBase
{
public:
	Logo();
	virtual ~Logo();

	void init();
	void update();
	void draw();
	void destroy();
	string getWindowTitle();

private:
	// Engine classes:
	GameTime gameTime;
	// used effects:
	PostEffect postEffect;
	WorldObjectEffect objectEffect;
	// other:
	WorldObject logo;
	LONGLONG startTime;
};

