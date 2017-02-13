#include "stdafx.h"
#include "hangon.h"

#if (defined(_DEBUG))
static int NUM_METEOR = 500;
#else
static int NUM_METEOR = 2000;
#endif
static int NUM_THREADS = 4; // 4

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
	objectEffect.init(&xapp().objectStore, NUM_THREADS, NUM_METEOR + 19); // TODO: 17 triggers assert

	// initialize game time to real time:
	gameTime.init(1);
	startTime = gameTime.getRealTime();

	//xapp().setBackgroundColor(Colors::Black);
	xapp().setBackgroundColor(XMFLOAT4(0.0021973f, 0.0021973f, 0.0021973f, 1.0f));   // prevent strange smearing effect for total black pixels (only in HMD)

	float textSize = 0.5f;
	float lineHeight = 2 * textSize;
	xapp().camera.nearZ = 0.2f;
	xapp().camera.farZ = 2000.0f;
	xapp().camera.pos = XMFLOAT4(0.0f, 0.0f, -100.0f, 0.0f);
	//xapp().camera.setSpeed(1.0f); // seems ok for VR
	xapp().camera.setSpeed(10.5f); // faster for dev usability
	xapp().camera.fieldOfViewAngleY = 1.289f;
	xapp().world.setWorldSize(2048.0f, 782.0f, 2048.0f);

#define TEST_MUTI_XAPPS
#if defined(TEST_MUTI_XAPPS)
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
#if defined _DEBUG
	ambLvl *= 10;
#endif
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
	lights->directionalLights[0].color = lightControl.factor(1.0f, dirColor1);
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

	xapp().textureStore.loadTexture(L"mars_6k_color.dds", "mars");
	//xapp().textureStore.loadTexture(L"mars_12k_color.dds", "planet");
	TextureInfo *MarsTex = xapp().textureStore.getTexture("mars");
	xapp().objectStore.loadObject(L"sphere3.b", "Mars", 100);
	//xapp().objectStore.addObject(mars, "Mars", XMFLOAT3(2000.0f, -300.0f, -700.0f), MarsTex);
	xapp().objectStore.addObject(mars, "Mars", XMFLOAT3(0.0f, 0.0f, 0.0f), MarsTex);
	mars.rot().x = 0.1f;
	mars.rot().y = 0.1f;
	mars.rot().z = 0.1f;
	//object.drawNormals = true;
	mars.material.ambient = XMFLOAT4(1, 1, 1, 1);
	mars.material.specExp = 200.0f;       // no spec color 1 0 nothing
	mars.material.specIntensity = 1000.0f; // no spec color
	mars.material.specExp = 1.0f;       // no spec color 1 0 nothing
	mars.material.specIntensity = 0.0f; // no spec color
	vector<XMFLOAT4> points;
	points.push_back(XMFLOAT4(2000.0f, -300.0f, -700.0f, 1.0));//start
	points.push_back(XMFLOAT4(1995.0f, 100.0f, -700.0f, 1.0));
	points.push_back(XMFLOAT4(1990.0f, 200.0f, -700.0f, 1.0));
	points.push_back(XMFLOAT4(1980.0f, 300.0f, -700.0f, 1.0));
	points.push_back(XMFLOAT4(1970.0f, 400.0f, -700.0f, 1.0));
	points.push_back(XMFLOAT4(1950.0f, 500.0f, -700.0f, 1.0));
	points.push_back(XMFLOAT4(1925.0f, 600.0f, -700.0f, 1.0));
	points.push_back(XMFLOAT4(1895.0f, 700.0f, -700.0f, 1.0));
	points.push_back(XMFLOAT4(1855.0f, 800.0f, -700.0f, 1.0));
	points.push_back(XMFLOAT4(1800.0f, 900.0f, -700.0f, 1.0));
	points.push_back(XMFLOAT4(1750.0f, 1000.0f, -700.0f, 1.0)); //10
	points.push_back(XMFLOAT4(1660.0f, 1100.0f, -700.0f, 1.0));
	points.push_back(XMFLOAT4(1400.0f, 1200.0f, -700.0f, 1.0));
	points.push_back(XMFLOAT4(1300.0f, 1250.0f, -700.0f, 1.0));//mid
	points.push_back(XMFLOAT4(800.0f, 1250.0f, -700.0f, 1.0));//end

	auto &path = xapp().world.path;
	path.adjustTimings(points, 120.0f);
	path.defineAction("marsmove", mars, points);
	mars.setAction("marsmove");
	mars.pathDescMove->pathMode = Path_SimpleMode;
	mars.pathDescMove->starttime_f = gameTime.getTimeAbsSeconds();
	mars.pathDescMove->handleRotation = false;

	xapp().textureStore.loadTexture(L"2create_brick_0001.dds", "brickwall");
	//xapp().textureStore.loadTexture(L"mars_12k_color.dds", "planet");
	TextureInfo *BrickwallTex = xapp().textureStore.getTexture("brickwall");
	xapp().objectStore.loadObject(L"brickwall.b", "Brickwall");
	auto cp = xapp().camera.pos;
	XMFLOAT3 brickpos = XMFLOAT3(cp.x, cp.y - 1.85f, cp.z);
	xapp().objectStore.addObject(brickwall, "Brickwall", brickpos, BrickwallTex);
	//object.drawNormals = true;
	brickwall.material.ambient = XMFLOAT4(1, 1, 1, 1);
	brickwall.material.specExp = 200.0f;       // no spec color 1 0 nothing
	brickwall.material.specIntensity = 1000.0f; // no spec color
	brickwall.material.specExp = 1.0f;       // no spec color 1 0 nothing
	brickwall.material.specIntensity = 0.0f; // no spec color

	// stereo background music and mono sound from world object
	xapp().sound.openSoundFile(L"Wind-Mark_DiAngelo-1940285615.wav", "background_sound", true);
	xapp().sound.playSound("background_sound", SoundCategory::EFFECT);  // cleanup volume/type
	//xapp().sound.lowBackgroundMusicVolume();
