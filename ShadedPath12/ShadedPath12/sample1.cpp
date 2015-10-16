#include "stdafx.h"
#include "sample1.h"


Sample1::Sample1() : XAppBase()
{
	myClass = string(typeid(*this).name());
	xapp().registerApp(myClass, this);
}


Sample1::~Sample1()
{
}

string Sample1::getWindowTitle() {
	return "DX12 Sample";
}

void Sample1::init()
{
	linesEffect.init();
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
	linesEffect.add(lines);
	// initialize game time to real time:
	gameTime.init(1);
	startTime = gameTime.getRealTime();

	// most from old kitchen.cpp
	float textSize = 0.5f;
	float lineHeight = 2 * textSize;
	xapp().camera.nearZ = 0.2f;
	xapp().camera.farZ = 2000.0f;
	//xapp->camera.pos.z = -3.0f;
	//xapp().camera.pos = XMFLOAT4(1.0f, 1.7f, 1.0f, 0.0f);
	xapp().camera.pos = XMFLOAT4(0.0f, 0.0f, -5.0f, 0.0f);
	//xapp().camera.setSpeed(0.5f);
	xapp().camera.setSpeed(50.5f);
	xapp().camera.fieldOfViewAngleY = 1.289f;

	xapp().world.setWorldSize(2048.0f, 382.0f, 2048.0f);
	Grid *g = xapp().world.createWorldGrid(10.0f);
	linesEffect.add(g->lines);
}

void Sample1::update()
{
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
	static bool done = true;
	if (!done && gameTime.getSecondsBetween(startTime, now) > 3) {
		done = true;
		float aspectRatio = xapp().aspectRatio;
		LineDef myLines[] = {
			// start, end, color
			{ XMFLOAT3(0.1f, 0.15f * aspectRatio, 0.0f), XMFLOAT3(0.15f, -0.35f * aspectRatio, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(0.15f, -0.35f * aspectRatio, 0.0f), XMFLOAT3(-0.15f, -0.35f * aspectRatio, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(-0.15f, -0.35f * aspectRatio, 0.0f), XMFLOAT3(0.1f, 0.15f * aspectRatio, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) }
		};
		// add all intializer objects to vector:
		vector<LineDef> lines;
		for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l);});
		linesEffect.add(lines);
		LinesEffect::cbv_ c;
		XMMATRIX ident = XMMatrixIdentity();
		XMStoreFloat4x4(&c.wvp, ident);
		c.wvp._11 += 2.0f;
		linesEffect.updateCBV(c);
	}
	linesEffect.update();
	LinesEffect::cbv_ c;
	XMStoreFloat4x4(&c.wvp, xapp().camera.worldViewProjection());
	linesEffect.updateCBV(c);
}

void Sample1::draw()
{
	linesEffect.draw();
}

void Sample1::next()
{
	linesEffect.next();
}

void Sample1::destroy()
{
	linesEffect.destroy();
}

static Sample1 sample1;