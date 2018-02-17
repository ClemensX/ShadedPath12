#include "stdafx.h"
#include "sample1.h"


Sample1::Sample1() : XAppBase()
{
	myClass = string(typeid(*this).name());
	xapp->registerApp(myClass, this);
}


Sample1::~Sample1()
{
}

string Sample1::getWindowTitle() {
	return "DX12 Sample";
}

void Sample1::init()
{
	Log("app init()" << endl);
	float aspectRatio = xapp->aspectRatio;

	// initialize game time to real time:
	gameTime.init(1);
	startTime = gameTime.getRealTime();

	float textSize = 0.5f;
	float lineHeight = 2 * textSize;
	xapp->world.setWorldSize(2048.0f, 382.0f, 2048.0f);
	xapp->setMaxThreadCount(3);
	xapp->textureStore.loadTexture(L"dirt6_markings.dds", "markings");

	globalEffect.init();
	clearEffect.init();
	copyTextureEffect.init();

	Grid *g = xapp->world.createWorldGrid(10.0f, -1.65f);
	XMFLOAT3 myPoints[] = {
		XMFLOAT3(0.1f, 0.1f, 0.1f)
	};
	vector<XMFLOAT3> crossPoints;
	for_each(begin(myPoints), end(myPoints), [&crossPoints](XMFLOAT3 p) {crossPoints.push_back(p); });
	//dotcrossEffect.update(crossPoints);

}

void Sample1::update()
{
	//Log("app update()" << endl);
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
	static bool done = false;
	if (!done && gameTime.getSecondsBetween(startTime, now) > 3) {
		done = true;
		//if (xapp().pGraphicsAnalysis != nullptr) xapp().pGraphicsAnalysis->BeginCapture();
		//Sleep(1000);

		float aspectRatio = xapp->aspectRatio;
		// add all intializer objects to vector:
	}
	// update info text:
	string fr("Frame ");
	stringstream ss;
	ss << xapp->getAbsFrameCount();
	fr.append(ss.str());

	string fps_str("FPS ");
	stringstream sss;
	sss << xapp->fps;
	fps_str.append(sss.str());
	// WVP is now updated automatically during draw()
	//LinesEffect::CBV c;
	//XMStoreFloat4x4(&c.wvp, xapp().camera.worldViewProjection());
	//linesEffect.updateCBV(c);
}

void Sample1::draw()
{
	//Log("app draw()" << endl);
	clearEffect.draw();
	copyTextureEffect.draw("markings");
	//linesEffect.draw();
	//dotcrossEffect.draw();
	//textEffect.draw();
	//postEffect.draw();
}

void Sample1::destroy()
{
	Log("app destroy()" << endl);
	//linesEffect.destroy();
}

static Sample1 sample1;