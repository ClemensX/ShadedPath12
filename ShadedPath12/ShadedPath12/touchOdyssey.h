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
	void enableSpinLight(bool enable, WorldObject *o);
	void enableMovement(bool enable, WorldObject *o, double nowf);

private:
	// Engine classes:
	GameTime gameTime;
	// used effects:
	LinesEffect linesEffect;
	Linetext textEffect;
	WorldObjectEffect objectEffect;
	PostEffect postEffect;
	// other:
	LONGLONG startTime;
	int framenumLine, fpsLine;     // indexes into the text array for Linetext effect
	bool textureFullFrameTest = false;
	WorldObject bigRC;             // big right controller (static - not animated, stand-on plattform)
	WorldObject spinRC, spinLC;    // small left and right controller (static - not animated)
	WorldObject ghostRC, ghostLC;  // ghost images of controllers to indicate bond point with hands
	float globalAmbientLightLevel;
	float globalDirectionalLightLevel;
	XMFLOAT4 dirColor1, dirColor2;
	bool loadAvatarAssetsFromOculus = false; // true triggers loading all meshes and texures and saving to local files
	bool isMovingRC = false;
	bool isMovingRCFinished = false;
	bool isTurningRC = false;
	bool isTurningRCFinished = false;
	bool isBondedRC = false;
};

