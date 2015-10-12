#include "stdafx.h"


World::World(XApp *xapp) {
	this->xapp = xapp;
	//WorldObject::xapp = xapp;
	//Sound::xapp = xapp;
}


World::~World() {
}


void makeline(LineDef &ld, XMFLOAT3 start, XMFLOAT3 end) {
	ld.start = XMFLOAT3(start.x, start.y, start.z);
	ld.end = XMFLOAT3(end.x, end.y, end.z);
}

void World::drawBox(BoundingBox &box, LinesEffect *linestEffect) {
	XMFLOAT3 corners[8];
	box.GetCorners(&corners[0]);
	vector<LineDef> v;
	LineDef line;
	line.color = Colors::White;
	makeline(line, corners[0], corners[1]);	v.push_back(line);
	makeline(line, corners[1], corners[2]);	v.push_back(line);
	makeline(line, corners[2], corners[3]);	v.push_back(line);
	makeline(line, corners[3], corners[0]);	v.push_back(line);
	makeline(line, corners[4], corners[5]);	v.push_back(line);
	makeline(line, corners[5], corners[6]);	v.push_back(line);
	makeline(line, corners[6], corners[7]);	v.push_back(line);
	makeline(line, corners[7], corners[4]);	v.push_back(line);
	makeline(line, corners[0], corners[4]);	v.push_back(line);
	makeline(line, corners[1], corners[5]);	v.push_back(line);
	makeline(line, corners[2], corners[6]);	v.push_back(line);
	makeline(line, corners[3], corners[7]);	v.push_back(line);
	linestEffect->addOneTime(v);
}

/*void World::drawCoordinateSystem(XMFLOAT4 point, string label, unique_ptr<Linetext>& linetextEffect, unique_ptr<Dotcross>& dotcrossEffect, float charHeight) {
	wstring l = string2wstring(label);
	//Log("label for drawCoordinateSystem: " << l << "\n");
	XMFLOAT4 labelPos = point;
	float a = (1.0f * charHeight) / 1.73205f;
	labelPos.x += charHeight;
	labelPos.y += charHeight;;
	XMMATRIX rot = XMMatrixRotationRollPitchYaw(0, XM_PIDIV4, -XM_PIDIV4);
	XMFLOAT4X4 r;
	XMStoreFloat4x4(&r, rot);
	int labelLine = linetextEffect->addTextLine(labelPos, label, r);
	XMFLOAT4 xPos = point;
	XMFLOAT4 yPos = point;
	XMFLOAT4 zPos = point;
	float b = charHeight * 3;
	xPos.x += b;
	int xLine = linetextEffect->addTextLine(xPos, "-> x", Linetext::XY);
	yPos.y += b;
	int yLine = linetextEffect->addTextLine(yPos, "-> y", Linetext::YX);
	zPos.z += b;
	int zLine = linetextEffect->addTextLine(zPos, "-> z", Linetext::ZY);
	vector<XMFLOAT4> pts;
	pts.push_back(point);
	dotcrossEffect->update(pts);
}*/

void World::createGridXZ(Grid& grid, bool linesmode) {
	int zLineCount = grid.depthCells + 1;
	int xLineCount = grid.widthCells + 1;

	float halfWidth = grid.width / 2.0f;
	float halfDepth = grid.depth / 2.0f;

	float xstart = grid.center.x - halfWidth;
	float xend = grid.center.x + halfWidth;
	float xdiff = grid.width / grid.widthCells;
	float zstart = grid.center.z - halfDepth;
	float zend = grid.center.z + halfDepth;
	float zdiff = grid.depth / grid.depthCells;

	float x, z;
	if (linesmode == true) {
		LineDef line;
		line.color = Colors::Red;
		for (int xcount = 0; xcount < xLineCount; xcount++) {
			x = xstart + xcount * xdiff;
			XMFLOAT3 p1(x, grid.center.y, zstart);
			XMFLOAT3 p2(x, grid.center.y, zend);
			line.start = p1;
			line.end = p2;
			grid.lines.push_back(line);
			//grid.zLineEndpoints.push_back(p1);
			//grid.zLineEndpoints.push_back(p2);
		}
		for (int zcount = 0; zcount < zLineCount; zcount++) {
			z = zstart + zcount * zdiff;
			XMFLOAT3 p1(xstart, grid.center.y, z);
			XMFLOAT3 p2(xend, grid.center.y, z);
			line.start = p1;
			line.end = p2;
			grid.lines.push_back(line);
			//grid.xLineEndpoints.push_back(p1);
			//grid.xLineEndpoints.push_back(p2);
		}
	} else {
		float du = 1.0f / (grid.widthCells);
		float dv = 1.0f / (grid.depthCells);
		for (int xcount = 0; xcount < xLineCount; xcount++) {
			x = xstart + xcount * xdiff;
			for (int zcount = 0; zcount < zLineCount; zcount++) {
				z = zstart + zcount * zdiff;
				XMFLOAT3 v(x, grid.center.y, z);
				grid.vertices.push_back(v);
				XMFLOAT2 t(du * xcount, dv * zcount);
				grid.tex.push_back(t);
			}
		}
	}
}

