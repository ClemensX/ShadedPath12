#include "stdafx.h"
#include "objectViewer.h"

#define LOAD_AVATAR_DATAXXX

ObjectViewer::ObjectViewer() : XAppBase()
{
	myClass = string(typeid(*this).name());
	xapp().registerApp(myClass, this);
}


ObjectViewer::~ObjectViewer()
{
}

string ObjectViewer::getWindowTitle() {
	return "Object Viewer";
}

void ObjectViewer::init()
{
	postEffect.init();
	dotcrossEffect.init();
	linesEffect.init();
	xapp().world.linesEffect = &linesEffect;
	textEffect.init();
	//billboardEffect.init();
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
	//xapp().camera.pos = XMFLOAT4(-0.0696329f, 0.354773f, 0.324679f, 0.0f);
	//xapp().camera.setSpeed(1.0f); // seems ok for VR
	xapp().camera.setSpeed(10.5f); // faster for dev usability
	xapp().camera.fieldOfViewAngleY = 1.289f;
	//xapp().camera.fieldOfViewAngleY = 0.6f;
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
	xapp().textureStore.loadTexture(L"dirt6_markings.dds", "default");
	xapp().textureStore.loadTexture(L"metal1.dds", "metal");
	//xapp().textureStore.loadTexture(L"worm1.dds", "worm");
	//xapp().textureStore.loadTexture(L"mars_6k_color.dds", "planet");
	//xapp().textureStore.loadTexture(L"met1.dds", "meteor1");
	//xapp().textureStore.loadTexture(L"axistest.dds", "axistest");
	xapp().textureStore.loadTexture(L"white.dds", "white");
	// create a billboards:
	BillboardElement b;
	b.pos = XMFLOAT3(15.0f, 0.0f, 2.0f);
	b.normal = XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f);
	b.size = XMFLOAT2(2.0f, 1.0f);
	//size_t id1 = billboardEffect.add("default", b);

	// load object
	//xapp->world.loadObject(L"path.b", "Path");
	//xapp->world.addObject(pathObject, "Path", XMFLOAT3(10.0f, 10.0f, 10.0f), GRASSDIRT8);
	////set alpha:
	//pathObject.alpha = 0.5f;
	//xapp->objectStore.loadObject(L"shaded2.b", "Shaded");
	//xapp->objectStore.addObject(pathObject, "Shaded", XMFLOAT3(10.0f, 5.0f, 10.0f), GRASSDIRT8);

	TextureInfo *GrassTex, *HouseTex, *MetalTex, *WormTex, *PlanetTex, *Meteor1Tex, *AxistestTex;
	MetalTex = xapp().textureStore.getTexture("metal");
	//TextureInfo *GrassTex = xapp().textureStore.getTexture("grass");
	HouseTex = xapp().textureStore.getTexture("default");
	//TextureInfo *MetalTex = xapp().textureStore.getTexture("metal");
	//TextureInfo *WormTex = xapp().textureStore.getTexture("worm");
	//TextureInfo *PlanetTex = xapp().textureStore.getTexture("planet");
	//TextureInfo *Meteor1Tex = xapp().textureStore.getTexture("meteor1");
	//TextureInfo *AxistestTex = xapp().textureStore.getTexture("axistest");
	TextureInfo *WhiteTex = xapp().textureStore.getTexture("white");
	//TextureInfo *BrickwallTex = xapp().textureStore.getTexture("brickwall");
	xapp().lights.init();
	object.material.ambient = XMFLOAT4(1, 1, 1, 1);
	if (false) {
		/*
		Remember to smooth normals for organic meshes like this worm,
		otherwise you will see checkered display when lighting is on.
		This is due to sharp normal changes for adjacent triangles

		Normal smooting in blender 2.7:
		Properties Window -> Modifiers -> Add Modifier -> Normal Edit
		You have to enable Auto Smooth for Normals in Properties -> Data
		Apply the modifier.
		*/
		xapp().objectStore.loadObject(L"worm5.b", "Worm");
		xapp().objectStore.addObject(object, "Worm", XMFLOAT3(10.0f, 10.0f, 10.0f), WormTex);
		object.setAction("Armature");
		object.pathDescBone->pathMode = Path_Loop;
		object.pathDescBone->speed = 3.0;
		object.forceBoundingBox(BoundingBox(XMFLOAT3(0.0171146f, 2.33574f, -0.236285f), XMFLOAT3(1.29998f, 2.3272f, 9.97486f)));
		object.material.specExp = 100.0f;       // no spec color 1 0 nothing
		object.material.specIntensity = 10.0f; // no spec color
		//object.drawNormals = true;
	}
	if (false) {
		xapp().objectStore.loadObject(L"joint5_anim.b", "Joint");
		xapp().objectStore.addObject(object, "Joint", XMFLOAT3(10.0f, 10.0f, 10.0f), MetalTex);
		object.setAction("Armature");
		object.pathDescBone->pathMode = Path_Reverse;
		object.forceBoundingBox(BoundingBox(XMFLOAT3(3.16211f, 3.16214f, 7.28022f), XMFLOAT3(4.51012f, 4.51011f, 7.6599f)));
		object.material.specExp = 1.0f;       // no spec color
		object.material.specIntensity = 0.0f; // no spec color
		//object.drawNormals = true;
		//object.drawBoundingBox = true;
	}
	if (false) {
		xapp().objectStore.loadObject(L"shaded2.b", "Shaded");
		xapp().objectStore.addObject(object, "Shaded", XMFLOAT3(10.0f, 5.0f, 10.0f), GrassTex);
		//object.drawNormals = true;
		object.material.ambient = XMFLOAT4(1, 1, 1, 1);
		object.material.specExp = 200.0f;       // no spec color 1 0 nothing
		object.material.specIntensity = 1000.0f; // no spec color
		object.material.specExp = 1.0f;       // no spec color 1 0 nothing
		object.material.specIntensity = 0.0f; // no spec color
	}
	if (true) {
		xapp().objectStore.loadObject(L"house4_anim.b", "House");
		xapp().objectStore.addObject(object, "House", XMFLOAT3(1.0f, 1.0f, 1.0f), HouseTex);
		//object.drawBoundingBox = true;
		object.drawNormals = true;
		//object.setAction("Cube");
		//object.pathDescMove->pathMode = Path_Reverse;
		object.material.specExp = 1.0f;       // no spec color
		object.material.specIntensity = 0.0f; // no spec color
	}
	if (false) {
		xapp().objectStore.loadObject(L"sphere3.b", "Planet", 5);
		//xapp().objectStore.addObject(object, "Planet", XMFLOAT3(700.0f, 300.0f, -700.0f), PlanetTex);
		xapp().objectStore.addObject(object, "Planet", XMFLOAT3(10.0f, 30.0f, -70.0f), PlanetTex);
		//object.drawNormals = true;
		object.material.ambient = XMFLOAT4(1, 1, 1, 1);
		object.material.specExp = 200.0f;       // no spec color 1 0 nothing
		object.material.specIntensity = 1000.0f; // no spec color
		object.material.specExp = 1.0f;       // no spec color 1 0 nothing
		object.material.specIntensity = 0.0f; // no spec color
	}
	if (false) {
		xapp().objectStore.loadObject(L"meteor_single.b", "Meteor1");
		//xapp().objectStore.addObject(object, "Planet", XMFLOAT3(700.0f, 300.0f, -700.0f), PlanetTex);
		xapp().objectStore.addObject(object, "Meteor1", XMFLOAT3(10.0f, 30.0f, -70.0f), Meteor1Tex);
		//object.drawNormals = true;
		object.material.ambient = XMFLOAT4(1, 1, 1, 1);
		object.material.specExp = 200.0f;       // no spec color 1 0 nothing
		object.material.specIntensity = 1000.0f; // no spec color
		object.material.specExp = 1.0f;       // no spec color 1 0 nothing
		object.material.specIntensity = 0.0f; // no spec color
		auto &path = xapp().world.path;
		vector<XMFLOAT4> points = { {0.0f, 0.0f, 0.0f, 1.0f}, { 500.0f, 500.0f, 500.0f, 80.0f },{ 500.0f, 500.0f, 1000.0f, 160.0f } };
		path.defineAction("movetest", object, points);
		object.setAction("movetest");
	}
	if (false) {
		xapp().objectStore.loadObject(L"brickwall.b", "Brickwall");
		//xapp().objectStore.addObject(object, "Brickwall", XMFLOAT3(10.0f, 5.0f, 10.0f), BrickwallTex);
		//object.drawNormals = true;
		object.material.ambient = XMFLOAT4(1, 1, 1, 1);
		object.material.specExp = 200.0f;       // no spec color 1 0 nothing
		object.material.specIntensity = 1000.0f; // no spec color
		object.material.specExp = 1.0f;       // no spec color 1 0 nothing
		object.material.specIntensity = 0.0f; // no spec color
	}
	if (false) {
		xapp().objectStore.loadObject(L"axistest.b", "axistest");
		xapp().objectStore.addObject(object, "axistest", XMFLOAT3(10.0f, 10.0f, 10.0f), AxistestTex);
		//object.drawBoundingBox = true;
		object.drawNormals = true;
		//object.pathDescMove->pathMode = Path_Reverse;
		object.material.specExp = 1.0f;       // no spec color
		object.material.specIntensity = 0.0f; // no spec color
	}
	if (false) {
		xapp().objectStore.loadObject(L"logo.b", "logo");
		xapp().objectStore.addObject(object, "logo", XMFLOAT3(3.0f, -1.0f, 2.0f), MetalTex);
		//object.drawBoundingBox = true;
		//object.drawNormals = true;
		//object.pathDescMove->pathMode = Path_Reverse;
		object.material.specExp = 1.0f;       // no spec color
		object.material.specIntensity = 0.0f; // no spec color
	}
	if (false) {
		xapp().objectStore.loadObject(L"light1.b", "light1", 0.1f);
		xapp().objectStore.addObject(object, "light1", XMFLOAT3(3.0f, -1.0f, 2.0f), WhiteTex);
		//object.drawBoundingBox = true;
		//object.drawNormals = true;
		//object.pathDescMove->pathMode = Path_Reverse;
		object.material.specExp = 1.0f;       // no spec color
		object.material.specIntensity = 0.0f; // no spec color
	}
	if (false) {
		//xapp().objectStore.loadObject(L"413fd8923c71e_557a26331850dbf.b", "light1", 1.0f);  // left controller
		//xapp().objectStore.loadObject(L"413fd8923c71e_1274b22c61fc48a3.b", "light1", 1.0f);  // torso clothes
		//xapp().objectStore.loadObject(L"413fd8923c71e_450d4eca9f73b9a1.b", "light1", 1.0f);  // glasses
		//xapp().objectStore.loadObject(L"413fd8923c71e_47be498de8d01599.b", "light1", 1.0f);  // hair
		//xapp().objectStore.loadObject(L"413fd8923c71e_6a4ae11446026286.b", "light1", 1.0f);  // left hand
		//xapp().objectStore.loadObject(L"413fd8923c71e_6feb9283b780b5a3.b", "light1", 1.0f);  // right controller
		//xapp().objectStore.loadObject(L"413fd8923c71e_7f1ca835aeb1b69e.b", "light1", 1.0f);  // large empty cone
		//xapp().objectStore.loadObject(L"413fd8923c71e_8e78d539875b1886.b", "light1", 1.0f);  // open flat ring
		//xapp().objectStore.loadObject(L"413fd8923c71e_af2fdac13313089c.b", "light1", 1.0f);  // face
		xapp().objectStore.loadObject(L"413fd8923c71e_f82847a6b3ddf1a6.b", "light1", 1.0f);  // right hand
		xapp().objectStore.addObject(object, "light1", XMFLOAT3(3.0f, -1.0f, 2.0f), WhiteTex);
		//object.drawBoundingBox = true;
		//object.drawNormals = true;
		//object.pathDescMove->pathMode = Path_Reverse;
		object.material.specExp = 1.0f;       // no spec color
		object.material.specIntensity = 0.0f; // no spec color
	}
	if (false) {
		//XMFLOAT3 displacement(0.02f, 0.033f, 0.0f);
		XMFLOAT3 displacement(-0.017f, 0.032f, 0.0f);
		xapp().objectStore.loadObject(L"ovr_557a26331850dbf.b", "rightSpinController", 1.0f, &displacement);
		xapp().objectStore.addObject(object, "rightSpinController", XMFLOAT3(0.0f, 0.0f, 0.0f), WhiteTex);
		object.rot().x = XM_PIDIV2;
		// controller, shiny:
		object.material.ambient = XMFLOAT4(1, 1, 1, 1);
		object.material.specExp = 10.0f;
		object.material.specIntensity = 70.0f;
		object.disableSkinning = true;
		object.drawBoundingBox = true;
	}
	// draw lines for mesh:
	Log(" object created ok, #vertices == " << object.mesh->vertices.size() << endl);

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

	vector<XMFLOAT3> crossPoints;
	//for_each(begin(myPoints), end(myPoints), [&crossPoints](XMFLOAT3 p) {crossPoints.push_back(p); });
	XMFLOAT3 x = XMFLOAT3(6.0f, 10.0f, 8.0f);
	crossPoints.push_back(x);
	dotcrossEffect.update(crossPoints);

#if defined(LOAD_AVATAR_DATA)
	xapp().vr.loadAvatar();
#endif
}

void ObjectViewer::update()
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
	lights->ambientLights[0].ambient = XMFLOAT4(f,f,f,1);
	lights->directionalLights[0].color = lightControl.factor(globalDirectionalLightLevel, dirColor1);
	lights->directionalLights[1].color = lightControl.factor(globalDirectionalLightLevel, dirColor2);

	object.update();

	// disable for no spinning:
	//double fullturn_sec = 5.0;
	//double turnfrac = fmod(nowf, fullturn_sec) / fullturn_sec;  // 0.0 .. 1.0
	//object.rot().z = (float)(turnfrac * XM_2PI);

	//Log("obj pos " << object.pos().x << endl);

	if(xapp().ovrRendering)	xapp().vr.handleOVRMessages();
}

void ObjectViewer::draw()
{
	linesEffect.draw();
	dotcrossEffect.draw();
	textEffect.draw();
	//billboardEffect.draw();
	object.draw();
	postEffect.draw();
}

void ObjectViewer::destroy()
{
}

static ObjectViewer ObjectViewer;