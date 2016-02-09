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
	textEffect.init();
	billboardEffect.init();
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
	textEffect.addTextLine(XMFLOAT4(-5.0f, 5 * lineHeight, 0.0f, 0.0f), xapp().buildInfo, Linetext::XY);
	fpsLine = textEffect.addTextLine(XMFLOAT4(-5.0f, 4 * lineHeight, 0.0f, 0.0f), "FPS", Linetext::XY);
	framenumLine = textEffect.addTextLine(XMFLOAT4(-5.0f, 3 * lineHeight, 0.0f, 0.0f), "0123456789", Linetext::XY);

	XMFLOAT3 myPoints[] = {
		XMFLOAT3(0.1f, 0.1f, 0.1f)
	};

	xapp().world.drawCoordinateSystem(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), "Origin", textEffect, dotcrossEffect, textSize);

	// textures
	//xapp().textureStore.loadTexture(L"grassdirt8.dds", "grass");
	xapp().textureStore.loadTexture(L"dirt6_markings.dds", "default");

	// create a billboards:
	BillboardElement b;
	b.pos = XMFLOAT3(15.0f, 0.0f, 2.0f);
	b.normal = XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f);
	b.size = XMFLOAT2(2.0f, 1.0f);
	size_t id1 = billboardEffect.add("default", b);

	// load object
	//xapp->world.loadObject(L"path.b", "Path");
	//xapp->world.addObject(pathObject, "Path", XMFLOAT3(10.0f, 10.0f, 10.0f), GRASSDIRT8);
	////set alpha:
	//pathObject.alpha = 0.5f;
	//xapp->objectStore.loadObject(L"shaded2.b", "Shaded");
	//xapp->objectStore.addObject(pathObject, "Shaded", XMFLOAT3(10.0f, 5.0f, 10.0f), GRASSDIRT8);

	xapp().objectStore.loadObject(L"house4_anim.b", "House");
	xapp().objectStore.addObject(object, "House", XMFLOAT3(10.0f, 10.0f, 10.0f), 0);
	//pathObject.setAction("Cube");
	//pathObject.pathDescMove->pathMode = Path_Reverse;

}

void ObjectViewer::update()
{
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
	static bool done = false;
	if (!done && gameTime.getSecondsBetween(startTime, now) > 3) {
	}
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
	billboardEffect.update();
}

void ObjectViewer::draw()
{
	linesEffect.draw();
	dotcrossEffect.draw();
	textEffect.draw();
	billboardEffect.draw();
	postEffect.draw();
}

void ObjectViewer::destroy()
{
}

static ObjectViewer ObjectViewer;