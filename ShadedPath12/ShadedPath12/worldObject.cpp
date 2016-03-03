#include "stdafx.h"

// debug log level:
static const bool debug_basic = false;
static const bool debug_uv = false;

void MeshLoader::loadBinaryAsset(wstring filename, Mesh* mesh, float scale) {
	ifstream bfile(filename.c_str(), ios::in | ios::binary);
	if (debug_basic) Log("file opened: " << filename.c_str() << "\n");

	// basic assumptions about data types:
	if (sizeof(char) != 1 || sizeof(unsigned long) != 4 || sizeof(float) != 4) {
		Log("invalid data type lengths detected. compile with different options.\n");
		return;
	}

	// mesh processing:
	int mode;
	bfile.read((char*)&mode, 4);

	// read animation clips
	if (mode == 1) {
		int numAniClips;
		bfile.read((char*)&numAniClips, 4);
		//aniClips = new AnimationClip[numAniClips];
		for (int i = 0; i < numAniClips; i++) {
			int numAnimationNameLength;
			bfile.read((char*)&numAnimationNameLength, 4);
			char *clip_name = new char[numAnimationNameLength + 1];
			clip_name[numAnimationNameLength] = '\0';
			bfile.read((char*)clip_name, numAnimationNameLength);
			Log("animation clip name: " << clip_name << "\n");
			AnimationClip clip;
			clip.name = std::string(clip_name);
			Action *action = new Action();
			action->name = std::string(clip_name);// 'Armature'
			int numJoints;
			bfile.read((char*)&numJoints, 4);
			clip.numBones = numJoints;
			for (int j = 0; j < numJoints; j++) {
				int parentId;
				bfile.read((char*)&parentId, 4);
				clip.parents[j] = parentId;
				Curve curve;
				float f[16];
				for (int n = 0; n < 16; n++) {
					bfile.read((char*)&f[n], 4);
				}
				XMFLOAT4X4 m = XMFLOAT4X4(f);
				//XMMATRIX m4 = XMLoadFloat4x4(&m);
				clip.invBindMatrices.push_back(m);
				int keyframes;
				bfile.read((char*)&keyframes, 4);
				for (int m = 0; m < keyframes; m++) {
					float time;
					bfile.read((char*)&time, 4);
					BezTriple b;
					b.isBoneAnimation = true;
					b.cp[0] = time;
					float f2[16];
					for (int n = 0; n < 16; n++) {
						bfile.read((char*)&f2[n], 4);
						b.transMatrix[n] = f2[n];
					}
					//XMFLOAT4X4 m = XMFLOAT4X4(f);
					//b.poseMatrix = m;
					curve.bezTriples.push_back(b);
				}
				action->curves.push_back(curve);
			}
			mesh->clips[clip.name] = clip;
			if (true /*actions != NULL*/)
				mesh->actions[action->name] = *action;
			delete action;
			delete clip_name;
		}
	}
	// read number and verts
	int numVerts;
	bfile.read((char*)&numVerts, 4);
	float *verts = new float[numVerts];
	bfile.read((char*)verts, 4 * numVerts);
	//oss << "reading float: " << verts[0] << "\n";
	//Blender::Log(oss.str());

	// texture coords:
	int numTex = (numVerts * 2) / 3;
	float *tex = new float[numTex];
	bfile.read((char*)tex, 4 * numTex);

	// normals:
	int numNormals = numVerts;
	float *norm = new float[numNormals];
	bfile.read((char*)norm, 4 * numNormals);

	// bones and their weights:
	int *bones = 0;
	float *bone_weights = 0;
	if (mode == 1) {
		// we have skinned animation weights
		// for all vertices we have one int packed with the indices of the 4 bone influencing it
		int numBonePacks = numTex / 2;
		bones = new int[numBonePacks];
		bfile.read((char*)bones, 4 * numBonePacks);
		// for each bonePack we have the 4 weights:
		bone_weights = new float[4 * numBonePacks];
		bfile.read((char*)bone_weights, 4 * 4 * numBonePacks);
	}

	// vertices index buffer
	int numIndex;
	bfile.read((char*)&numIndex, 4);
	int *ints = new int[numIndex];
	bfile.read((char*)ints, 4 * numIndex);
	int numAnimationNameLength = 0;
	bfile.read((char*)&numAnimationNameLength, 4);
	// load animations:
	Action *action = 0;
	if (numAnimationNameLength != 0) {
		char *ani_name = new char[numAnimationNameLength + 1];
		ani_name[numAnimationNameLength] = '\0';
		bfile.read((char*)ani_name, numAnimationNameLength);
		Log("animation name: " << ani_name << "\n");
		action = new Action();
		action->name = std::string(ani_name);
		for (int i = 0; i < 9; i++) {
			Curve curve;
			int numSegments;
			bfile.read((char*)&numSegments, 4);
			for (int j = 0; j < numSegments; j++) {
				BezTriple b;
				b.isBoneAnimation = false;
				bfile.read((char*)&b.h1[0], 4);
				bfile.read((char*)&b.h1[1], 4);
				bfile.read((char*)&b.cp[0], 4);
				bfile.read((char*)&b.cp[1], 4);
				bfile.read((char*)&b.h2[0], 4);
				bfile.read((char*)&b.h2[1], 4);
				curve.bezTriples.push_back(b);
			}
			action->curves.push_back(curve);
		}
		mesh->actions[action->name] = *action;
		delete action;
		delete ani_name;
	}
	bfile.close();
	// process loaded mesh:
	//Mesh m;
	//Mesh *mesh = &this->mesh;
	mesh->initBoundigBox();
	mesh->numVertices = numVerts / 3;
	mesh->vertices.reserve(mesh->numVertices);
	mesh->numIndexes = numIndex;
	mesh->indexes.reserve(mesh->numIndexes);

	WorldObjectVertex::VertexTextured vertex;
	for (int i = 0; i < mesh->numVertices; i++) {
		vertex.Pos.x = verts[i * 3] * scale;
		vertex.Pos.y = verts[i * 3 + 1] * scale;
		vertex.Pos.z = verts[i * 3 + 2] * scale;
		mesh->addToBoundingBox(vertex.Pos);
		vertex.Normal.x = norm[i * 3];
		vertex.Normal.y = norm[i * 3 + 1];
		vertex.Normal.z = norm[i * 3 + 2];
		vertex.Tex.x = tex[i * 2];
		vertex.Tex.y = tex[i * 2 + 1];
		mesh->vertices.push_back(vertex);
	}
	std::vector<WorldObjectVertex::VertexSkinned> *skinnedVertices = 0;
	if (mode == 1) {
		mesh->skinnedVertices.reserve(mesh->numVertices);
		WorldObjectVertex::VertexSkinned vertex;
		for (int i = 0; i < mesh->numVertices; i++) {
			vertex.Pos.x = verts[i * 3] * scale;
			vertex.Pos.y = verts[i * 3 + 1] * scale;
			vertex.Pos.z = verts[i * 3 + 2] * scale;
			mesh->addToBoundingBox(vertex.Pos);
			vertex.Tex.x = tex[i * 2];
			vertex.Tex.y = tex[i * 2 + 1];
			vertex.Normal.x = norm[i * 3];
			vertex.Normal.y = norm[i * 3 + 1];
			vertex.Normal.z = norm[i * 3 + 2];
			vertex.Weights.x = bone_weights[i * 4];
			vertex.Weights.y = bone_weights[i * 4 + 1];
			vertex.Weights.z = bone_weights[i * 4 + 2];
			vertex.Weights.w = bone_weights[i * 4 + 3];
			unsigned int ui = bones[i];
			vertex.BoneIndices[0] = ui & 0xff;
			vertex.BoneIndices[1] = (ui & 0xff00) >> 8;
			vertex.BoneIndices[2] = (ui & 0xff0000) >> 16;
			vertex.BoneIndices[3] = (ui & 0xff000000) >> 24;
			mesh->skinnedVertices.push_back(vertex);
		}
	}
	for (int i = 0; i < mesh->numIndexes; i++) {
		mesh->indexes.push_back(ints[i]);
	}

	// free mem
	delete verts;
	delete ints;
	delete tex;
	delete norm;
	if (mode == 1) {
		delete bones;
		delete bone_weights;
		//delete skinnedVertices;
	}
}

