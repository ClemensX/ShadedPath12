#include "stdafx.h"
#include "hangon.h"

#if (defined(_DEBUG))
static int NUM_METEOR = 500;
#else
static int NUM_METEOR = 1500;
#endif
static int NUM_THREADS = 2;

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
	objectEffect.init(&xapp().objectStore, NUM_THREADS, NUM_METEOR + 10);

	// initialize game time to real time:
	gameTime.init(1);
	startTime = gameTime.getRealTime();

	//xapp().setBackgroundColor(Colors::Black);
	xapp().setBackgroundColor(XMFLOAT4(0.0021973f, 0.0021973f, 0.0021973f, 1.0f));   // prevent strange smearing effect for total black pixels (only in HMD)

	float textSize = 0.5f;
	float lineHeight = 2 * textSize;
	xapp().camera.nearZ = 0.2f;
	xapp().camera.farZ = 2000.0f;
	xapp().camera.pos = XMFLOAT4(0.0f, 0.0f, -3.0f, 0.0f);
	//xapp().camera.setSpeed(1.0f); // seems ok for VR
	xapp().camera.setSpeed(10.5f); // faster for dev usability
	xapp().camera.fieldOfViewAngleY = 1.289f;
	xapp().world.setWorldSize(2048.0f, 782.0f, 2048.0f);

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

	// ambient light
	float ambLvl = 0.04f;
	//ambLvl *= 10;
	lights->ambientLights[0].ambient = XMFLOAT4(ambLvl, ambLvl, ambLvl, 1);
	assert(0 < MAX_AMBIENT);
	//globalAmbientLightLevel = 0.3f;
	//globalDirectionalLightLevel = 1.0f;
	//globalAmbientLightLevel = 0.0f;
	//globalDirectionalLightLevel = 0.0f;

	// directional lights:
	XMFLOAT4 dirColor1 = XMFLOAT4(0.980f, 0.910f, 0.723f, 1.0f);
	//dirColor2 = XMFLOAT4(0.6f, 0.4f, 0.6f, 1.0f);
	auto &lightControl = xapp().lights;
	//lights->directionalLights[0].color = dirColor1;
	lights->directionalLights[0].color = lightControl.factor(0.1f, dirColor1);
	//lights->directionalLights[0].color = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	lights->directionalLights[0].pos = XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f);
	lights->directionalLights[0].used_fill.x = 0.0f;

	// point lights:
	lights->pointLights[0].color = dirColor1;
	lights->pointLights[0].pos = XMFLOAT4(6.0f, 250.0f, 25.0f, 1.0f);
	lights->pointLights[0].range_reciprocal = 1.0f / 300.0f;
	lights->pointLights[0].used = 1.0f;

	xapp().world.drawCoordinateSystem(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), "Origin", textEffect, dotcrossEffect, textSize);
	//initStarfield(10000, 300.0f);
	initMeteorField();
	TextureInfo *Meteor1Tex = xapp().textureStore.getTexture("meteor1");
	xapp().objectStore.addObject(movingMeteor, "Meteor1", XMFLOAT3(10.0f, 30.0f, -70.0f), Meteor1Tex);
	//object.drawNormals = true;
	movingMeteor.material.ambient = XMFLOAT4(1, 1, 1, 1);
	movingMeteor.material.specExp = 200.0f;       // no spec color 1 0 nothing
	movingMeteor.material.specIntensity = 1000.0f; // no spec color
	movingMeteor.material.specExp = 1.0f;       // no spec color 1 0 nothing
	movingMeteor.material.specIntensity = 0.0f; // no spec color
	//xapp().world.

	// stereo background music and mono sound from world object
	xapp().sound.openSoundFile(L"Wind-Mark_DiAngelo-1940285615.wav", "background_sound", true);
	xapp().sound.playSound("background_sound", SoundCategory::EFFECT);  // cleanup volume/type
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

void HangOn::initMeteorField() {
	//xapp().textureStore.loadTexture(L"dirt6_markings.dds", "default");
	xapp().textureStore.loadTexture(L"met1.dds", "meteor1");
	TextureInfo *Meteor1Tex = xapp().textureStore.getTexture("meteor1");
	//xapp().objectStore.loadObject(L"house4_anim.b", "House");
	xapp().objectStore.loadObject(L"meteor_single.b", "Meteor1");
	//xapp().objectStore.addObject(object, "Planet", XMFLOAT3(700.0f, 300.0f, -700.0f), PlanetTex);
	//xapp().objectStore.addObject(object, "Meteor1", XMFLOAT3(10.0f, 30.0f, -70.0f), Meteor1Tex);

	xapp().objectStore.createGroup("meteor");
	for (int i = 0; i < NUM_METEOR; i++) {
		XMFLOAT3 p = xapp().world.getRandomPos(50);
		//p.x = p.y = p.z = 0.0f;
		//p.x = i * 10.0f;
		xapp().objectStore.addObject("meteor", "Meteor1", p, Meteor1Tex);
	}
    //object.drawBoundingBox = true;
	//object.drawNormals = true;
	//object.setAction("Cube");
	//object.pathDescMove->pathMode = Path_Reverse;
	// update meteor data:
	auto grp = xapp().objectStore.getGroup("meteor");
	for (auto & w : *grp) {
		w.get()->material.specExp = 1.0f;       // no spec color
		w.get()->material.specIntensity = 0.0f; // no spec color
		w.get()->material.ambient = XMFLOAT4(1, 1, 1, 1);
		float rx = MathHelper::RandF(0.0f, XM_PI * 2.0f);
		w.get()->rot().x = rx;
		float ry = MathHelper::RandF(0.0f, XM_PI * 2.0f);
		w.get()->rot().y = ry;
		float rz = MathHelper::RandF(0.0f, XM_PI * 2.0f);
		w.get()->rot().z = rz;
	}
}

void HangOn::update()
{
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
	static bool done = false;
	xapp().lights.update();
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
	auto grp = xapp().objectStore.getGroup("meteor");
	// update meteor position:
	if (movingMeteorOn == false) {
		movingMeteorOn = true;
		auto &path = xapp().world.path;
		vector<XMFLOAT4> points = { { 0.0f, 0.0f, 0.0f, 1.0f },{ 500.0f, 500.0f, 500.0f, 80.0f },{ 500.0f, 500.0f, 1000.0f, 160.0f } };
		path.defineAction("movetest", movingMeteor, points);
		movingMeteor.setAction("movetest");
	}
	else {
		CBVLights *lights = &xapp().lights.lights;
		XMFLOAT4 lpos = XMFLOAT4(movingMeteor.pos().x, movingMeteor.pos().y, movingMeteor.pos().z, 1.0f);
		lights->pointLights[0].pos = lpos;
	}
	xapp().sound.Update();
}

void HangOn::draw()
{
	linesEffect.draw();
	//dotcrossEffect.draw();
	//textEffect.draw();

	if (movingMeteorOn) movingMeteor.draw();
	// optimization: draw whole group (objects with same mesh)
	xapp().objectStore.drawGroup("meteor", NUM_THREADS); // TODO
	//xapp().objectStore.drawGroup("meteor", 0); // use for bulk update w/o threads

	postEffect.draw();
}

void HangOn::destroy()
{
}

static HangOn soundtest;