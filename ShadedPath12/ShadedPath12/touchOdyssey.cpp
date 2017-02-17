#include "stdafx.h"
#include "touchOdyssey.h"

#include <filesystem>  // moved here from stdafx.h due to strange header compile error
using namespace std::tr2::sys;

// change to base name of meshes saved in data/mesh folder (part before _ )
#define USERID L"413fd8923c71e"


TouchOdyssey::TouchOdyssey() : XAppBase()
{
	myClass = string(typeid(*this).name());
	xapp().registerApp(myClass, this);
}


TouchOdyssey::~TouchOdyssey()
{
}

string TouchOdyssey::getWindowTitle() {
	return "Touch Odyssey";
}


void TouchOdyssey::init()
{
	postEffect.init();
	linesEffect.init();
	xapp().world.linesEffect = &linesEffect;
	textEffect.init();
	//billboardEffect.init();
	objectEffect.init(&xapp().objectStore, 1);
	float aspectRatio = xapp().aspectRatio;

	// initialize game time to real time:
	gameTime.init(1);
	startTime = gameTime.getRealTime();

	xapp().setBackgroundColor(XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
	//xapp().setBackgroundColor(Colors::Silver);
	//xapp().setBackgroundColor(XMFLOAT4(0.0021973f, 0.0021973f, 0.0021973f, 1.0f));   // prevent strange smearing effect for total black pixels (only in HMD)

	float textSize = 0.5f;
	float lineHeight = 2 * textSize;
	xapp().camera.nearZ = 0.02f;
	xapp().camera.farZ = 500.0f;
	xapp().camera.pos = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	xapp().camera.setSpeed(1.0f); // normal speed
	xapp().camera.fieldOfViewAngleY = 1.289f;
	xapp().world.setWorldSize(2048.0f, 382.0f, 2048.0f);

	textEffect.setSize(textSize);
	textEffect.addTextLine(XMFLOAT4(-5.0f, 7 * lineHeight, 0.0f, 0.0f), xapp().buildInfo, Linetext::XY);
	textEffect.addTextLine(XMFLOAT4(-5.0f, 6 * lineHeight, 0.0f, 0.0f), "F1-F2 to change abient light level", Linetext::XY);
	textEffect.addTextLine(XMFLOAT4(-5.0f, 5 * lineHeight, 0.0f, 0.0f), "F3-F4 to change directional+ light level", Linetext::XY);
	fpsLine = textEffect.addTextLine(XMFLOAT4(-5.0f, 4 * lineHeight, 0.0f, 0.0f), "FPS", Linetext::XY);
	framenumLine = textEffect.addTextLine(XMFLOAT4(-5.0f, 3 * lineHeight, 0.0f, 0.0f), "0123456789", Linetext::XY);

	// textures
	// for controller:
	xapp().textureStore.loadTexture(L"ovr_1c4a32f0d1f4dbb1.dds", "leftController");
	xapp().textureStore.loadTexture(L"ovr_1c0685581a5a8aa4.dds", "rightController");
	// for hands:
	//xapp().textureStore.loadTexture(L"hand_color.dds", "white");
	//xapp().textureStore.loadTexture(L"413fd8923c71e_951f51e94248778a.dds", "RightContollerTex");
	TextureInfo *GrassTex, *HouseTex, *MetalTex, *WormTex, *PlanetTex, *Meteor1Tex, *AxistestTex;
	TextureInfo *RightContollerTex = xapp().textureStore.getTexture("rightController");
	assert(RightContollerTex != nullptr && !RightContollerTex->filename.empty());
	TextureInfo *LeftContollerTex = xapp().textureStore.getTexture("leftController");
	assert(LeftContollerTex != nullptr && !LeftContollerTex->filename.empty());
	xapp().lights.init();
	bigRC.material.ambient = XMFLOAT4(1, 1, 1, 1);
	if (true) {
		xapp().objectStore.loadObject(L"ovr_557a26331850dbf.b", "rightController", 30.0f);
		xapp().objectStore.addObject(bigRC, "rightController", XMFLOAT3(0.0f, -2.0f, 0.0f), RightContollerTex);
		bigRC.rot().x = XM_PIDIV2;
		// controller, shiny:
		bigRC.material.ambient = XMFLOAT4(1, 1, 1, 1);
		bigRC.material.specExp = 20.0f; // 10
		bigRC.material.specIntensity = 700.0f; //70
		bigRC.disableSkinning = true;
	}
	// load controller mesh with correction to mesh data to allow smooth spinning
	//XMFLOAT3 displacement(0.02f, 0.033f, 0.0f);
	XMFLOAT3 displacement(-0.017f, 0.032f, 0.0f);
	xapp().objectStore.loadObject(L"ovr_557a26331850dbf.b", "rightSpinController", 1.0f, &displacement);
	xapp().objectStore.addObject(spinRC, "rightSpinController", XMFLOAT3(-0.4f, -0.3f, 1.9f), RightContollerTex);
	spinRC.rot().x = XM_PI - 0.6f;
	// controller, shiny:
	spinRC.material.ambient = XMFLOAT4(1, 1, 1, 1);
	spinRC.material.specExp = 20.0f; // 10
	spinRC.material.specIntensity = 700.0f; //70
	spinRC.disableSkinning = true;
	spinRC.alpha = 1.0f;

	XMFLOAT3 displacement2(0.017f, 0.032f, 0.0f);
	xapp().objectStore.loadObject(L"ovr_6feb9283b780b5a3.b", "leftSpinController", 1.0f, &displacement2);
	xapp().objectStore.addObject(spinLC, "leftSpinController", XMFLOAT3(-0.75f, -0.3f, 1.9f), LeftContollerTex);
	spinLC.rot().x = XM_PI - 0.6f;
	// controller, shiny:
	spinLC.material.ambient = XMFLOAT4(1, 1, 1, 1);
	spinLC.material.specExp = 10.0f; // 10
	spinLC.material.specIntensity = 70.0f; //70
	spinLC.disableSkinning = true;
	spinLC.alpha = 1.0f;
	// ghost images, indicate where the controllers should be moved to for bonding with hands:
	xapp().objectStore.addObject(ghostLC, "leftSpinController", XMFLOAT3(-0.4f, -0.4f, 0.35f), LeftContollerTex);
	ghostLC.material.ambient = XMFLOAT4(1, 1, 1, 1);
	ghostLC.material.specExp = 10.0f;
	ghostLC.material.specIntensity = 70.0f;
	ghostLC.disableSkinning = true;
	ghostLC.alpha = 0.1f;
	xapp().objectStore.addObject(ghostRC, "rightSpinController", XMFLOAT3(-0.1f, -0.4f, 0.35f), RightContollerTex);
	ghostRC.material.ambient = XMFLOAT4(1, 1, 1, 1);
	ghostRC.material.specExp = 10.0f;
	ghostRC.material.specIntensity = 70.0f;
	ghostRC.disableSkinning = true;
	ghostRC.alpha = 0.1f;

	CBVLights *lights = &xapp().lights.lights;

	// ambient light
	//lights->ambient[0].ambient = XMFLOAT4(0.3, 0.3, 0.3, 1); overwritten in update()
	assert(0 < MAX_AMBIENT);
	globalAmbientLightLevel = 0.3f;
	globalDirectionalLightLevel = 1.0f;
	globalAmbientLightLevel = 0.0f;
	globalDirectionalLightLevel = 0.0f;

	// directional lights:
	dirColor1 = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	dirColor2 = XMFLOAT4(0.6f, 0.4f, 0.6f, 1.0f);
	lights->directionalLights[0].color = dirColor1;
	//lights->directionalLights[0].color = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	lights->directionalLights[0].pos = XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f);
	lights->directionalLights[0].used_fill.x = 1.0f;

	lights->directionalLights[1].color = dirColor2;
	lights->directionalLights[1].pos = XMFLOAT4(0.0f, 0.5f, 1.0f, 1.0f);
	lights->directionalLights[1].used_fill.x = 1.0f;

	// point lights:
	lights->pointLights[0].color = dirColor1;
	//lights->pointLights[0].pos = XMFLOAT4(7.0f, 10.0f, 8.0f, 1.0f);
	lights->pointLights[0].pos = XMFLOAT4(-1.0f, -0.5f, -0.5f, 1.0f);
	lights->pointLights[0].range_reciprocal = 1.0f / 3.0f;//40.0f;
	lights->pointLights[0].used = 1.0f;

	lights->pointLights[1].color = Colors::White;
	lights->pointLights[1].pos = XMFLOAT4(7.0f, 10.0f, 8.0f, 1.0f);
	lights->pointLights[1].range_reciprocal = 1.0f / 0.3f;//40.0f;
	lights->pointLights[1].used = 0.0f;

	if (loadAvatarAssetsFromOculus)
		xapp().vr.loadAvatarFromOculus();
	else
		xapp().vr.loadAvatarDefault();

	// mono game sound from world object
	xapp().sound.openSoundFile(L"Blue_Danube_by_Strauss.wav", "music", true);
	xapp().sound.playSound("music", SoundCategory::MUSIC);
	//xapp().sound.lowBackgroundMusicVolume();
	xapp().sound.openSoundFile(L"interaction_whoosh_small_01.wav", "target_aquired", false);
	xapp().sound.openSoundFile(L"interaction_magic_spell_02.wav", "bonded", false);
}

void TouchOdyssey::enableMovement(bool enable, WorldObject * o, double nowf)
{
	if (!isMovingRCFinished) {
		if (enable && !isMovingRC) {
			isMovingRC = true;
			// movement path for RC:
			vector<XMFLOAT4> points;
			points.push_back(XMFLOAT4(spinRC.pos().x, spinRC.pos().y, spinRC.pos().z, 1.0)); // start
			points.push_back(XMFLOAT4(ghostRC.pos().x, ghostRC.pos().y, ghostRC.pos().z, 400)); // end
			spinRC.pos() = XMFLOAT3(0, 0, 0);
			spinRC.objectStartPos = spinRC.pos();
			//points.push_back(XMFLOAT4(0, 0, -2.5, 200)); // end
			vector<XMFLOAT3> rotations;
			rotations.push_back(XMFLOAT3(0, 0, 0)); // start
			rotations.push_back(XMFLOAT3(XM_PIDIV2, 0, 0)); // end
			rotations.push_back(XMFLOAT3(XM_PIDIV4, 0.5f, 0)); // end
			auto &path = xapp().world.path;
			path.adjustTimingsConst(points, 10.0f);//10.0f);
			path.defineAction("moveRC", spinRC, points, nullptr);//&rotations);
			spinRC.setAction("moveRC");
			spinRC.pathDescMove->pathMode = Path_SimpleMode;
			spinRC.pathDescMove->starttime_f = nowf;
			spinRC.pathDescMove->handleRotation = false;
			spinRC.pos() = XMFLOAT3(points[0].x, points[0].y, points[0].z); // reset pos to prevent wrong redraw bug 
			WorldObject *o = &xapp().vr.avatarInfo.handRight.o;
			if (o && o->useQuaternionRotation) {
				xapp().sound.playSound("target_aquired");
				o->soundDef = &xapp().sound.sounds["target_aquired"];
				o->maxListeningDistance = 20.0f;
				xapp().sound.addWorldObject(o, nullptr);
			}
		}
		else if (!enable && isMovingRC) {
			// stop movement:
			spinRC.pathDescMove->pathMode = Path_Stopped;
		}
		else if (enable && isMovingRC && spinRC.pathDescMove->pathMode == Path_Stopped) {
			// re-enable movement:
			isMovingRC = false;
		}
		else if (enable && isMovingRC) {
			if (spinRC.pathDescMove->isLastPos) {
				isMovingRCFinished = true;
				spinRC.pathDescMove->handleRotation = false;
			}
		}
	}
	else if (isMovingRCFinished) {
		if (!isTurningRC) {
			isTurningRC = true;
			// movement path for RC:
			vector<XMFLOAT4> points;
			points.push_back(XMFLOAT4(spinRC.pos().x, spinRC.pos().y, spinRC.pos().z, 1.0)); // start
			points.push_back(XMFLOAT4(spinRC.pos().x, spinRC.pos().y, spinRC.pos().z, 100.0)); // == end
			spinRC.pos() = XMFLOAT3(0, 0, 0);
			spinRC.objectStartPos = spinRC.pos();
			//points.push_back(XMFLOAT4(0, 0, -2.5, 200)); // end
			vector<XMFLOAT3> rotations;
			rotations.push_back(XMFLOAT3(spinRC.rot().y, -spinRC.rot().x, spinRC.rot().z)); // start
			rotations.push_back(XMFLOAT3(ghostRC.rot().x, ghostRC.rot().y, ghostRC.rot().z)); // end
																							  //spinRC.rot() = XMFLOAT3(0, 0, 0);
			auto &path = xapp().world.path;
			//path.adjustTimingsConst(points, 10.0f);
			spinRC.pathDescMove->isLastPos = false;
			path.defineAction("moveRC", spinRC, points, &rotations);
			spinRC.setAction("moveRC");
			spinRC.pathDescMove->pathMode = Path_SimpleMode;
			spinRC.pathDescMove->starttime_f = nowf;
			spinRC.pathDescMove->handleRotation = true;
			spinRC.pos() = XMFLOAT3(points[0].x, points[0].y, points[0].z); // reset pos to prevent wrong redraw bug 
																			//spinRC.rot() = XMFLOAT3(rotations[0].x, rotations[0].y, rotations[0].z); // reset pos to prevent wrong redraw bug 
																			//spinRC.rot() = XMFLOAT3(rotations[0].y, -rotations[0].x, rotations[0].z); // reset pos to prevent wrong redraw bug 
		}
		else if (isTurningRC) {
			if (spinRC.pathDescMove->isLastPos) {
				isTurningRCFinished = true;
			}
		}
	}
}

void TouchOdyssey::enableSpinLight(bool enable, WorldObject * o)
{
	CBVLights *lights = &xapp().lights.lights;
	if (!enable) {
		lights->pointLights[1].used = 0.0f;
	}
	else {
		lights->pointLights[1].pos = XMFLOAT4(o->pos().x, o->pos().y, o->pos().z - 0.03f, 1.0f);
		lights->pointLights[1].used = 1.0f;
	}
}

void TouchOdyssey::update()
{
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
	double nowf = gameTime.getTimeAbsSeconds();
	static bool done = false;
	if (!done && gameTime.getSecondsBetween(startTime, now) > 3) {
	}
	//startMovement(nowf);

	// ambient light level
	if (xapp().keyDown(VK_F1)) {
		globalAmbientLightLevel -= 0.01f;
	}
	if (xapp().keyDown(VK_F2)) {
		globalAmbientLightLevel += 0.01f;
	}
	if (globalAmbientLightLevel < 0.0f) globalAmbientLightLevel = 0.0f;
	if (globalAmbientLightLevel > 1.0f) globalAmbientLightLevel = 1.0f;

	// directional light level
	if (xapp().keyDown(VK_F3)) {
		globalDirectionalLightLevel -= 0.01f;
	}
	if (xapp().keyDown(VK_F4)) {
		globalDirectionalLightLevel += 0.01f;
	}
	if (globalDirectionalLightLevel < 0.0f) globalDirectionalLightLevel = 0.0f;
	if (globalDirectionalLightLevel > 1.0f) globalDirectionalLightLevel = 1.0f;

	//linesEffect.update();
	//dotcrossEffect.update();
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
	linesEffect.update();
	//billboardEffect.update();

	CBVLights *lights = &xapp().lights.lights;
	auto &lightControl = xapp().lights;
	float f = globalAmbientLightLevel;
	lights->ambientLights[0].ambient = XMFLOAT4(f, f, f, 1);
	lights->directionalLights[0].color = lightControl.factor(globalDirectionalLightLevel, dirColor1);
	lights->directionalLights[1].color = lightControl.factor(globalDirectionalLightLevel, dirColor2);
	bigRC.update();
	ghostLC.update();
	ghostRC.update();
	spinRC.update();
	//Log("spinRC rot x y z " << spinRC.rot().x << " " << spinRC.rot().y << " " << spinRC.rot().z << endl);
	spinLC.update();
	//Log("obj pos " << bigRC.pos().x << endl);

	if (xapp().ovrRendering)	xapp().vr.handleOVRMessages();
	if (!&xapp().vr.avatarInfo.readyToRender) return;

	// simple spin until we reach automated last turning step
	double fullturn_sec = 5.0;
	double turnfrac = fmod(nowf, fullturn_sec) / fullturn_sec;  // 0.0 .. 1.0
	float d = Util::distance3(&spinRC.pos(), &ghostRC.pos());
	if (!isTurningRC && !isTurningRCFinished && d > 0.2f) {
		spinRC.rot().z = (float)(turnfrac * XM_2PI);
		//Log("turn z " << spinRC.rot().z << endl);
	}
	spinLC.rot().z = (float)(turnfrac * XM_2PI);

	// tests:
	{
		XMVECTOR start = XMVectorSet(-2.0f, -0.6f, -0.56f, 0.0f);
		XMVECTOR end = XMVectorSet(-1.0f, -0.2f, -0.1f, 0.0f);
		XMVECTOR np = Util::movePointToDistance(start, end, 0.01f); // 1 cm
																	//Log("np = " << XMVectorGetX(np) << endl);
		XMVECTOR point = XMLoadFloat3(&spinRC.pos());
	}

	WorldObject *o = &xapp().vr.avatarInfo.handRight.o;
	if (isTurningRCFinished && !isBondedRC) {
		float d = Util::distance3(&spinRC.pos(), &o->pos());
		if (d < 0.045f) {
			isBondedRC = true;
			xapp().sound.playSound("bonded");
			o->soundDef = &xapp().sound.sounds["bonded"];
			o->maxListeningDistance = 20.0f;
			xapp().sound.addWorldObject(o, nullptr);

		}
	}
	if (o && o->useQuaternionRotation) {

		// distance
		// make same calc using Util methods:
		XMVECTOR lp1, lp2;
		XMVECTOR start = XMVectorZero();
		XMVECTOR end = XMVectorSet(-1.0f, -0.2f, -0.1f, 0.0f);
		XMVECTOR point = XMLoadFloat3(&spinRC.pos());
		Util::calcBeamFromObject(&lp1, &lp2, o, start, end);
		float dist2 = Util::distancePoint2Beam(lp1, lp2, point);
		//Log("dist2: " << dist2 << endl);
		bool hit = Util::isTargetHit(o, start, end, &spinRC, 0.27f);
		//if (hit) Log("HIT!" << endl);
		enableSpinLight(hit, &spinRC);
		enableMovement(hit, &spinRC, nowf);
		//if (hit) startMovement(nowf);

		// debug lines:
		vector<LineDef> lines;
		LineDef line;
		line.color = Colors::Red;
		XMStoreFloat3(&line.start, lp1);
		XMStoreFloat3(&line.end, lp2);
		lines.push_back(line);
		//xapp().world.linesEffect->addOneTime(lines);
	}
	xapp().lights.update();
	xapp().sound.Update();
}

void TouchOdyssey::draw()
{
	//xapp().vr.drawController(true);
	xapp().vr.drawHand(true);
	if (isBondedRC)
		xapp().vr.drawController(false);
	else
		spinRC.draw();
	xapp().vr.drawHand(false);
	bigRC.draw();
	spinLC.draw();
	//Log("spinRC rot x y z " << spinRC.rot().x << " " << spinRC.rot().y << " " << spinRC.rot().z << endl);
	//Log("spinRC pos x y z " << spinRC.pos().x << " " << spinRC.pos().y << " " << spinRC.pos().z << endl);
	ghostLC.draw();
	if (!isTurningRCFinished)
		ghostRC.draw();
	linesEffect.draw();
	postEffect.draw();
}

void TouchOdyssey::destroy()
{
}

static TouchOdyssey TouchOdyssey;