void Mesh::createVertexAndIndexBuffer(WorldObjectEffect *worldObjectEffect) {
	worldObjectEffect->createAndUploadVertexBuffer(this);

}

void WorldObject::calculateBoundingBox(BoundingBox &box, bool maximise) {
	//XMFLOAT3 bboxVertexMin, bboxVertexMax;
	if (!maximise) {
		bboxVertexMin.x = bboxVertexMin.y = bboxVertexMin.z = FLT_MAX;
		bboxVertexMax.x = bboxVertexMax.y = bboxVertexMax.z = -FLT_MAX;
	}
	for (auto v : mesh->vertices) {
		XMFLOAT3 p = v.Pos;
		if (p.x < bboxVertexMin.x) bboxVertexMin.x = p.x;
		if (p.y < bboxVertexMin.y) bboxVertexMin.y = p.y;
		if (p.z < bboxVertexMin.z) bboxVertexMin.z = p.z;
		if (p.x > bboxVertexMax.x) bboxVertexMax.x = p.x;
		if (p.y > bboxVertexMax.y) bboxVertexMax.y = p.y;
		if (p.z > bboxVertexMax.z) bboxVertexMax.z = p.z;
	}
	XMVECTOR v1, v2, vp;
	vp = XMLoadFloat3(&pos());
	v1 = XMLoadFloat3(&bboxVertexMin);
	v2 = XMLoadFloat3(&bboxVertexMax);
	BoundingBox currentBox;
	box.CreateFromPoints(box, vp + v1, vp + v2);
	//if (!maximise) {
	//	box = currentBox;
	//} else {
	//	// bounding box always increases with bone skinning to find hull enclosing all moves
	//	box.Extents.x = max(abs(box.Extents.x), currentBox.Extents.x);
	//	box.Extents.y = max(abs(box.Extents.y), currentBox.Extents.y);
	//	box.Extents.z = max(abs(box.Extents.z), currentBox.Extents.z);
	//}
}

