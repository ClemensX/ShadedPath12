#include "stdafx.h"
#include "objectViewer.h"


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
	objectEffect.init(&xapp().objectStore);
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
	xapp().world.setWorldSize(2048.0f, 382.0f, 2048.0f);

	textEffect.setSize(textSize);
	dotcrossEffect.setLineLength(6.0f * textSize);
	textEffect.addTextLine(XMFLOAT4(-5.0f, 6 * lineHeight, 0.0f, 0.0f), xapp().buildInfo, Linetext::XY);
	textEffect.addTextLine(XMFLOAT4(-5.0f, 5 * lineHeight, 0.0f, 0.0f), "F1-F2 to change abient light level", Linetext::XY);
	fpsLine = textEffect.addTextLine(XMFLOAT4(-5.0f, 4 * lineHeight, 0.0f, 0.0f), "FPS", Linetext::XY);
	framenumLine = textEffect.addTextLine(XMFLOAT4(-5.0f, 3 * lineHeight, 0.0f, 0.0f), "0123456789", Linetext::XY);

	XMFLOAT3 myPoints[] = {
		XMFLOAT3(0.1f, 0.1f, 0.1f)
	};

	xapp().world.drawCoordinateSystem(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), "Origin", textEffect, dotcrossEffect, textSize);

	// textures
	xapp().textureStore.loadTexture(L"grassdirt8.dds", "grass");
	xapp().textureStore.loadTexture(L"dirt6_markings.dds", "default");
	xapp().textureStore.loadTexture(L"metal1.dds", "metal");
	xapp().textureStore.loadTexture(L"worm1.dds", "worm");

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

	TextureInfo *GrassTex = xapp().textureStore.getTexture("grass");
	TextureInfo *HouseTex = xapp().textureStore.getTexture("default");
	TextureInfo *MetalTex = xapp().textureStore.getTexture("metal");
	TextureInfo *WormTex = xapp().textureStore.getTexture("worm");
	xapp().lights.init();
	if (false) {
		xapp().objectStore.loadObject(L"worm5.b", "Worm");
		xapp().objectStore.addObject(object, "Worm", XMFLOAT3(10.0f, 10.0f, 10.0f), WormTex);
		object.setAction("Armature");
		object.pathDescBone->pathMode = Path_Loop;
		object.pathDescBone->speed = 10000.0;
		object.forceBoundingBox(BoundingBox(XMFLOAT3(0.0171146f, 2.33574f, -0.236285f), XMFLOAT3(1.29998f, 2.3272f, 9.97486f)));
	}
	if (false) {
		xapp().objectStore.loadObject(L"joint5_anim.b", "Joint");
		xapp().objectStore.addObject(object, "Joint", XMFLOAT3(10.0f, 10.0f, 10.0f), MetalTex);
		object.setAction("Armature");
		object.pathDescBone->pathMode = Path_Reverse;
		object.pathDescBone->speed = 1000.0;
		object.forceBoundingBox(BoundingBox(XMFLOAT3(3.16211f, 3.16214f, 7.28022f), XMFLOAT3(4.51012f, 4.51011f, 7.6599f)));
		//object.drawBoundingBox = true;
	}
	if (true) {
		xapp().objectStore.loadObject(L"shaded2.b", "Shaded");
		xapp().objectStore.addObject(object, "Shaded", XMFLOAT3(10.0f, 5.0f, 10.0f), GrassTex);
		//object.alpha = 0.7f;
		object.material.ambient = XMFLOAT4(1, 1, 1, 1);
	}
	if (false) {
		xapp().objectStore.loadObject(L"house4_anim.b", "House");
		xapp().objectStore.addObject(object, "House", XMFLOAT3(10.0f, 10.0f, 10.0f), HouseTex);
		//object.drawBoundingBox = true;
		object.setAction("Cube");
		object.pathDescMove->pathMode = Path_Reverse;
		object.pathDescMove->speed = 5000.0f;
	}
	// draw lines for mesh:
	Log(" object created ok, #vertices == " << object.mesh->vertices.size() << endl);
	//vector<LineDef> ol;
	//for (int i = 0; i < object.mesh->vertices.size() - 1; i++) {
	//	LineDef l;
	//	l.color = XMFLOAT4(0.5f, 0.5f, 0.0f, 1.0f);
	//	l.start = object.mesh->vertices[i].Pos;
	//	l.end = object.mesh->vertices[i+1].Pos;
	//	ol.push_back(l);
	//}
	//linesEffect.add(ol);

	//pathObject.setAction("Cube");
	//pathObject.pathDescMove->pathMode = Path_Reverse;

	CBVLights *lights = &xapp().lights.lights;
	lights->ambient[0].ambient = XMFLOAT4(0.3, 0.3, 0.3, 1);
	assert(0 < MAX_AMBIENT);
	globalAmbientLightLevel = 1.0f;
}

void ObjectViewer::update()
{
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
	static bool done = false;
	if (!done && gameTime.getSecondsBetween(startTime, now) > 3) {
	}
	if (xapp().keyDown(VK_F1)) {
		globalAmbientLightLevel -= 0.01;
	}
	if (xapp().keyDown(VK_F2)) {
		globalAmbientLightLevel += 0.01;
	}
	if (globalAmbientLightLevel < 0.0f) globalAmbientLightLevel = 0.0f;
	if (globalAmbientLightLevel > 1.0f) globalAmbientLightLevel = 1.0f;

	xapp().lights.update();
	linesEffect.update();
	dotcrossEffect.update();
	// update info text:
	string fr("Frame ");
	stringstream ss;
	ss << xapp().framenum++;
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
	float f = globalAmbientLightLevel;
	lights->ambient[0].ambient = XMFLOAT4(f,f,f,1);
	object.update();
	//Log("obj pos " << object.pos().x << endl);
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