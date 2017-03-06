#include "stdafx.h"
#include "meshObjectViewer.h"

static MeshObjectViewer MeshObjectViewer;

MeshObjectViewer::MeshObjectViewer()
{
	myClass = string(typeid(*this).name());
	xapp().registerApp(myClass, this);
	objStore = MeshObjectStore::getStore();
}

MeshObjectViewer::~MeshObjectViewer()
{
}

string MeshObjectViewer::getWindowTitle() {
	return "Mesh Object Viewer";
}

void MeshObjectViewer::init()
{
	postEffect.init();
	dotcrossEffect.init();
	linesEffect.init();
	xapp().world.linesEffect = &linesEffect;
	textEffect.init();
	objStore->setMaxObjectCount(1000);
	objStore->init();
	//objectEffect.init(&xapp().objectStore, 1);
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
	xapp().camera.pos = XMFLOAT4(0.0f, 0.0f, -3.0f, 0.0f);
	xapp().camera.setSpeed(10.5f); // faster for dev usability
	xapp().camera.fieldOfViewAngleY = 1.289f;
	xapp().world.setWorldSize(2048.0f, 382.0f, 2048.0f);

	textEffect.setSize(textSize);
	dotcrossEffect.setLineLength(6.0f * textSize);
	textEffect.addTextLine(XMFLOAT4(-5.0f, 7 * lineHeight, 0.0f, 0.0f), xapp().buildInfo, Linetext::XY);
	textEffect.addTextLine(XMFLOAT4(-5.0f, 6 * lineHeight, 0.0f, 0.0f), "F1-F2 to change abient light level", Linetext::XY);
	textEffect.addTextLine(XMFLOAT4(-5.0f, 5 * lineHeight, 0.0f, 0.0f), "F3-F4 to change directional+ light level", Linetext::XY);
	fpsLine = textEffect.addTextLine(XMFLOAT4(-5.0f, 4 * lineHeight, 0.0f, 0.0f), "FPS", Linetext::XY);
	framenumLine = textEffect.addTextLine(XMFLOAT4(-5.0f, 3 * lineHeight, 0.0f, 0.0f), "0123456789", Linetext::XY);

	XMFLOAT3 myPoints[] = {
		XMFLOAT3(0.1f, 0.1f, 0.1f)
	};

	xapp().world.drawCoordinateSystem(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), "Origin", textEffect, dotcrossEffect, textSize);

	// textures
	//xapp().textureStore.loadTexture(L"grassdirt8.dds", "grass");
	//xapp().textureStore.loadTexture(L"dirt6_markings.dds", "default");
	xapp().textureStore.loadTexture(L"metal1.dds", "metal");
	//xapp().textureStore.loadTexture(L"worm1.dds", "worm");
	//xapp().textureStore.loadTexture(L"mars_6k_color.dds", "planet");
	//xapp().textureStore.loadTexture(L"met1.dds", "meteor1");
	//xapp().textureStore.loadTexture(L"axistest.dds", "axistest");
	xapp().textureStore.loadTexture(L"white.dds", "white");

	#pragma warning( disable : 4101 )
	TextureInfo *GrassTex, *HouseTex, *MetalTex, *WormTex, *PlanetTex, *Meteor1Tex, *AxistestTex;
	#pragma warning( default : 4101 )
	MetalTex = xapp().textureStore.getTexture("metal");
	//TextureInfo *GrassTex = xapp().textureStore.getTexture("grass");
	//TextureInfo *HouseTex = xapp().textureStore.getTexture("default");
	//TextureInfo *MetalTex = xapp().textureStore.getTexture("metal");
	//TextureInfo *WormTex = xapp().textureStore.getTexture("worm");
	//TextureInfo *PlanetTex = xapp().textureStore.getTexture("planet");
	//TextureInfo *Meteor1Tex = xapp().textureStore.getTexture("meteor1");
	//TextureInfo *AxistestTex = xapp().textureStore.getTexture("axistest");
	TextureInfo *WhiteTex = xapp().textureStore.getTexture("white");
	//TextureInfo *BrickwallTex = xapp().textureStore.getTexture("brickwall");
	xapp().lights.init();
	//object.material.ambient = XMFLOAT4(1, 1, 1, 1);

	objStore->gpuUploadPhaseStart();
	// object creation:
	if (true) {
		objStore->loadObject(L"house4_anim.b", "House");
		//xapp().objectStore.addObject(object, "House", XMFLOAT3(10.0f, 10.0f, 10.0f), HouseTex);
		object.drawBoundingBox = true;
		object.drawNormals = true;
		//object.setAction("Cube");
		//object.pathDescMove->pathMode = Path_Reverse;
		object.material.specExp = 1.0f;       // no spec color
		object.material.specIntensity = 0.0f; // no spec color
	}

	// draw lines for mesh:
	//Log(" object created ok, #vertices == " << object.mesh->vertices.size() << endl);

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
	lights->pointLights[0].pos = XMFLOAT4(6.0f, 10.0f, 8.0f, 1.0f);
	lights->pointLights[0].range_reciprocal = 1.0f / 30.0f;
	lights->pointLights[0].used = 1.0f;

	objStore->gpuUploadPhaseEnd();
}

void MeshObjectViewer::update()
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
	//billboardEffect.update();

	CBVLights *lights = &xapp().lights.lights;
	auto &lightControl = xapp().lights;
	float f = globalAmbientLightLevel;
	lights->ambientLights[0].ambient = XMFLOAT4(f, f, f, 1);
	lights->directionalLights[0].color = lightControl.factor(globalDirectionalLightLevel, dirColor1);
	lights->directionalLights[1].color = lightControl.factor(globalDirectionalLightLevel, dirColor2);

	objStore->update();
}

void MeshObjectViewer::draw()
{
	linesEffect.draw();
	dotcrossEffect.draw();
	textEffect.draw();
	objStore->draw();
	postEffect.draw();
}

void MeshObjectViewer::destroy()
{
}