// initialize bbox vertices to FLT_MAX / FLT_MIN for easy point adding
void Mesh::initBoundigBox() {
	bboxVertexMin.x = bboxVertexMin.y = bboxVertexMin.z = FLT_MAX;
	bboxVertexMax.x = bboxVertexMax.y = bboxVertexMax.z = -FLT_MAX;
}

void Mesh::addToBoundingBox(XMFLOAT3 p) {
	if (p.x < bboxVertexMin.x) bboxVertexMin.x = p.x;
	if (p.y < bboxVertexMin.y) bboxVertexMin.y = p.y;
	if (p.z < bboxVertexMin.z) bboxVertexMin.z = p.z;
	if (p.x > bboxVertexMax.x) bboxVertexMax.x = p.x;
	if (p.y > bboxVertexMax.y) bboxVertexMax.y = p.y;
	if (p.z > bboxVertexMax.z) bboxVertexMax.z = p.z;
}

void Mesh::getBoundingBox(BoundingBox &b) {
	XMVECTOR v1, v2;
	v1 = XMLoadFloat3(&bboxVertexMin);
	v2 = XMLoadFloat3(&bboxVertexMax);
	b.CreateFromPoints(b, v1, v2);
}


void WorldObject::forceBoundingBox(BoundingBox box) {
	boundingBox = box;
	boundingBoxAlreadySet = true;
}

