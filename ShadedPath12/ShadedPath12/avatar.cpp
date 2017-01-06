#include "stdafx.h"
#include "avatar.h"

// un-comment to load mehes from oculus servers and store locally
//#define LOAD_AVATAR_DATA

// change to base name of meshes saved in data/mesh folder (part before _ )
#define USERID L"413fd8923c71e"


Avatar::Avatar() : XAppBase()
{
	myClass = string(typeid(*this).name());
	xapp().registerApp(myClass, this);
}


Avatar::~Avatar()
{
}

string Avatar::getWindowTitle() {
	return "Avatar";
}

void Avatar::loadLocalAvatarMeshes(wstring userId)
{
	avatarMeshesLoadStartet = true;
	//string userId = w2s(userIdL);
	// look for mesh files for this userId:
	wstring basename = userId + L"_";
	wstring path_string = xapp().findFileForCreation(L"", XApp::FileCategory::MESH);
	//Log("path : " << path_string << endl);
	auto bin_path = std::tr2::sys::path(path_string);
	//Log("path : " << bin_path << endl);
	assert(exists(bin_path));
	auto matches_mask = [basename](const wstring& filename) {
		return regex_search(filename, wregex(basename + L".*\\.b"));
	};
	xapp().objectStore.createGroup("avatar");
	TextureInfo *WhiteTex = xapp().textureStore.getTexture("white");
	float add = 0.0f;
	for (auto it = directory_iterator(bin_path); it != directory_iterator(); ++it)
	{
		const auto& file = it->path();
		wstring filename = file.filename().c_str();
		if (matches_mask(filename)) {
			//Log("file: " << file << endl);
			string meshId = w2s(filename);
			xapp().objectStore.loadObject(filename, meshId);
			XMFLOAT3 p = XMFLOAT3();
			p.x = p.y = p.z = 0.0f;
			p.x = add;
			add += 0.2f;
			xapp().objectStore.addObject("avatar", meshId, p, WhiteTex);
		}
	}
	if (xapp().ovrRendering)	xapp().vr.handleOVRMessages();
	auto grp = xapp().objectStore.getGroup("avatar");
	for (auto & w : *grp) {
		//w.get()->material.specExp = 1.0f;       // no spec color
		//w.get()->material.specIntensity = 0.0f; // no spec color
		w.get()->material.specExp = 20.0f;
		w.get()->material.specIntensity = 700.0f;
		w.get()->material.ambient = XMFLOAT4(1, 1, 1, 1);
	}

	avatarMeshesLoadFinished = true;
}

