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
	spinRC.material.specExp = 10.0f;
	spinRC.material.specIntensity = 70.0f;
	spinRC.disableSkinning = true;
	spinRC.alpha = 1.0f;
	// ghost images, indicate where the controllers should be moved to for bonding with hands:
	xapp().objectStore.loadObject(L"ovr_6feb9283b780b5a3.b", "leftSpinController", 1.0f, &displacement);
	xapp().objectStore.addObject(ghostLC, "leftSpinController", XMFLOAT3(-0.4f, -0.2f, 0.55f), LeftContollerTex);
	ghostLC.material.ambient = XMFLOAT4(1, 1, 1, 1);
	ghostLC.material.specExp = 10.0f;
	ghostLC.material.specIntensity = 70.0f;
	ghostLC.disableSkinning = true;
	ghostLC.alpha = 0.1f;
	xapp().objectStore.addObject(ghostRC, "rightSpinController", XMFLOAT3(-0.1f, -0.2f, 0.55f), RightContollerTex);
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
	lights->pointLights[0].pos = XMFLOAT4(7.0f, 10.0f, 8.0f, 1.0f);
	lights->pointLights[0].range_reciprocal = 1.0f / 40.0f;
	lights->pointLights[0].used = 1.0f;

	if (loadAvatarAssetsFromOculus)
		xapp().vr.loadAvatarFromOculus();
	else
		xapp().vr.loadAvatarDefault();
}

void TouchOdyssey::update()
{
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
	double nowf = gameTime.getTimeAbsSeconds();
	static bool done = false;
	if (!done && gameTime.getSecondsBetween(startTime, now) > 3) {
	}

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

	xapp().lights.update();
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
	lights->ambientLights[0].ambient = XMFLOAT4(f,f,f,1);
	lights->directionalLights[0].color = lightControl.factor(globalDirectionalLightLevel, dirColor1);
	lights->directionalLights[1].color = lightControl.factor(globalDirectionalLightLevel, dirColor2);
	bigRC.update();
	spinRC.update();
	ghostLC.update();
	ghostRC.update();
	//Log("obj pos " << bigRC.pos().x << endl);

	if(xapp().ovrRendering)	xapp().vr.handleOVRMessages();

	double fullturn_sec = 5.0;
	double turnfrac = fmod(nowf, fullturn_sec) / fullturn_sec;  // 0.0 .. 1.0
	spinRC.rot().z = (float) (turnfrac * XM_2PI);
	//fullturn_sec *= 2.0; // half rotation speed vertically
	//turnfrac = fmod(nowf, fullturn_sec) / fullturn_sec;  // 0.0 .. 1.0
	//mars.rot().y = turnfrac * XM_2PI;

	// debug lines:
	WorldObject *o = &xapp().vr.avatarInfo.handRight.o;
	if (o) {
		XMVECTOR r2 = XMLoadFloat4(&o->quaternion); //XMQuaternionRotationRollPitchYaw(rot().y, rot().x, rot().z);
		// rotate point
		//XMVECTOR p = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
		XMVECTOR p = XMVectorSet(-1.0f, -0.2f, -0.1f, 0.0f);
		XMVECTOR rotP = XMVector3Rotate(p, r2);
		XMVECTOR t = XMLoadFloat3(&o->pos());
		rotP = rotP + t;
		XMFLOAT3 f;
		XMStoreFloat3(&f, rotP);

		vector<LineDef> lines;
		LineDef line;
		line.color = Colors::Red;
		line.start = o->pos();
		//line.start.y -= 0.06f;
		//line.end = XMFLOAT3(line.start.x + 1.0f, line.start.y + 1.0f, line.start.z + 1.0f);
		line.end = XMFLOAT3(f.x, f.y, f.z);
		lines.push_back(line);
		xapp().world.linesEffect->addOneTime(lines);

		// distance
		XMVECTOR lp1, lp2, refp;
		lp1 = XMLoadFloat3(&o->pos());
		lp2 = rotP;
		refp = XMLoadFloat3(&spinRC.pos());
		float dist = XMVectorGetX(XMVector3LinePointDistance(lp1, lp2, refp));
		Log("dist: " << dist << endl);
	}
}

void TouchOdyssey::draw()
{
	//xapp().vr.drawController(true);
	xapp().vr.drawHand(true);
	//xapp().vr.drawController(false);
	xapp().vr.drawHand(false);
	bigRC.draw();
	spinRC.draw();
	ghostLC.draw();
	ghostRC.draw();
	linesEffect.draw();
	postEffect.draw();
}

void TouchOdyssey::destroy()
{
}

static TouchOdyssey TouchOdyssey;