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

	// initialize game time to real time:
	gameTime.init(1);
	startTime = gameTime.getRealTime();

	float textSize = 0.5f;//1.0f;
	float lineHeight = 2.0 * textSize;
	xapp().camera.nearZ = 0.2f;
	xapp().camera.farZ = 2000.0f;
	//xapp->camera.pos.z = -3.0f;
	//xapp().camera.pos = XMFLOAT4(1.0f, 1.7f, 1.0f, 0.0f);
	xapp().camera.pos = XMFLOAT4(0.0f, 0.0f, -5.0f, 0.0f);
	//xapp().camera.setSpeed(1.0f); // seems ok for VR
	xapp().camera.setSpeed(10.5f); // faster for dev usability
	xapp().camera.fieldOfViewAngleY = 1.289f;

	xapp().world.setWorldSize(2048.0f, 382.0f, 2048.0f);
	textEffect.setSize(textSize);
	textEffect.addTextLine(XMFLOAT4(0.0f, 0 * lineHeight, 0.0f, 0.0f), xapp().buildInfo, Linetext::XY);
	fpsLine = textEffect.addTextLine(XMFLOAT4(0.0f, 1 * lineHeight, 0.0f, 0.0f), "FPS", Linetext::XY);
	framenumLine = textEffect.addTextLine(XMFLOAT4(0.0f, 2 * lineHeight, 0.0f, 0.0f), "0123456789", Linetext::XY);

	bool manyLines = true;
	if (manyLines) {
		for (int i = 0; i < 1000; i++) {
			textEffect.addTextLine(XMFLOAT4(0.0f, (i+5) * lineHeight, 0.0f, 0.0f), "The big brown fox jumps over the lazy dog!", Linetext::XY);
		}
	}
}

void TestLinetext::update()
{
	static int callNum = 0;
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
	// update text:
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
//	if (++callNum == 20) {
		textEffect.update();
		callNum = 0;
//	}
}

void TestLinetext::draw()
{
	textEffect.draw();
	//Sleep(5);
	postEffect.draw();
}

void TestLinetext::destroy()
{
}

static TestLinetext testLinetext;