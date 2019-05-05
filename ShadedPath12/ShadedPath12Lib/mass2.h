#pragma once
#include "xapp.h"
class MassTest2 :	public XAppBase
{
public:
	MassTest2();
	virtual ~MassTest2();

	void init();
	void update();
	void draw();
	void destroy();
	string getWindowTitle();
	void initMeteorField();

private:
	// Engine classes:
	GameTime gameTime;
	// used effects:
	LinesEffect linesEffect;
	Dotcross dotcrossEffect;
	Linetext textEffect;
	PostEffect postEffect;
	Billboard billboardEffect;
	//WorldObjectEffect objectEffect;
	// other:
	LONGLONG startTime;
	int framenumLine, fpsLine;  // indexes into the text array for Linetext effect
	bool textureFullFrameTest = false;
	//MeshObject object;
	float globalAmbientLightLevel;
	float globalDirectionalLightLevel;
	XMFLOAT4 dirColor1, dirColor2;
	MeshObjectStore *objStore = nullptr;
};