#endif // Multi XApps
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
		XMFLOAT3 p = xapp().world.getRandomPos(-50);
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
	//LONGLONG now = gameTime.getRealTime();
	double nowf = gameTime.getTimeAbsSeconds();
	double dt = gameTime.getDeltaTime(); // seconds since last frame
	static bool done = false;
	xapp().lights.update();
	//linesEffect.update();
	//dotcrossEffect.update();

	// update info text:
	string fps_str("FPS ");
	stringstream sss;
	sss << xapp().fps;
	fps_str.append(sss.str());
	textEffect.changeTextLine(fpsLine, fps_str);
	//textEffect.update();

	object.update();
	auto grp = xapp().objectStore.getGroup("meteor");
	// update meteor position:
	if (movingMeteorOn == false) {
		movingMeteorOn = true;
		bool nearMe = MathHelper::RandF() > 0.7f;  // percentage of meteors that go near me
		vector<XMFLOAT4> points;
		auto start = xapp().world.getRandomPos(50);
		start.y = 1000;
		points.push_back(XMFLOAT4(start.x, start.y, start.z, 1.0));
		if (nearMe) {
			auto end = xapp().camera.pos;
			end.y = -100;
			points.push_back(XMFLOAT4(end.x, end.y, end.z, 250.0));
		} else {
			auto end = xapp().world.getRandomPos(50);
			end.y = -1000;
			points.push_back(XMFLOAT4(end.x, end.y, end.z, 250.0));
		}
		/*		float rx = MathHelper::RandF(0.0f, XM_PI * 2.0f);
		w.get()->rot().x = rx;
		float ry = MathHelper::RandF(0.0f, XM_PI * 2.0f);
		w.get()->rot().y = ry;
		float rz = MathHelper::RandF(0.0f, XM_PI * 2.0f);
		w.get()->rot().z = rz;
*/
		float rx = MathHelper::RandF(0.0f, XM_PI * 2.0f);
		movingMeteor.rot().x = rx;
		float ry = MathHelper::RandF(0.0f, XM_PI * 2.0f);
		movingMeteor.rot().y = ry;
		float rz = MathHelper::RandF(0.0f, XM_PI * 2.0f);
		movingMeteor.rot().z = rz;
		//vector<XMFLOAT4> points = { { 0.0f, 0.0f, 0.0f, 1.0f },{ 500.0f, 500.0f, 500.0f, 80.0f },{ 500.0f, 500.0f, 1000.0f, 160.0f } };
		//vector<XMFLOAT4> points = { { 500.0f, 500.0f, 500.0f, 1.0f },{ 500.0f, 500.0f, 1000.0f, 80.0f },{ 0.0f, 0.0f, 0.0f, 160.0f } };
		auto &path = xapp().world.path;
		path.defineAction("movetest", movingMeteor, points);
		movingMeteor.setAction("movetest");
		movingMeteor.pathDescMove->pathMode = Path_SimpleMode;
		movingMeteor.pathDescMove->starttime_f = nowf;
		movingMeteor.pathDescMove->handleRotation = false;
	}
	else {
		CBVLights *lights = &xapp().lights.lights;
		XMFLOAT4 lpos = XMFLOAT4(movingMeteor.pos().x, movingMeteor.pos().y, movingMeteor.pos().z, 1.0f);
		lights->pointLights[0].pos = lpos;
		if (movingMeteor.pathDescMove->isLastPos) {
			movingMeteorOn = false;
		}
	}
	double fullturn_sec = 1000.0;
	double turnfrac = fmod(nowf, fullturn_sec)/fullturn_sec;  // 0.0 .. 1.0
	mars.rot().x = turnfrac * XM_2PI;
	fullturn_sec *= 2.0; // half rotation speed vertically
	turnfrac = fmod(nowf, fullturn_sec) / fullturn_sec;  // 0.0 .. 1.0
	mars.rot().y = turnfrac * XM_2PI;
	xapp().sound.Update();
}
/*
XMFLOAT3 planetPos[] = {
XMFLOAT3(2000.0f, -300.0f, -700.0f),//start
XMFLOAT3(1995.0f, 100.0f, -700.0f),
XMFLOAT3(1990.0f, 200.0f, -700.0f),
XMFLOAT3(1980.0f, 300.0f, -700.0f),
XMFLOAT3(1970.0f, 400.0f, -700.0f),
XMFLOAT3(1950.0f, 500.0f, -700.0f),
XMFLOAT3(1925.0f, 600.0f, -700.0f),
XMFLOAT3(1895.0f, 700.0f, -700.0f),
XMFLOAT3(1855.0f, 800.0f, -700.0f),
XMFLOAT3(1800.0f, 900.0f, -700.0f),
XMFLOAT3(1750.0f, 1000.0f, -700.0f), //10
XMFLOAT3(1660.0f, 1100.0f, -700.0f),
XMFLOAT3(1400.0f, 1200.0f, -700.0f),
XMFLOAT3(1300.0f, 1250.0f, -700.0f),//mid
XMFLOAT3(800.0f, 1250.0f, -700.0f)//end
};
PathDesc paths[] = {
//{ 2.0f*SpeedFactor },
//{}
//speed, pos, look, cur segment, num segments, starttime
{ 2.0f*SpeedFactor, &titlePos[0], NULL, 0, 4, 0L },
{ 1.5f*SpeedFactor, &camIntroPos[0], NULL, 0, 2, 0L },
{ 0.5f*SpeedFactor, &planetPos[0], NULL, 0, 14, 0L }
//	{0.4f, &titlePos[0], NULL, 0, 4, 0L},
//	{0.3f, &camIntroPos[0], NULL, 0, 2, 0L}
};

// object movement
XMFLOAT3* objPos = &xapp->world.path.getPos(PATHID_PLANET, xapp->gametime.getRealTime(), 0);
planet.pos() = *objPos;
planet.rot().x += 0.0002f;
planet.rot().y += 0.00009f;
planet.update();

*/
void HangOn::draw()
{
	linesEffect.draw();
	//dotcrossEffect.draw();
	//textEffect.draw();

	if (movingMeteorOn) {
		static XMFLOAT4 dirColor1 = XMFLOAT4(0.980f, 0.910f, 0.723f, 1.0f);
		CBVLights *lights = &xapp().lights.lights;
		XMFLOAT4 currentAmbientLight = lights->ambientLights[0].ambient;// = XMFLOAT4(ambLvl, ambLvl, ambLvl, 1);
		lights->ambientLights[0].ambient = dirColor1;//XMFLOAT4(1.0f, 1.0f, 1.0f, 1);
		movingMeteor.draw();
		lights->ambientLights[0].ambient = currentAmbientLight;
	}
	{
		static XMFLOAT4 dirColor1 = XMFLOAT4(0.980f, 0.910f, 0.723f, 1.0f);
		CBVLights *lights = &xapp().lights.lights;
		XMFLOAT4 currentAmbientLight = lights->ambientLights[0].ambient;// = XMFLOAT4(ambLvl, ambLvl, ambLvl, 1);
		//lights->ambientLights[0].ambient = dirColor1;//XMFLOAT4(1.0f, 1.0f, 1.0f, 1);
		lights->directionalLights[0].used_fill.x = 1.0f;
		mars.draw();
		brickwall.draw();
		lights->directionalLights[0].used_fill.x = 0.0f;
		lights->ambientLights[0].ambient = currentAmbientLight;
	}
	// optimization: draw whole group (objects with same mesh)
	xapp().objectStore.drawGroup("meteor", NUM_THREADS); // TODO
	//xapp().objectStore.drawGroup("meteor", 0); // use for bulk update w/o threads

	postEffect.draw();
}

void HangOn::destroy()
{
}

static HangOn hangon;