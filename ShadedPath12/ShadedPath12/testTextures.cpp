#include "stdafx.h"
#include "testTextures.h"


TestTextures::TestTextures() : XAppBase()
{
	myClass = string(typeid(*this).name());
	xapp().registerApp(myClass, this);
}


TestTextures::~TestTextures()
{
}

string TestTextures::getWindowTitle() {
	return "Texture Sample";
}

void TestTextures::init()
{
	postEffect.init();
	dotcrossEffect.init();
	linesEffect.init();
	textEffect.init();
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
	xapp().camera.pos = XMFLOAT4(-0.0696329f, 0.354773f, 0.324679f, 0.0f);
	//xapp().camera.setSpeed(1.0f); // seems ok for VR
	xapp().camera.setSpeed(10.5f); // faster for dev usability
	xapp().camera.fieldOfViewAngleY = 1.289f;
	xapp().world.setWorldSize(2048.0f, 382.0f, 2048.0f);

	textEffect.setSize(textSize);
	dotcrossEffect.setLineLength(6.0f * textSize);
	textEffect.addTextLine(XMFLOAT4(-5.0f, 5 * lineHeight, 0.0f, 0.0f), "Shaded Path 12 Engine Build 2015_11_23", Linetext::XY);
	fpsLine = textEffect.addTextLine(XMFLOAT4(-5.0f, 4 * lineHeight, 0.0f, 0.0f), "FPS", Linetext::XY);
	framenumLine = textEffect.addTextLine(XMFLOAT4(-5.0f, 3 * lineHeight, 0.0f, 0.0f), "0123456789", Linetext::XY);

	XMFLOAT3 myPoints[] = {
		XMFLOAT3(0.1f, 0.1f, 0.1f)
	};

	xapp().world.drawCoordinateSystem(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), "Origin", textEffect, dotcrossEffect, textSize);

	// textures
	//ComPtr<ID3D12CommandAllocator> commandAllocator;
	//ComPtr<ID3D12CommandList> commandList;

	//ThrowIfFailed(xapp().device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
	//ThrowIfFailed(xapp().device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, xapp().pipelineState.Get(), IID_PPV_ARGS(&commandList)));
	//xapp().
	//xapp().textureStore.loadTexture(L"ceil.dds", "default", nullptr);
}

void TestTextures::update()
{
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
	static bool done = false;
	if (!done && gameTime.getSecondsBetween(startTime, now) > 3) {
		done = true;
		//if (xapp().pGraphicsAnalysis != nullptr) xapp().pGraphicsAnalysis->BeginCapture();
		//Sleep(1000);

		float aspectRatio = xapp().aspectRatio;
		LineDef myLines[] = {
			// start, end, color
			{ XMFLOAT3(3.1f, 0.15f * aspectRatio, 0.0f), XMFLOAT3(3.15f, -0.35f * aspectRatio, 0.0f), XMFLOAT4(4.0f, 0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(3.15f, -0.35f * aspectRatio, 0.0f), XMFLOAT3(2.85f, -0.35f * aspectRatio, 0.0f), XMFLOAT4(4.0f, 0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(2.85f, -0.35f * aspectRatio, 0.0f), XMFLOAT3(3.1f, 0.15f * aspectRatio, 0.0f), XMFLOAT4(4.0f, 0.0f, 0.0f, 1.0f) }
		};
		// add all intializer objects to vector:
		vector<LineDef> lines;
		for_each(begin(myLines), end(myLines), [&lines](LineDef l) {lines.push_back(l);});
		linesEffect.add(lines);
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
	// WVP is now updated automatically during draw()
	//LinesEffect::CBV c;
	//XMStoreFloat4x4(&c.wvp, xapp().camera.worldViewProjection());
	//linesEffect.updateCBV(c);
}

void TestTextures::draw()
{
	linesEffect.draw();
	dotcrossEffect.draw();
	textEffect.draw();
	postEffect.draw();
}

void TestTextures::destroy()
{
	//linesEffect.destroy();
}

static TestTextures testTextures;