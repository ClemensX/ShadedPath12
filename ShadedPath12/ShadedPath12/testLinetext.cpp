#include "stdafx.h"
#include "testLinetext.h"


TestLinetext::TestLinetext() : XAppBase()
{
	myClass = string(typeid(*this).name());
	xapp().registerApp(myClass, this);
}


TestLinetext::~TestLinetext()
{
}

string TestLinetext::getWindowTitle() {
	return "DX12 Sample - Test Linetext shader";
}

void TestLinetext::init()
{
	textEffect.init();
	postEffect.init();
	float aspectRatio = xapp().aspectRatio;

	LineDef myLines[] = {
		// start, end, color
		{ XMFLOAT3(0.0f, 0.25f * aspectRatio, 0.0f), XMFLOAT3(0.25f, -0.25f * aspectRatio, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(0.25f, -0.25f * aspectRatio, 0.0f), XMFLOAT3(-0.25f, -0.25f * aspectRatio, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-0.25f, -0.25f * aspectRatio, 0.0f), XMFLOAT3(0.0f, 0.25f * aspectRatio, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }
	};
	vector<LineDef> lines;
	// add all intializer objects to vector:
	for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l);});
	//linesEffect.add(lines);
	// initialize game time to real time:
	gameTime.init(1);
	startTime = gameTime.getRealTime();

	// most from old kitchen.cpp
	float textSize = 1.0f;
	float lineHeight = 0.2 * textSize;
	xapp().camera.nearZ = 0.2f;
	xapp().camera.farZ = 2000.0f;
	//xapp->camera.pos.z = -3.0f;
	//xapp().camera.pos = XMFLOAT4(1.0f, 1.7f, 1.0f, 0.0f);
	xapp().camera.pos = XMFLOAT4(0.0f, 0.0f, -5.0f, 0.0f);
	//xapp().camera.setSpeed(1.0f); // seems ok for VR
	xapp().camera.setSpeed(10.5f); // faster for dev usability
	xapp().camera.fieldOfViewAngleY = 1.289f;

	xapp().world.setWorldSize(2048.0f, 382.0f, 2048.0f);
	//textEffect.addTextLine(XMFLOAT4(14.0f, 2 * lineHeight, 10.0f, 0.0f), "Shaded Path Engine Build 2015_01_11", Linetext::XY);
	textEffect.addTextLine(XMFLOAT4(0.0f, 0 * lineHeight, 0.0f, 0.0f), "Shaded Path 12 Engine Build 2015_11_23", Linetext::XY);
	//textEffect.addTextLine(XMFLOAT4(0.0f, 1 * lineHeight, 0.0f, 0.0f), "Huhu!", Linetext::XY);
}

void TestLinetext::update()
{
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
}

void TestLinetext::draw()
{
	// update and draw text:
	string fr("Frame ");
	stringstream ss;
	ss << xapp().framenum++;
	fr.append(ss.str());
	//textEffect->changeTextLine(framenumLine, fr);

	string fps_str("FPS ");
	stringstream sss;
	sss << xapp().fps;
	fps_str.append(sss.str());
	//textEffect->changeTextLine(fpsLine, fps_str);
	textEffect.draw();
	Sleep(5);
	postEffect.draw();
}

void TestLinetext::destroy()
{
}

static TestLinetext testLinetext;