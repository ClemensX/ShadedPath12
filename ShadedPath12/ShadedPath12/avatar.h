#pragma once
#include "xapp.h"
class Avatar :	public XAppBase
{
public:
	Avatar();
	virtual ~Avatar();

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
	float globalAmbientLightLevel;
	float globalDirectionalLightLevel;
	XMFLOAT4 dirColor1, dirColor2;

	void loadLocalAvatarMeshes(wstring userId);
	bool avatarMeshesLoadFinished = false;
	bool avatarMeshesLoadStartet = false;
	bool loadAvatarAssetsFromOculus = false; // true triggers loading all meshes and texures and saving to local files
};

