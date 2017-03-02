#include "stdafx.h"
#include "soundtest.h"


Soundtest::Soundtest() : XAppBase()
{
	myClass = string(typeid(*this).name());
	xapp().registerApp(myClass, this);
}


Soundtest::~Soundtest()
{
}

string Soundtest::getWindowTitle() {
	return "Soundtest";
}

void Soundtest::init()
{
	dotcrossEffect.init();
	linesEffect.init();
	textEffect.init();
	postEffect.init();
	objectEffect.init(&xapp().objectStore, 1);
	float aspectRatio = xapp().aspectRatio;

	LineDef myLines[] = {
		// start, end, color
		{ XMFLOAT3(3.0f, 0.25f * aspectRatio, 0.0f), XMFLOAT3(3.25f, -0.25f * aspectRatio, 0.0f), XMFLOAT4(4.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(3.25f, -0.25f * aspectRatio, 0.0f), XMFLOAT3(2.75f, -0.25f * aspectRatio, 0.0f), XMFLOAT4(3.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(2.75f, -0.25f * aspectRatio, 0.0f), XMFLOAT3(3.0f, 0.25f * aspectRatio, 0.0f), XMFLOAT4(3.0f, 0.0f, 1.0f, 1.0f) }
	};
	vector<LineDef> lines;
	// add all intializer objects to vector:
	for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l);});
	linesEffect.add(lines);
	// initialize game time to real time:
	gameTime.init(1);
	startTime = gameTime.getRealTime();

	float textSize = 0.5f;
	float lineHeight = 2 * textSize;
	xapp().camera.nearZ = 0.2f;
	xapp().camera.farZ = 2000.0f;
	//xapp->camera.pos.z = -3.0f;
	//xapp().camera.pos = XMFLOAT4(1.0f, 1.7f, 1.0f, 0.0f);
	xapp().camera.pos = XMFLOAT4(0.0f, 0.0f, -3.0f, 0.0f);
	xapp().camera.pos = XMFLOAT4(-0.0696329f, 0.354773f, 0.324679f, 0.0f);
	//xapp().camera.setSpeed(1.0f); // seems ok for VR
	xapp().camera.setSpeed(10.5f); // faster for dev usability
	xapp().camera.fieldOfViewAngleY = 1.289f;
	xapp().world.setWorldSize(2048.0f, 382.0f, 2048.0f);

	textEffect.setSize(textSize);
	dotcrossEffect.setLineLength(6.0f * textSize);
	textEffect.addTextLine(XMFLOAT4(-5.0f, 6 * lineHeight, 0.0f, 0.0f), xapp().buildInfo, Linetext::XY);
	textEffect.addTextLine(XMFLOAT4(-5.0f, 5 * lineHeight, 0.0f, 0.0f), "Background music by Niklas Fehr", Linetext::XY);
	fpsLine = textEffect.addTextLine(XMFLOAT4(-5.0f, 4 * lineHeight, 0.0f, 0.0f), "FPS", Linetext::XY);
	framenumLine = textEffect.addTextLine(XMFLOAT4(-5.0f, 3 * lineHeight, 0.0f, 0.0f), "0123456789", Linetext::XY);

	Grid *g = xapp().world.createWorldGrid(10.0f, -1.65f);
	linesEffect.add(g->lines);

	xapp().textureStore.loadTexture(L"worm1.dds", "worm");
	TextureInfo *WormTex = xapp().textureStore.getTexture("worm");
	xapp().lights.init();
	object.material.ambient = XMFLOAT4(1, 1, 1, 1);

	xapp().objectStore.loadObject(L"worm5.b", "Worm");
	xapp().objectStore.addObject(object, "Worm", XMFLOAT3(10.0f, 10.0f, 10.0f), WormTex);
	object.setAction("Armature");
	object.pathDescBone->pathMode = Path_Loop;
	object.pathDescBone->speed = 3.0;
	object.forceBoundingBox(BoundingBox(XMFLOAT3(0.0171146f, 2.33574f, -0.236285f), XMFLOAT3(1.29998f, 2.3272f, 9.97486f)));
	object.material.specExp = 100.0f;       // no spec color 1 0 nothing
	object.material.specIntensity = 10.0f; // no spec color

	CBVLights *lights = &xapp().lights.lights;
	auto &lightControl = xapp().lights;
	float f = 1.0f;
	lights->ambientLights[0].ambient = XMFLOAT4(f, f, f, 1);

	xapp().world.drawCoordinateSystem(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), "Origin", textEffect, dotcrossEffect, textSize);

	// stereo game sound from world object
	//xapp->sound.openSoundFile(L"Shaded_Path_Game_Music.wav", "intro_music", true);
	//xapp->sound.playSound("intro_music");
	//pathObject.soundDef = &xapp->sound.sounds["intro_music"];
	//xapp->sound.addWorldObject(&pathObject, nullptr);

	// mono game sound from world object
	//xapp->sound.openSoundFile(L"worm_move.wav", "worm_move", true);
	//xapp->sound.playSound("worm_move");
	//pathObject.soundDef = &xapp->sound.sounds["worm_move"];
	//xapp->sound.addWorldObject(&pathObject, nullptr);

	// stereo background music and mono sound from world object
	xapp().sound.openSoundFile(L"Shaded_Path_Game_Music.wav", "intro_music", true);
	xapp().sound.playSound("intro_music", SoundCategory::MUSIC);
	xapp().sound.lowBackgroundMusicVolume();
	// mono game sound from world object
	xapp().sound.openSoundFile(L"worm_move.wav", "worm_move", true);
	xapp().sound.playSound("worm_move");
	object.soundDef = &xapp().sound.sounds["worm_move"];
	object.maxListeningDistance = 20;
	xapp().sound.addWorldObject(&object, nullptr);
}

