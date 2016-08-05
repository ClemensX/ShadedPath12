#include "stdafx.h"
#include "logo.h"

static Logo logo_instance;

Logo::Logo()
{
	myClass = string(typeid(*this).name());
	xapp().registerApp(myClass, this);
}

Logo::~Logo()
{
}

void Logo::init()
{
	postEffect.init();
	objectEffect.init(&xapp().objectStore, 1);

	// initialize game time to real time:
	gameTime.init(1);
	startTime = gameTime.getRealTime();

	//xapp().setBackgroundColor(Colors::Black);
	xapp().setBackgroundColor(XMFLOAT4(0.0021973f, 0.0021973f, 0.0021973f, 1.0f));   // prevent strange smearing effect for total black pixels (only in HMD)

	xapp().camera.nearZ = 0.2f;
	xapp().camera.farZ = 2000.0f;
	xapp().camera.pos = XMFLOAT4(0.0f, 0.0f, -3.0f, 0.0f);
	xapp().camera.setSpeed(10.5f); // faster for dev usability
	xapp().camera.fieldOfViewAngleY = 1.289f;
	xapp().world.setWorldSize(2048.0f, 782.0f, 2048.0f);

	xapp().textureStore.loadTexture(L"metal1.dds", "metal");
	TextureInfo *MetalTex = xapp().textureStore.getTexture("metal");
	//object.drawBoundingBox = true;
	//object.drawNormals = true;
	//object.pathDescMove->pathMode = Path_Reverse;
	xapp().lights.init();
	logo.material.ambient = XMFLOAT4(1, 1, 1, 1);
	logo.material.specExp = 1.0f;       // no spec color
	logo.material.specIntensity = 0.0f; // no spec color
	xapp().objectStore.loadObject(L"logo.b", "logo");
	xapp().objectStore.addObject(logo, "logo", XMFLOAT3(0.0f, 0.0f, 0.0f), MetalTex);
	//logo.drawBoundingBox = true;
	//logo.drawNormals = true;
	//logo.pathDescMove->pathMode = Path_Reverse;
	logo.material.specExp = 1.0f;       // no spec color
	logo.material.specIntensity = 0.0f; // no spec color

	// lights
	CBVLights *lights = &xapp().lights.lights;

	// ambient light
	lights->ambientLights[0].ambient = XMFLOAT4(0.3, 0.3, 0.3, 1);
	assert(0 < MAX_AMBIENT);

	// directional lights:
	lights->directionalLights[0].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	lights->directionalLights[0].pos = XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f);
	lights->directionalLights[0].used_fill.x = 1.0f;

	lights->directionalLights[1].color = XMFLOAT4(0.6f, 0.4f, 0.6f, 1.0f);
	lights->directionalLights[1].pos = XMFLOAT4(0.0f, 0.5f, 1.0f, 1.0f);
	lights->directionalLights[1].used_fill.x = 1.0f;

	// point lights:
	lights->pointLights[0].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	lights->pointLights[0].pos = XMFLOAT4(6.0f, 10.0f, 8.0f, 1.0f);
	lights->pointLights[0].range_reciprocal = 1.0f / 30.0f;
	lights->pointLights[0].used = 1.0f;
}

void Logo::update()
{
	xapp().lights.update();
}

void Logo::draw()
{
	logo.draw();
	postEffect.draw();
}

void Logo::destroy()
{
}

string Logo::getWindowTitle()
{
	return "Logo";
}
