#pragma once
#include "xapp.h"
class TouchOdyssey :	public XAppBase
{
public:
	TouchOdyssey();
	virtual ~TouchOdyssey();

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
	WorldObjectEffect objectEffect;
	PostEffect postEffect;
	// other:
	LONGLONG startTime;
	int framenumLine, fpsLine;  // indexes into the text array for Linetext effect
	bool textureFullFrameTest = false;
	WorldObject bigRC;  // big right controller (static - not animated)
	WorldObject spinRC, spinLC;  // small left and right controller (static - not animated)
	float globalAmbientLightLevel;
	float globalDirectionalLightLevel;
	XMFLOAT4 dirColor1, dirColor2;
	bool loadAvatarAssetsFromOculus = false; // true triggers loading all meshes and texures and saving to local files
};

