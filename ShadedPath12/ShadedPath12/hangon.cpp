#include "stdafx.h"
#include "hangon.h"


HangOn::HangOn() : XAppBase()
{
	myClass = string(typeid(*this).name());
	xapp().registerApp(myClass, this);
}


HangOn::~HangOn()
{
}

string HangOn::getWindowTitle() {
	return "Hang On";
}

void HangOn::init()
{
	dotcrossEffect.init();
	linesEffect.init();
	textEffect.init();
	postEffect.init();
	objectEffect.init(&xapp().objectStore);

	// initialize game time to real time:
	gameTime.init(1);
	startTime = gameTime.getRealTime();

	float textSize = 0.5f;
	float lineHeight = 2 * textSize;
	xapp().camera.nearZ = 0.2f;
	xapp().camera.farZ = 2000.0f;
	xapp().camera.pos = XMFLOAT4(0.0f, 0.0f, -3.0f, 0.0f);
	//xapp().camera.setSpeed(1.0f); // seems ok for VR
	xapp().camera.setSpeed(10.5f); // faster for dev usability
	xapp().camera.fieldOfViewAngleY = 1.289f;
	xapp().world.setWorldSize(2048.0f, 382.0f, 2048.0f);

	textEffect.setSize(textSize);
	dotcrossEffect.setLineLength(6.0f * textSize);
	textEffect.addTextLine(XMFLOAT4(-5.0f, -6 * lineHeight, 0.0f, 0.0f), xapp().buildInfo, Linetext::XY);
	textEffect.addTextLine(XMFLOAT4(-5.0f, -5 * lineHeight, 0.0f, 0.0f), "Background music by Niklas Fehr", Linetext::XY);
	fpsLine = textEffect.addTextLine(XMFLOAT4(-5.0f, -4 * lineHeight, 0.0f, 0.0f), "FPS", Linetext::XY);

	xapp().textureStore.loadTexture(L"grassdirt8.dds", "grass");
	TextureInfo *GrassTex = xapp().textureStore.getTexture("grass");
	xapp().lights.init();
	object.material.ambient = XMFLOAT4(1, 1, 1, 1);

	xapp().objectStore.loadObject(L"shaded2.b", "Shaded");
	xapp().objectStore.addObject(object, "Shaded", XMFLOAT3(0.0f, 0.0f, 00.0f), GrassTex);
	//object.drawNormals = true;
	object.material.ambient = XMFLOAT4(1, 1, 1, 1);
	object.material.specExp = 200.0f;       // no spec color 1 0 nothing
	object.material.specIntensity = 1000.0f; // no spec color

	CBVLights *lights = &xapp().lights.lights;
	auto &lightControl = xapp().lights;
	float f = 1.0f;
	lights->ambientLights[0].ambient = XMFLOAT4(f, f, f, 1);

	xapp().world.drawCoordinateSystem(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), "Origin", textEffect, dotcrossEffect, textSize);
	initStarfield(10000, 300.0f);
	//xapp().world.

	// stereo background music and mono sound from world object
	xapp().sound.openSoundFile(L"Shaded_Path_Game_Music.wav", "intro_music", true);
	//xapp().sound.playSound("intro_music", SoundCategory::MUSIC);
	//xapp().sound.lowBackgroundMusicVolume();

/*	// mono game sound from world object
	xapp().sound.openSoundFile(L"worm_move.wav", "worm_move", true);
	xapp().sound.playSound("worm_move");
	object.soundDef = &xapp().sound.sounds["worm_move"];
	object.maxListeningDistance = 20.0f;
	xapp().sound.addWorldObject(&object, nullptr);
*/
}

void HangOn::initStarfield(int num, float minHeight)
{
	vector<XMFLOAT3> addCrossPoints;
	for (int i = 0; i < num; i++) {
		XMFLOAT3 rnd = xapp().world.getRandomPos(minHeight);
		addCrossPoints.push_back(rnd);
	}
	dotcrossEffect.update(addCrossPoints);
	Log("# crosses: " << dotcrossEffect.points.size() << endl);
}

void HangOn::update()
{
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
	static bool done = false;
	linesEffect.update();
	dotcrossEffect.update();

	// update info text:
	string fps_str("FPS ");
	stringstream sss;
	sss << xapp().fps;
	fps_str.append(sss.str());
	textEffect.changeTextLine(fpsLine, fps_str);
	textEffect.update();

	object.update();
	xapp().sound.Update();
}

void HangOn::draw()
{
	linesEffect.draw();
	dotcrossEffect.draw();
	textEffect.draw();
	object.draw();
	postEffect.draw();
}

void HangOn::destroy()
{
}

static HangOn soundtest;