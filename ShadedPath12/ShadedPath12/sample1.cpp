#include "stdafx.h"
#include "Effects/lines.h"
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
}

void Sample1::update()
{
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
	if (gameTime.getSecondsBetween(startTime, now) > 3) {
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
	}
	linesEffect.update();
}

void Sample1::draw()
{
	linesEffect.draw();
}

static Sample1 sample1;