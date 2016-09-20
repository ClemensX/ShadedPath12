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
	xapp().textureStore.loadTexture(L"white.dds", "white");
	TextureInfo *MetalTex = xapp().textureStore.getTexture("metal");
	TextureInfo *WhiteTex = xapp().textureStore.getTexture("white");
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

	XMFLOAT4 lightStartPos4 = XMFLOAT4(-2, 0, -4.5, 1);
//	XMFLOAT3 lightStartPos3 = XMFLOAT3(-2, 0, -4.5);
	XMFLOAT3 lightStartPos3 = XMFLOAT3(0, 0, 0);
	// init light objects:
	xapp().objectStore.loadObject(L"light1.b", "light1", 0.01f);
	for (int i = 0; i < 1; i++) {
		woLights[i].material.ambient = XMFLOAT4(1, 1, 1, 1);
		woLights[i].material.specExp = 1.0f;       // no spec color
		woLights[i].material.specIntensity = 0.0f; // no spec color
		xapp().objectStore.addObject(woLights[i], "light1", lightStartPos3, WhiteTex);
		woLights[i].material.specExp = 1.0f;       // no spec color
		woLights[i].material.specIntensity = 0.0f; // no spec color

	}
	// lights
	CBVLights *lights = &xapp().lights.lights;

	// ambient light
	lights->ambientLights[0].ambient = XMFLOAT4(0.1, 0.1, 0.1, 1);
	assert(0 < MAX_AMBIENT);

	// directional lights:
	lights->directionalLights[0].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	lights->directionalLights[0].pos = XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f);
	lights->directionalLights[0].used_fill.x = 0.0f;

	lights->directionalLights[1].color = XMFLOAT4(0.6f, 0.4f, 0.6f, 1.0f);
	lights->directionalLights[1].pos = XMFLOAT4(0.0f, 0.5f, 1.0f, 1.0f);
	lights->directionalLights[1].used_fill.x = 0.0f;

	// point lights:
	lights->pointLights[0].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	lights->pointLights[0].pos = lightStartPos4;
	lights->pointLights[0].range_reciprocal = 1.0f / 10.0f;
	lights->pointLights[0].used = 1.0f;
	//lights->pointLights[0].used = 0.0f;

	startMovement = true;
}

void Logo::update()
{
	gameTime.advanceTime();
	double nowf = gameTime.getTimeAbsSeconds();
	xapp().lights.update();
	if (startMovement == true) {
		startMovement = false;
		vector<XMFLOAT4> points;
		points.push_back(XMFLOAT4(-1, -1, 2, 1.0)); // start
		points.push_back(XMFLOAT4(0, 0, -2.5, 100)); // end
		points.push_back(XMFLOAT4(0, 0, -2.5, 200)); // end
		vector<XMFLOAT3> rotations;
		rotations.push_back(XMFLOAT3(0, 0, 0)); // start
		//rotations.push_back(XMFLOAT3(XM_PIDIV2, XM_PIDIV2, XM_PIDIV2)); // end
		rotations.push_back(XMFLOAT3(XM_PIDIV2, 0, 0)); // end
		rotations.push_back(XMFLOAT3(XM_PIDIV4, 0.5f, 0)); // end

		auto &path = xapp().world.path;
		path.defineAction("movelogo", logo, points, &rotations);
		logo.setAction("movelogo");
		logo.pathDescMove->pathMode = Path_SimpleMode;
		logo.pathDescMove->starttime_f = nowf;
		logo.pathDescMove->handleRotation = true;

		// light movement:
		{
			vector<XMFLOAT4> pointsL;
			pointsL.push_back(XMFLOAT4(-10, 2, -5.5, 1.0)); // start
			pointsL.push_back(XMFLOAT4(0, 2, -5.5, 100)); // end
			pointsL.push_back(XMFLOAT4(10, 2, 5.5, 200)); // end
			path.defineAction("movelight", woLights[0], pointsL);
			woLights[0].setAction("movelight");
			woLights[0].pathDescMove->pathMode = Path_Reverse;
			woLights[0].pathDescMove->starttime_f = nowf;
			woLights[0].pathDescMove->handleRotation = false;
		}
	}
	CBVLights *lights = &xapp().lights.lights;
	XMFLOAT4 curPos = XMFLOAT4(woLights[0].pos().x, woLights[0].pos().y, woLights[0].pos().z, 1);
	//Log("curPos " << curPos.x << " " << curPos.y << " " << curPos.z << endl);
	lights->pointLights[0].pos = curPos;
}

void Logo::draw()
{
	logo.draw();
	woLights[0].draw();
	postEffect.draw();
}

void Logo::destroy()
{
}

string Logo::getWindowTitle()
{
	return "Logo";
}
