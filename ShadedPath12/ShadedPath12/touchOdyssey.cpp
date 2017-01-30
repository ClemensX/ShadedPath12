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
	textEffect.init();
	//billboardEffect.init();
	objectEffect.init(&xapp().objectStore, 1);
	float aspectRatio = xapp().aspectRatio;

	// initialize game time to real time:
	gameTime.init(1);
	startTime = gameTime.getRealTime();

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
	xapp().textureStore.loadTexture(L"ovr_1c0685581a5a8aa4.dds", "rightController");
	// for hands:
	//xapp().textureStore.loadTexture(L"hand_color.dds", "white");
	//xapp().textureStore.loadTexture(L"413fd8923c71e_951f51e94248778a.dds", "RightContollerTex");
	TextureInfo *GrassTex, *HouseTex, *MetalTex, *WormTex, *PlanetTex, *Meteor1Tex, *AxistestTex;
	TextureInfo *RightContollerTex = xapp().textureStore.getTexture("rightController");
	assert(RightContollerTex != nullptr && !RightContollerTex->filename.empty());
	xapp().lights.init();
	bigRC.material.ambient = XMFLOAT4(1, 1, 1, 1);
	if (true) {
		xapp().objectStore.loadObject(L"ovr_557a26331850dbf.b", "rightController", 30.0f);  // right hand
		xapp().objectStore.addObject(bigRC, "rightController", XMFLOAT3(0.0f, -2.0f, 0.0f), RightContollerTex);
		bigRC.rot().x = XM_PIDIV2;
		// controller, shiny:
		bigRC.material.ambient = XMFLOAT4(1, 1, 1, 1);
		bigRC.material.specExp = 20.0f; // 10
		bigRC.material.specIntensity = 700.0f; //70
 		bigRC.disableSkinning = true;
	}
	// draw lines for mesh:
	Log(" object created ok, #vertices == " << bigRC.mesh->vertices.size() << endl);

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
	//billboardEffect.update();

	CBVLights *lights = &xapp().lights.lights;
	auto &lightControl = xapp().lights;
	float f = globalAmbientLightLevel;
	lights->ambientLights[0].ambient = XMFLOAT4(f,f,f,1);
	lights->directionalLights[0].color = lightControl.factor(globalDirectionalLightLevel, dirColor1);
	lights->directionalLights[1].color = lightControl.factor(globalDirectionalLightLevel, dirColor2);
	bigRC.update();
	//Log("obj pos " << bigRC.pos().x << endl);

	if(xapp().ovrRendering)	xapp().vr.handleOVRMessages();
}

void TouchOdyssey::draw()
{
	//xapp().vr.drawController(true);
	xapp().vr.drawHand(true);
	//xapp().vr.drawController(false);
	xapp().vr.drawHand(false);
	bigRC.draw();
	postEffect.draw();
}

void TouchOdyssey::destroy()
{
}

static TouchOdyssey TouchOdyssey;