void Avatar::init()
{
	postEffect.init();
	dotcrossEffect.init();
	linesEffect.init();
	xapp().world.linesEffect = &linesEffect;
	textEffect.init();
	//billboardEffect.init();
	objectEffect.init(&xapp().objectStore, 1);
	float aspectRatio = xapp().aspectRatio;

	// initialize game time to real time:
	gameTime.init(1);
	startTime = gameTime.getRealTime();

	float textSize = 0.5f;
	float lineHeight = 2 * textSize;
	xapp().camera.nearZ = 0.02f;
	xapp().camera.farZ = 500.0f;
	xapp().camera.pos = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	xapp().camera.setSpeed(1.0f); // normal speed
	xapp().camera.fieldOfViewAngleY = 1.289f;
	xapp().world.setWorldSize(2048.0f, 382.0f, 2048.0f);

	textEffect.setSize(textSize);
	dotcrossEffect.setLineLength(6.0f * textSize);
	textEffect.addTextLine(XMFLOAT4(-5.0f, 7 * lineHeight, 0.0f, 0.0f), xapp().buildInfo, Linetext::XY);
	textEffect.addTextLine(XMFLOAT4(-5.0f, 6 * lineHeight, 0.0f, 0.0f), "F1-F2 to change abient light level", Linetext::XY);
	textEffect.addTextLine(XMFLOAT4(-5.0f, 5 * lineHeight, 0.0f, 0.0f), "F3-F4 to change directional+ light level", Linetext::XY);
	fpsLine = textEffect.addTextLine(XMFLOAT4(-5.0f, 4 * lineHeight, 0.0f, 0.0f), "FPS", Linetext::XY);
	framenumLine = textEffect.addTextLine(XMFLOAT4(-5.0f, 3 * lineHeight, 0.0f, 0.0f), "0123456789", Linetext::XY);

	xapp().world.drawCoordinateSystem(XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f), "Origin", textEffect, dotcrossEffect, textSize);

	// textures
	//xapp().textureStore.loadTexture(L"white.dds", "white");
	//xapp().textureStore.loadTexture(L"sample2.dds", "white");
	xapp().textureStore.loadTexture(L"touch_controller.dds", "white");
	//xapp().textureStore.loadTexture(L"rgbtest4.dds", "white");
	//xapp().textureStore.loadTexture(L"413fd8923c71e_1c0685581a5a8aa4.dds", "white");
	//xapp().textureStore.loadTexture(L"413fd8923c71e_951f51e94248778a.dds", "white");
	TextureInfo *GrassTex, *HouseTex, *MetalTex, *WormTex, *PlanetTex, *Meteor1Tex, *AxistestTex;
	TextureInfo *WhiteTex = xapp().textureStore.getTexture("white");
	xapp().lights.init();
	object.material.ambient = XMFLOAT4(1, 1, 1, 1);
	if (true) {
		//xapp().objectStore.loadObject(L"413fd8923c71e_557a26331850dbf.b", "light1", 1.0f);  // left controller
		//xapp().objectStore.loadObject(L"413fd8923c71e_1274b22c61fc48a3.b", "light1", 1.0f);  // torso clothes
		//xapp().objectStore.loadObject(L"413fd8923c71e_450d4eca9f73b9a1.b", "light1", 1.0f);  // glasses
		//xapp().objectStore.loadObject(L"413fd8923c71e_47be498de8d01599.b", "light1", 1.0f);  // hair
		//xapp().objectStore.loadObject(L"413fd8923c71e_6a4ae11446026286.b", "light1", 1.0f);  // left hand
		xapp().objectStore.loadObject(L"413fd8923c71e_6feb9283b780b5a3.b", "light1", 1.0f);  // right controller  LEFT!!!
		//xapp().objectStore.loadObject(L"413fd8923c71e_7f1ca835aeb1b69e.b", "light1", 1.0f);  // large empty cone
		//xapp().objectStore.loadObject(L"413fd8923c71e_8e78d539875b1886.b", "light1", 1.0f);  // open flat ring
		//xapp().objectStore.loadObject(L"413fd8923c71e_af2fdac13313089c.b", "light1", 1.0f);  // face
		//xapp().objectStore.loadObject(L"413fd8923c71e_f82847a6b3ddf1a6.b", "light1", 1.0f);  // right hand
		//xapp().objectStore.loadObject(L"light1.b", "light1", 1.0f);  // right hand
		xapp().objectStore.addObject(object, "light1", XMFLOAT3(0.0f, 0.0f, -0.5f), WhiteTex);
		//object.drawBoundingBox = true;
		//object.drawNormals = true;
		//object.pathDescMove->pathMode = Path_Reverse;
		object.material.specExp = 20.0f;
		object.material.specIntensity = 700.0f;
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
	lights->pointLights[0].pos = XMFLOAT4(7.0f, 10.0f, 8.0f, 1.0f);
	lights->pointLights[0].range_reciprocal = 1.0f / 40.0f;
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

void Avatar::update()
{
	gameTime.advanceTime();
	LONGLONG now = gameTime.getRealTime();
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
	//Log("obj pos " << object.pos().x << endl);

	if (avatarMeshesLoadStartet == false) {
		loadLocalAvatarMeshes(USERID);
	}
	if(xapp().ovrRendering)	xapp().vr.handleOVRMessages();
	auto grp = xapp().objectStore.getGroup("avatar");
	for (auto & w : *grp) {
		w->update();
	}

}

void Avatar::draw()
{
	//linesEffect.draw();
	//dotcrossEffect.draw();
	//textEffect.draw();
	//billboardEffect.draw();
	//object.draw();
	auto grp = xapp().objectStore.getGroup("avatar");
	for (auto & w : *grp) {
		w->draw();
	}
	postEffect.draw();
}

void Avatar::destroy()
{
}

static Avatar Avatar;