void WorldObject::getBoundingBox(BoundingBox &box) {
	if (boundingBoxAlreadySet) box = boundingBox;
	else mesh->getBoundingBox(box);
}

void WorldObject::update() {
	if (this->pathDescBone) {
		xapp().world.path.updateScene(pathDescBone, this, xapp().gametime.getTimeAbs());
	}
	if (drawBoundingBox) {
		//unique_ptr<Lines>& linesEffect = (unique_ptr<Lines>&)Effect::getUniquePtr(Effect::LINES);
		BoundingBox bbox;
		//mesh->getBoundingBox(bbox);
		getBoundingBox(bbox);
		XMVECTOR r = XMLoadFloat3(&rot());
		XMVECTOR t = XMLoadFloat3(&pos());
		//XMVECTOR r2 = XMQuaternionRotationRollPitchYaw(0.0f/*pos().x*/, 0.0f/*pos().y*/, 0.0f/*pos().z*/);//pitch, yaw, roll); // TODO
		XMVECTOR r2 = XMQuaternionRotationRollPitchYaw(rot().y, rot().x, rot().z);//pitch, yaw, roll); // TODO
		bbox.Transform(bbox, 1.0f, r2, t);
		xapp().world.drawBox(bbox);
	}
	if (drawNormals) {
		//unique_ptr<Lines>& linesEffect = (unique_ptr<Lines>&)Effect::getUniquePtr(Effect::LINES);
		XMVECTOR r2 = XMQuaternionRotationRollPitchYaw(rot().y, rot().x, rot().z);
		// rotate all normals then draw them for each vertex
		vector<LineDef> lines;
		for (auto &v : mesh->vertices) {
			// rotate normal
			XMVECTOR rotNorm = XMLoadFloat3(&v.Normal);
			rotNorm = XMVector3Rotate(rotNorm, r2);
			//XMFLOAT3 n;
			//XMStoreFloat3(&n, rotNorm);
			// rotate vertex
			XMVECTOR rotV = XMLoadFloat3(&v.Pos);
			rotV = XMVector3Rotate(rotV, r2);
			// translate vertex:
			XMVECTOR t = XMLoadFloat3(&pos());
			rotV = rotV + t;
			XMFLOAT3 vt;
			XMStoreFloat3(&vt, rotV);
			XMFLOAT3 vtEnd;
			XMStoreFloat3(&vtEnd, rotV + rotNorm);

			LineDef line;
			line.color = Colors::Silver;
			line.start = vt;
			line.end = vtEnd;
			lines.push_back(line);
		}
		xapp().world.linesEffect->addOneTime(lines);

	}
}

float getVLen(XMFLOAT3 &p0, XMFLOAT3 &p1) {
	XMVECTOR pv0 = XMLoadFloat3(&p0);
	XMVECTOR pv1 = XMLoadFloat3(&p1);
	return XMVectorGetX(XMVector3Length(pv1-pv0));
}

