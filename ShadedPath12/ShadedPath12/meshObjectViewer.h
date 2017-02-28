#pragma once
#include "xapp.h"
class MeshObjectViewer :	public XAppBase
{
public:
	MeshObjectViewer();
	virtual ~MeshObjectViewer();

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
	//WorldObjectEffect objectEffect;
	// other:
	LONGLONG startTime;
	int framenumLine, fpsLine;  // indexes into the text array for Linetext effect
	bool textureFullFrameTest = false;
	MeshObject object;
	float globalAmbientLightLevel;
	float globalDirectionalLightLevel;
	XMFLOAT4 dirColor1, dirColor2;
};