Grid* World::createWorldGrid(float lineGap) {
	grid.center = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	grid.depth = sizez;
	grid.width = sizex;
	grid.depthCells = (int)(grid.depth / lineGap);
	grid.widthCells = (int)(grid.width / lineGap);
	//createGridXZ(grid);
	float low = 0.0f; // -sizey / 2.0f;
	float high = sizey;// / 2.0f;
	float step = lineGap;
	for (float y = low; y <= high; y += step) {
		grid.center.y = y;
//		createGridXZ(grid);
	}
	grid.center.y = 0;
	createGridXZ(grid);
	grid.center.y = low;
	createGridXZ(grid);
	grid.center.y = high;
	createGridXZ(grid);
	return &grid;
}

// lights:
/*
void World::addPointLight(PointLight pLight, int id) {
	if (id == -1) {
		id = pointLights.size();
	}
	PointLightStored p;
	p.l = pLight;
	p.id = id;
	pointLights.push_back(p);
}

void World::addDirectionalLight(DirectionalLight dLight, int id) {
	if (id == -1) {
		id = directionalLights.size();
	}
	DirectionalLightStored d;
	d.l = dLight;
	d.id = id;
	directionalLights.push_back(d);
}

PointLight& World::getPointLight(int id) {
	return pointLights[id].l;
}

DirectionalLight& World::getDirectionalLight(int id) {
	return directionalLights[id].l;
}
*/

// textures:

/*
void World::setTextures(vector<TextureInfo> &ti) {
	for (auto t : ti) {
		//textureInfos.push_back(t);
		textureMap[t.textureID] = t;
	}
	// read the texture files:
	for (auto t : textureMap) {
		//Log("texture load " << t.filename << endl);
		ComPtr<ID3D11Texture2D> tex;
		wstring filename = xapp->findFile(t.second.filename, XApp::TEXTURE);
		ThrowIfFailed(CreateDDSTextureFromFile(xapp->device.Get(), filename.c_str(), (ID3D11Resource**)(tex.GetAddressOf()), &t.second.texSRV));
		// copy data from local copy to original stored in map:
		textureMap[t.first].texSRV = t.second.texSRV;
		D3D11_TEXTURE2D_DESC td;
		tex->GetDesc(&td);
	}
}

TextureInfo *World::getTexture(TextureID id) {
	assert(textureMap.count(id) > 0);
	return &textureMap[id];
}

void World::createRandomTexture1DSRV(ComPtr<ID3D11ShaderResourceView> &randomTexSRV) {
	// 
	// Create the random data.
	//
	XMFLOAT4 randomValues[1024];

	for (int i = 0; i < 1024; ++i)
	{
		randomValues[i].x = MathHelper::RandF(-1.0f, 1.0f);
		randomValues[i].y = MathHelper::RandF(-1.0f, 1.0f);
		randomValues[i].z = MathHelper::RandF(-1.0f, 1.0f);
		randomValues[i].w = MathHelper::RandF(-1.0f, 1.0f);
	}

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = randomValues;
	initData.SysMemPitch = 1024 * sizeof(XMFLOAT4);
	initData.SysMemSlicePitch = 0;

	//
	// Create the texture.
	//
	D3D11_TEXTURE1D_DESC texDesc;
	texDesc.Width = 1024;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	texDesc.ArraySize = 1;

	ID3D11Texture1D* randomTex = 0;
	ThrowIfFailed(xapp->device.Get()->CreateTexture1D(&texDesc, &initData, &randomTex));

	//
	// Create the resource view.
	//
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	viewDesc.Texture1D.MipLevels = texDesc.MipLevels;
	viewDesc.Texture1D.MostDetailedMip = 0;

	//ID3D11ShaderResourceView* randomTexSRV = 0;
	ThrowIfFailed(xapp->device.Get()->CreateShaderResourceView(randomTex, &viewDesc, &randomTexSRV));

	randomTex->Release();
}
*/