void WorldObject::draw() {
	WorldObjectEffect *worldObjectEffect = xapp().objectStore.getWorldObjectEffect();
	// quaternion
	XMVECTOR q = XMQuaternionIdentity();
	//XMVECTOR q = XMVectorSet(rot().x, rot().y, rot().z, 0.0f);
	XMVECTOR q_origin = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMMATRIX rotateM = XMMatrixRotationRollPitchYaw(rot().y, rot().x, rot().z);
	q = XMQuaternionRotationMatrix(rotateM);
	q = XMQuaternionNormalize(q);
	// scalar
	XMVECTOR s = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	// translation
	XMVECTOR t = XMVectorSet(pos().x, pos().y, pos().z, 0.0f);
	// toWorld matrix:
	XMMATRIX toWorld = XMMatrixAffineTransformation(s, q_origin, q, t);

	xapp().camera.viewTransform();
	xapp().camera.projectionTransform();
	BoundingBox box;
	getBoundingBox(box);
	int visible = xapp().camera.calculateVisibility(box, toWorld);  // TODO first move, then calc visibility
																   //Log("visible == " << visible << endl);
	if (visible == 0) return;
	XMMATRIX p = XMLoadFloat4x4(&xapp().camera.projection);
	XMMATRIX v = XMLoadFloat4x4(&xapp().camera.view);
//	XMMATRIX wvp = toWorld * (v * p);
	XMMATRIX wvp = toWorld;
	wvp = XMMatrixTranspose(wvp);
	// TODO adjust finalWVP to left/right eye
	XMFLOAT4X4 finalWvp;
	XMStoreFloat4x4(&finalWvp, wvp);
	//TextureInfo *info = xapp().textureStore.getTexture(this->textureID);
	TextureInfo *info = this->textureID;
	if (action) {
		//move object
		XMFLOAT3 pos, rot;
		xapp().world.path.getPos(*this, xapp().gametime.getTimeAbs(), pos, rot);
		pos.x = objectStartPos.x + pos.x * scale;
		pos.y = objectStartPos.y + pos.y * scale;
		pos.z = objectStartPos.z + pos.z * scale;
		float diff = getVLen(this->pos(), pos);
		double t = xapp().gametime.getTimeAbs();
		//Log(" diff " << setprecision(9) << diff << " " << t << endl);
		if (diff < 0.00001f) {
			//Log(" diff " << diff << endl);
		}
		this->pos() = pos;
		this->rot() = rot;
		worldObjectEffect->draw(mesh, mesh->vertexBuffer, mesh->indexBuffer, finalWvp, mesh->numIndexes, info, material, alpha);
		//centralObject->draw(&path, &worldUtil, &mTerrain, md3dDevice, md3dImmediateContext, *gCamera, getGameTimeAbs());
	}
	else {
		if (mesh->skinnedVertices.size() > 0) {
			for (int skV = 0; skV < (int)mesh->skinnedVertices.size(); skV++) {
				//if (skV > mesh->skinnedVertices.size()-60)
				//path->recalculateBoneAnimation(this->pathDescBone, this, worldUtil, terrain, device, dc, cam, time);

				WorldObjectVertex::VertexSkinned *v = &mesh->skinnedVertices[skV];
				//XMVECTOR vfinal = Path::skin(v, &mesh->clips["Armature"], &boneAction->curves, pathDescBone->curSegment, 7.0f, pathDescBone->percentage);
				XMVECTOR vfinal = xapp().world.path.skin(v, pathDescBone);
				XMFLOAT3 vfinal_flo;
				XMStoreFloat3(&vfinal_flo, vfinal);
				mesh->vertices[skV].Pos = vfinal_flo;
				//if (isCopiedObject && skV < 1000) {
				//	mesh->vertices[skV].Pos.x = mesh->vertices[skV].Pos.x - 5;
				//	//mesh->vertices[skV].Pos.y = 0.0f;
				//	//mesh->vertices[skV].Pos.z = 0.0f;
				//}
			}
			//if (vertexBuffer) ReleaseCOM(vertexBuffer);
			//if (indexBuffer) ReleaseCOM(indexBuffer);
			//prepareDxResources(device, dc);
			mesh->createVertexAndIndexBuffer(worldObjectEffect);
			worldObjectEffect->draw(mesh, mesh->vertexBuffer, mesh->indexBuffer, finalWvp, mesh->numIndexes, info, material, alpha);
		}
		else {
			// no skinned vertices
			worldObjectEffect->draw(mesh, mesh->vertexBuffer, mesh->indexBuffer, finalWvp, mesh->numIndexes, info, material, alpha);
		}
	}
}

