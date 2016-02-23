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
	//xapp().textureStore.loadTexture(L"dirt6_markings.dds", "default");
	xapp().textureStore.loadTexture(L"vac_00.dds", "vac00");
	xapp().textureStore.loadTexture(L"vac_01.dds", "vac01");
	xapp().textureStore.loadTexture(L"vac_02.dds", "vac02");
	xapp().textureStore.loadTexture(L"vac_03.dds", "vac03");
	xapp().textureStore.loadTexture(L"vac_04.dds", "vac04");
	xapp().textureStore.loadTexture(L"vac_05.dds", "vac05");
	xapp().textureStore.loadTexture(L"vac_06.dds", "vac06");
	xapp().textureStore.loadTexture(L"vac_07.dds", "vac07");
	xapp().textureStore.loadTexture(L"vac_08.dds", "vac08");
	xapp().textureStore.loadTexture(L"vac_09.dds", "vac09");
	xapp().textureStore.loadTexture(L"vac_10.dds", "vac10");
	xapp().textureStore.loadTexture(L"vac_11.dds", "vac11");
	TextureInfo *tex = xapp().textureStore.getTexture("default");
	if (tex->available && textureFullFrameTest) {
		postEffect.setAlternateFinalFrame(tex->m_srvHeap.Get());
	}

	// create some billboards:
	BillboardElement b;
	b.pos = XMFLOAT3(15.0f, 0.0f, 2.0f);
	b.normal = XMFLOAT4(0.0f, 0.0f, -1.0f, 1.0f);
	b.size = XMFLOAT2(2.0f, 1.0f);
	//size_t id1 = billboardEffect.add("default", b);
	b.pos = XMFLOAT3(-1.0f, 1.5f, 1.0f);
	b.size = XMFLOAT2(36.29f, 24.192f);
	b.size = XMFLOAT2(3.629f, 2.4192f);
	size_t id2 = billboardEffect.add("vac00", b);
	//BillboardElement &ref = billboardEffect.get("default", id1);
	//ref.pos.z -= 0.1f;
	//ref = billboardEffect.get("default", id1);
	
//	unsigned long total_billboards = 4000000;
	unsigned long total_billboards = 1000000;
//	unsigned long total_billboards = 50000;
//	unsigned long total_billboards = 5;
	unsigned long billboards_per_texture = total_billboards / 12;

	// create randomly positioned billboards for each vacXX texture we have:
	for (int tex_number = 0; tex_number < 12; tex_number++) {
		//assemble texture name:
		string texName;
		if (tex_number < 10)
			texName = string("vac0").append(to_string(tex_number));
		else 
			texName = string("vac").append(to_string(tex_number));
		//Log(elvec.first.c_str() << endl);
		auto *tex = xapp().textureStore.getTexture(texName);
		for (int i = 0; i < billboards_per_texture; i++) {
			XMFLOAT3 rnd = xapp().world.getRandomPos();
			b.pos.x = rnd.x;
			b.pos.y = rnd.y;
			b.pos.z = rnd.z;
			billboardEffect.add(texName, b);
		}
	}
	// tests
	//{
	//	XMFLOAT3 a = XMFLOAT3(1, 0, 0);
	//	XMFLOAT3 b = XMFLOAT3(0, 1, 0);
	//	XMVECTOR av = XMLoadFloat3(&a);
	//	XMVECTOR bv = XMLoadFloat3(&b);
	//	XMVECTOR crossv = XMVector3Cross(av, bv);
	//	XMFLOAT3 cross;
	//	XMStoreFloat3(&cross, crossv);
	//	Log("cross " << cross.x);
	//}
}

void TestTextures::update()
{
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
	static bool done = false;
	if (!done && gameTime.getSecondsBetween(startTime, now) > 3) {
		done = true;
		TextureInfo *tex = xapp().textureStore.getTexture("grass");
		if (tex->available && textureFullFrameTest) {
			postEffect.setAlternateFinalFrame(tex->m_srvHeap.Get());
		}
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
	billboardEffect.update();
}

void TestTextures::draw()
{
	linesEffect.draw();
	dotcrossEffect.draw();
	textEffect.draw();
	billboardEffect.draw();
	postEffect.draw();
}

void TestTextures::destroy()
{
	//linesEffect.destroy();
}

static TestTextures testTextures;