void Soundtest::update()
{
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
	static bool done = false;
	if (!done && gameTime.getSecondsBetween(startTime, now) > 3) {
		done = true;
		//if (xapp().pGraphicsAnalysis != nullptr) xapp().pGraphicsAnalysis->BeginCapture();
		//Sleep(1000);

		float aspectRatio = xapp().aspectRatio;
		LineDef myLines[] = {
			// start, end, color
			{ XMFLOAT3(3.1f, 0.15f * aspectRatio, 0.0f), XMFLOAT3(3.15f, -0.35f * aspectRatio, 0.0f), XMFLOAT4(4.0f, 0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(3.15f, -0.35f * aspectRatio, 0.0f), XMFLOAT3(2.85f, -0.35f * aspectRatio, 0.0f), XMFLOAT4(4.0f, 0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(2.85f, -0.35f * aspectRatio, 0.0f), XMFLOAT3(3.1f, 0.15f * aspectRatio, 0.0f), XMFLOAT4(4.0f, 0.0f, 0.0f, 1.0f) }
		};
		// add all intializer objects to vector:
		vector<LineDef> lines;
		for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l);});
		linesEffect.add(lines);
	}
	linesEffect.update();
	dotcrossEffect.update();
	// update info text:
	string fr("Frame ");
	stringstream ss;
	ss << xapp().getFramenum();
	fr.append(ss.str());
	textEffect.changeTextLine(framenumLine, fr);

	string fps_str("FPS ");
	stringstream sss;
	sss << xapp().fps;
	fps_str.append(sss.str());
	textEffect.changeTextLine(fpsLine, fps_str);
	textEffect.update();
	object.update();
	xapp().sound.Update();
}

void Soundtest::draw()
{
	linesEffect.draw();
	dotcrossEffect.draw();
	textEffect.draw();
	object.draw();
	postEffect.draw();
}

void Soundtest::destroy()
{
	//linesEffect.destroy();
}

static Soundtest soundtest;