void WorldObject::setAction(string name) {
	Log(" action == " << action << endl);
	Action *action = &mesh->actions[name];
	Log(" action == " << action << endl);
	assert(action->name.compare(name) == 0); // assert that action was found
	Log(" action...isbone == " << action->curves[0].bezTriples[0].isBoneAnimation << endl);
	Log(" action..curves.size == " << action->curves.size() << endl);
	PathDesc* d;
	if (action->curves[0].bezTriples[0].isBoneAnimation) {
		this->boneAction = action;
		this->pathDescBone = new PathDesc();
		d = this->pathDescBone;
		d->isBoneAnimation = true;
		d->clip = &mesh->clips[name];
		d->lastCurves = 0;
		d->lastPercentage = -1.0f;
		d->lastSegment = -1;
		assert(d->clip != 0);
	}
	else {
		this->action = action;
		this->pathDescMove = new PathDesc();
		d = this->pathDescMove;
		d->isBoneAnimation = false;
	}
	d->speed = 1.0f;
	d->numSegments = (int)action->curves[0].bezTriples.size();
	d->numSegments--;
	d->curSegment = 0;
	d->starttime = 0L;
	//d->starttime_f = 0.0f;
	d->currentPos = XMFLOAT3(0.0, 0.0f, 0.0f);
	d->fps = 25.0f; // blender default
	d->segments = NULL;
	d->pathMode = Path_Reverse;
	d->currentReverseRun = false;
	Log(" this->action == " << this->action << endl);
	Log(" this->boneAction == " << this->boneAction << endl);
}

XMFLOAT3& WorldObject::pos() {
	return _pos;
}

XMFLOAT3& WorldObject::rot() {
	return _rot;
}

WorldObject::WorldObject() {
	pathDescBone = nullptr;
	scale = 1.0f;
	drawBoundingBox = false;
	drawNormals = false;
}

WorldObject::~WorldObject() {
	if (pathDescBone) {
		if (pathDescBone->segments) delete[] pathDescBone->segments;
		delete pathDescBone;
	}
	if (pathDescMove) {
		if (pathDescMove->segments) delete[] pathDescMove->segments;
		delete pathDescMove;
	}
}

// object store:
void WorldObjectStore::loadObject(wstring filename, string id, float scale) {
	MeshLoader loader;
	wstring binFile = xapp().findFile(filename.c_str(), XApp::MESH);
	Mesh mesh;
	meshes[id] = mesh;
	loader.loadBinaryAsset(binFile, &meshes[id], scale);
	meshes[id].createVertexAndIndexBuffer(this->objectEffect);
}

void WorldObjectStore::createGroup(string groupname) {
	if (groups.count(groupname) > 0) return;  // do not recreate groups
											  //vector<WorldObject> *newGroup = groups[groupname];
	const auto &newGroup = groups[groupname];
	Log(" ---groups size " << groups.size() << endl);
	Log(" ---newGroup vecor size " << newGroup.size() << endl);
}

const vector<unique_ptr<WorldObject>> *WorldObjectStore::getGroup(string groupname) {
	if (groups.count(groupname) == 0) return nullptr;
	return &groups[groupname];
}

void WorldObjectStore::addObject(string groupname, string id, XMFLOAT3 pos, TextureID tid) {
	assert(groups.count(groupname) > 0);
	auto &grp = groups[groupname];
	grp.push_back(unique_ptr<WorldObject>(new WorldObject()));
	WorldObject *w = grp[grp.size() - 1].get();
	addObjectPrivate(w, id, pos, tid);
}

void WorldObjectStore::addObject(WorldObject &w, string id, XMFLOAT3 pos, TextureID tid) {
	addObjectPrivate(&w, id, pos, tid);
}

void WorldObjectStore::addObjectPrivate(WorldObject *w, string id, XMFLOAT3 pos, TextureID tid) {
	assert(meshes.count(id) > 0);
	Mesh &mesh = meshes.at(id);
	w->pos() = pos;
	w->objectStartPos = pos;
	w->mesh = &mesh;
	w->textureID = tid;
	//w.wireframe = false;
	w->alpha = 1.0f;
	w->action = nullptr;
}

void WorldObjectStore::setWorldObjectEffect(WorldObjectEffect *weff) {
	this->objectEffect = weff;
}