#include "stdafx.h"

// debug log level:
static const bool debug_basic = false;
static const bool debug_uv = false;

XMFLOAT4X4 toLeft(XMFLOAT4X4 r) {
	return r;
	XMFLOAT4X4 l = r;
	l._31 *= -1.0f;
	l._32 *= -1.0f;
	l._33 *= -1.0f;
	l._34 *= -1.0f;

	l._13 *= -1.0f;
	l._23 *= -1.0f;
	l._33 *= -1.0f;
	l._43 *= -1.0f;
	return l;
}

ObjectDef* MeshLoader::findWithName(vector<ObjectDef> &objectDefs, string meshName)
{
	for (auto& od : objectDefs) {
		Mesh* m = od.mesh;
		if (meshName.compare(m->nameFromCollada) == 0) {
			// found
			return &od;
		}
	}
	return nullptr;
}

void MeshLoader::loadBinaryAssets(wstring filename, vector<ObjectDef> &objectDefs) {
	ifstream bfile(filename.c_str(), ios::in | ios::binary);
	if (debug_basic) Log("file opened: " << filename.c_str() << "\n");

	// basic assumptions about data types:
	if (sizeof(char) != 1 || sizeof(unsigned long) != 4 || sizeof(float) != 4) {
		Log("invalid data type lengths detected. compile with different options.\n");
		return;
	}

	// load number of meshes:
	int meshCount;
	bfile.read((char*)&meshCount, 4);
	if (meshCount != objectDefs.size()) {
		Log("mesh count in binary: " << meshCount << " parameters list: " << objectDefs.size() << endl);
		Error(L"Cannot load meshes from binary file. Parameters do not match mesh count.");
	}
	int count = 0;
	while (count < meshCount) {
		count++;
		int numMeshNameLength;
		bfile.read((char*)&numMeshNameLength, 4);
		char* mesh_name = new char[numMeshNameLength + 1];
		mesh_name[numMeshNameLength] = '\0';
		bfile.read((char*)mesh_name, numMeshNameLength);
		string meshName = std::string(mesh_name);
		ObjectDef *odef = findWithName(objectDefs, meshName);
		Mesh* mesh = odef->mesh;
		float scale = odef->scale;
		XMFLOAT3* displacement = odef->displacement;
		if (mesh == nullptr) {
			Log("not found in binary file: " << meshName.c_str() << endl);
			Error(L"cannot find requested collada mesh in binary file.");
		}
		Log("loading mesh: " << mesh->nameFromCollada.c_str() << endl);
		delete mesh_name;
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
				char* clip_name = new char[numAnimationNameLength + 1];
				clip_name[numAnimationNameLength] = '\0';
				bfile.read((char*)clip_name, numAnimationNameLength);
				Log("animation clip name: " << clip_name << "\n");
				AnimationClip clip;
				clip.name = std::string(clip_name);
				Action* action = new Action();
				action->name = std::string(clip_name);// 'Armature'
				int numJoints;
				bfile.read((char*)&numJoints, 4);
				clip.numBones = numJoints;
				for (int j = 0; j < numJoints; j++) {
					int parentId;
					bfile.read((char*)&parentId, 4);
					clip.parents[j] = parentId;
					Curve curve;
					// read inverse bind matrix
					float f[16];
					for (int n = 0; n < 16; n++) {
						bfile.read((char*)&f[n], 4);
					}
					XMFLOAT4X4 m = XMFLOAT4X4(f);
					//XMMATRIX m4 = XMLoadFloat4x4(&m);
					clip.invBindMatrices.push_back(toLeft(m));
					// read bone bind pose (not used in animation - just to be able to display bone structure
					for (int n = 0; n < 16; n++) {
						bfile.read((char*)&f[n], 4);
					}
					m = XMFLOAT4X4(f);
					clip.boneBindPoseMatrices.push_back(toLeft(m));
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
		float* verts = new float[numVerts];
		bfile.read((char*)verts, 4 * numVerts);
		//oss << "reading float: " << verts[0] << "\n";
		//Blender::Log(oss.str());

		// texture coords:
		int numTex = (numVerts * 2) / 3;
		float* tex = new float[numTex];
		bfile.read((char*)tex, 4 * numTex);

		// normals:
		int numNormals = numVerts;
		float* norm = new float[numNormals];
		bfile.read((char*)norm, 4 * numNormals);

		// bones and their weights:
		int* bones = 0;
		float* bone_weights = 0;
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
		int* ints = new int[numIndex];
		bfile.read((char*)ints, 4 * numIndex);
		int numAnimationNameLength = 0;
		bfile.read((char*)&numAnimationNameLength, 4);
		// load animations:
		Action* action = 0;
		if (numAnimationNameLength != 0) {
			char* ani_name = new char[numAnimationNameLength + 1];
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
			if (displacement != nullptr) {
				vertex.Pos.x += displacement->x;
				vertex.Pos.y += displacement->y;
				vertex.Pos.z += displacement->z;
			}
			mesh->addToBoundingBox(vertex.Pos);
			vertex.Normal.x = norm[i * 3];
			vertex.Normal.y = norm[i * 3 + 1];
			vertex.Normal.z = norm[i * 3 + 2];
			vertex.Tex.x = tex[i * 2];
			vertex.Tex.y = tex[i * 2 + 1];
			mesh->vertices.push_back(vertex);
		}
		std::vector<WorldObjectVertex::VertexSkinned>* skinnedVertices = 0;
		if (mode == 1) {
			mesh->skinnedVertices.reserve(mesh->numVertices);
			WorldObjectVertex::VertexSkinned vertex;
			for (int i = 0; i < mesh->numVertices; i++) {
				vertex.Pos.x = verts[i * 3] * scale;
				vertex.Pos.y = verts[i * 3 + 1] * scale;
				vertex.Pos.z = verts[i * 3 + 2] * scale;
				if (displacement != nullptr) {
					vertex.Pos.x += displacement->x;
					vertex.Pos.y += displacement->y;
					vertex.Pos.z += displacement->z;
				}
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
	bfile.close();
}

//void Mesh::createVertexAndIndexBuffer(WorldObjectEffect *worldObjectEffect) {
//	worldObjectEffect->createAndUploadVertexBuffer(this);
//
//}

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

XMMATRIX WorldObject::calcToWorld() {
	// quaternion
	XMVECTOR q = XMQuaternionIdentity();
	//XMVECTOR q = XMVectorSet(rot().x, rot().y, rot().z, 0.0f);
	XMVECTOR q_origin = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMMATRIX rotateM = XMMatrixRotationRollPitchYaw(rot().y, rot().x, rot().z);
	q = XMQuaternionRotationMatrix(rotateM);
	if (useQuaternionRotation) {
		q = XMLoadFloat4(&quaternion);
	}
	q = XMQuaternionNormalize(q);
	// scalar
	XMVECTOR s = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	// translation
	XMVECTOR t = XMVectorSet(pos().x, pos().y, pos().z, 0.0f);
	// toWorld matrix:
	return XMMatrixAffineTransformation(s, q_origin, q, t);
}

void WorldObject::update() {
	if (this->pathDescBone) {
		//xapp().world.path.updateScene(pathDescBone, this, xapp().gametime.getTimeAbsSeconds());
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
		//xapp().world.drawBox(bbox);
	}
	//if (drawNormals) {
	//	//unique_ptr<Lines>& linesEffect = (unique_ptr<Lines>&)Effect::getUniquePtr(Effect::LINES);
	//	XMVECTOR r2 = XMQuaternionRotationRollPitchYaw(rot().y, rot().x, rot().z);
	//	// rotate all normals then draw them for each vertex
	//	vector<LineDef> lines;
	//	for (auto &v : mesh->vertices) {
	//		// rotate normal
	//		XMVECTOR rotNorm = XMLoadFloat3(&v.Normal);
	//		rotNorm = XMVector3Rotate(rotNorm, r2);
	//		//XMFLOAT3 n;
	//		//XMStoreFloat3(&n, rotNorm);
	//		// rotate vertex
	//		XMVECTOR rotV = XMLoadFloat3(&v.Pos);
	//		rotV = XMVector3Rotate(rotV, r2);
	//		// translate vertex:
	//		XMVECTOR t = XMLoadFloat3(&pos());
	//		rotV = rotV + t;
	//		XMFLOAT3 vt;
	//		XMStoreFloat3(&vt, rotV);
	//		XMFLOAT3 vtEnd;
	//		XMStoreFloat3(&vtEnd, rotV + rotNorm);

	//		LineDef line;
	//		line.color = Colors::Silver;
	//		line.start = vt;
	//		line.end = vtEnd;
	//		lines.push_back(line);
	//	}
	//	xapp().world.linesEffect->addOneTime(lines);

	//}
}

float getVLen(XMFLOAT3 &p0, XMFLOAT3 &p1) {
	XMVECTOR pv0 = XMLoadFloat3(&p0);
	XMVECTOR pv1 = XMLoadFloat3(&p1);
	return XMVectorGetX(XMVector3Length(pv1-pv0));
}

/*void WorldObject::drawMeshFromTriangleLines(vector<WorldObjectVertex::VertexSkinned>* vertices, LinesEffect* linesEffect, unsigned long user)
{
	vector<LineDef> lines;
	for (size_t i = 0; i < vertices->size(); i += 3) {
		// take set of 3 vertices and draw triangle:
		LineDef a, b, c;
		a.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f);  // blue
		a.start = vertices->at(i).Pos;
		a.end = vertices->at(i + 1).Pos;
		b.color = a.color;
		b.start = vertices->at(i + 1).Pos;
		b.end = vertices->at(i + 2).Pos;
		c.color = a.color;
		c.start = vertices->at(i + 2).Pos;
		c.end = vertices->at(i).Pos;
		lines.push_back(a);
		lines.push_back(b);
		lines.push_back(c);
	}
	linesEffect->addOneTime(lines, user);
}*/

void WorldObject::drawMeshFromTriangleLines(vector<WorldObjectVertex::VertexTextured>* vertices, LinesEffect* linesEffect, float time, unsigned long user)
{
	vector<LineDef> lines;
	for (size_t i = 0; i < vertices->size(); i += 3) {
		// take set of 3 vertices and draw triangle:
		LineDef a, b, c;
		a.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f);  // blue
		a.start = vertices->at(i).Pos;
		a.end = vertices->at(i + 1).Pos;
		b.color = a.color;
		b.start = vertices->at(i + 1).Pos;
		b.end = vertices->at(i + 2).Pos;
		c.color = a.color;
		c.start = vertices->at(i + 2).Pos;
		c.end = vertices->at(i).Pos;
		lines.push_back(a);
		lines.push_back(b);
		lines.push_back(c);
	}
	linesEffect->addOneTime(lines, user);
}

XMMATRIX WorldObject::bonePoseTransform(int poseIndex, PathDesc* pathDescBone, float time, unsigned long user)
{
	static const int maxDepth = 10;
	assert(poseIndex < maxDepth);

	//auto& poses = pathDescBone->clip->boneBindPoseMatrices;
	array<XMMATRIX, maxDepth> poseM;
	int curMatrixIndex = 0;
	do {
		poseM[curMatrixIndex++] = XMMatrixTranspose(XMLoadFloat4x4(&pathDescBone->clip->boneBindPoseMatrices[poseIndex]));
		poseIndex = pathDescBone->clip->parents[poseIndex];
	} while (poseIndex != -1);
	// now multiply all matrices along the bone structure:
	XMMATRIX f = XMMatrixIdentity();
	for (int i = curMatrixIndex-1; i >= 0; i--) {
		f *= poseM[i];
	}
	return f;
}

template<size_t sz>
inline void WorldObject::transformAlongBones(array<XMFLOAT3, sz> &points, int poseIndex, PathDesc* pathDescBone, float time, unsigned long user)
{
	auto& poses = pathDescBone->clip->boneBindPoseMatrices;
	assert(poseIndex >= 0 && poseIndex < poses.size() );
	XMMATRIX t = bonePoseTransform(poseIndex, pathDescBone, time, user);
	for (int i = 0; i < sz; i++) {
		XMVECTOR v = XMLoadFloat3(&points[i]);
		v = XMVector3Transform(v, t);
		XMStoreFloat3(&points[i], v);
		// for unclear reasons we must flip y and z:
		Util::flipYZ3(points[i]);
	}
}

void WorldObject::drawSkeletonFromLines(PathDesc* pathDescBone, vector<WorldObjectVertex::VertexTextured>* vertices, LinesEffect* linesEffect, float time, unsigned long user)
{
	if (pathDescBone == nullptr) {
		// nothing to do for non-animated objects
		return;
	}
	vector<LineDef> lines;
	auto& poses = pathDescBone->clip->boneBindPoseMatrices;
	XMVECTOR p = XMVectorZero(); // we start with root bone at origin
	for (size_t i = 0; i < poses.size(); i++) {
		int parentIndex = pathDescBone->clip->parents[i];
		if (parentIndex == -1) {
			continue;  // nothing to do for root bone
		}
		// from part: transform to parent bone space:
		array<XMFLOAT3, 4> from;
		array<XMFLOAT3, 1> to;
		from[0] = XMFLOAT3(-BoneBaseDisplacement, 0.0f, 0.0f);
		from[2] = XMFLOAT3(BoneBaseDisplacement, 0.0f, 0.0f);
		from[3] = XMFLOAT3(0.0f, -BoneBaseDisplacement, 0.0f);
		from[1] = XMFLOAT3(0.0f, BoneBaseDisplacement, 0.0f);
		transformAlongBones<4>(from, parentIndex, pathDescBone, time, user);
		to[0] = XMFLOAT3(0.0f, 0.0f, 0.0f);
		transformAlongBones<1>(to, (int)i, pathDescBone, time, user);
		LineDef a;
		a.color = XMFLOAT4(0.0f, 1.0f, 1.0f, 0.0f);  // turque

		// draw 4 lines from base to end
		a.start = from[0];
		a.end = to[0];
		lines.push_back(a);
		a.start = from[1];
		lines.push_back(a);
		a.start = from[2];
		lines.push_back(a);
		a.start = from[3];
		lines.push_back(a);

		// draw 4 lines to form a base
		a.start = from[0];
		a.end = from[1];
		lines.push_back(a);
		a.start = from[1];
		a.end = from[2];
		lines.push_back(a);
		a.start = from[2];
		a.end = from[3];
		lines.push_back(a);
		a.start = from[3];
		a.end = from[0];
		lines.push_back(a);
	}
	linesEffect->addOneTime(lines, user);
}
/*
void WorldObject::drawSkeletonFromLines(PathDesc* pathDescBone, vector<WorldObjectVertex::VertexTextured>* vertices, LinesEffect* linesEffect, float time, unsigned long user)
{
	vector<LineDef> lines;
	auto& poses = pathDescBone->clip->boneBindPoseMatrices;
	XMVECTOR p = XMVectorZero(); // we start with root bone at origin
	for (size_t i = 0; i < poses.size(); i++) {
		int parentIndex = pathDescBone->clip->parents[i];
		if (parentIndex == -1) {
			continue;  // nothing to do for root bone
		}
		// 2 matrices for this bone:
		XMFLOAT4X4 fromF = poses.at(parentIndex);
		XMFLOAT4X4 toF = poses.at(i);
		XMMATRIX from = XMLoadFloat4x4(&fromF);
		XMMATRIX to = XMLoadFloat4x4(&toF);
		from = XMMatrixTranspose(from);
		to = XMMatrixTranspose(to);
		XMVECTOR rFrom = XMVector3Transform(p, from);
		XMVECTOR rTo = XMVector3Transform(rFrom, to);
		p = rFrom; // for next iteration
		XMFLOAT3 start, end;
		XMStoreFloat3(&start, rFrom);
		XMStoreFloat3(&end, rTo);
		// for unclear reasons we must flip y and z:
		Util::flipYZ3(start);
		Util::flipYZ3(end);
		LineDef a;
		a.color = XMFLOAT4(0.0f, 1.0f, 1.0f, 0.0f);  // turque

		a.start = start;
		a.end = end;
		lines.push_back(a);
		//linesEffect->
	}
	linesEffect->addOneTime(lines, user);
}
*/
void WorldObject::drawSkeleton(XMFLOAT4 color, Path* path, LinesEffect* linesEffect, float time, unsigned long user) {
	if (this->pathDescBone != nullptr) {
		path->updateTime(this, time);
		path->recalculateBoneAnimation(this->pathDescBone, this, this->pathDescBone->percentage);
	}
	//drawMeshFromTriangleLines(&mesh->skinnedVertices, linesEffect, user);
	if (mesh->skinnedVertices.size() > 0 && !disableSkinning) {
		for (int skV = 0; skV < (int)mesh->skinnedVertices.size(); skV++) {
			WorldObjectVertex::VertexSkinned* v = &mesh->skinnedVertices[skV];
			XMVECTOR vfinal, normfinal;
			if (isNonKeyframeAnimated) assert(false); // should not happen //xapp().world.path.skinNonKeyframe(vfinal, normfinal, v, pathDescBone);
			else path->skin(vfinal, normfinal, v, pathDescBone);
			// TODO handle cpu calculated normals after animation/skinning here
			XMFLOAT3 vfinal_flo;
			XMStoreFloat3(&vfinal_flo, vfinal);
			mesh->vertices[skV].Pos = vfinal_flo;
			XMFLOAT3 normfinal_flo;
			XMStoreFloat3(&normfinal_flo, normfinal);
			mesh->vertices[skV].Normal = normfinal_flo;
		}
		drawMeshFromTriangleLines(&mesh->vertices, linesEffect, time, user);
		drawSkeletonFromLines(this->pathDescBone, &mesh->vertices, linesEffect, time, user);
	}
	else if (mesh->vertices.size() > 0 && !disableSkinning) {
		//Log("draw skeleton for unskinned object");
		for (int skV = 0; skV < (int)mesh->skinnedVertices.size(); skV++) {
			WorldObjectVertex::VertexTextured* v = &mesh->vertices[skV];
			XMVECTOR vfinal, normfinal;
			// no skinning to do for non-animated objects - just use pose
			vfinal = XMLoadFloat3(&v->Pos);
			normfinal = XMLoadFloat3(&v->Normal);
			// TODO handle cpu calculated normals after animation/skinning here
			XMFLOAT3 vfinal_flo;
			XMStoreFloat3(&vfinal_flo, vfinal);
			mesh->vertices[skV].Pos = vfinal_flo;
			XMFLOAT3 normfinal_flo;
			XMStoreFloat3(&normfinal_flo, normfinal);
			mesh->vertices[skV].Normal = normfinal_flo;
		}
		drawMeshFromTriangleLines(&mesh->vertices, linesEffect, time, user);
	}
}

void WorldObject::draw() {
/*	WorldObjectEffect *worldObjectEffect = xapp().objectStore.getWorldObjectEffect();
	
	BoundingBox box;
	XMFLOAT4X4 finalWorld;
	XMMATRIX toWorld;
	TextureInfo *info;
	{
		//unique_lock<mutex> lock(worldObjectEffect->mutex_wo_drawing); //TODO: extensive checks...
		//Log(" obj draw locked" << objectNum << endl);
		toWorld = calcToWorld();
		Camera *cam = nullptr;
		if (worldObjectEffect->inThreadOperation) {
			// copy global camera to thread camera: TODO: should only be needed once
			worldObjectEffect->threadLocal.camera = xapp().camera;
			cam = &worldObjectEffect->threadLocal.camera;
		}
		else {
			cam = &xapp().camera;
		}
		cam->viewTransform();
		cam->projectionTransform();
		//getBoundingBox(box);//}
		int visible = true; // TODO rethink visibility xapp().camera.calculateVisibility(box, toWorld);  // TODO first move, then calc visibility
																		//Log("visible == " << visible << endl);
		if (visible == 0) {
			//Log(" obj invisible" << objectNum << endl);
			return;
		}

		XMStoreFloat4x4(&finalWorld, XMMatrixTranspose(toWorld));
		info = this->textureID;
		if (action) {
			//move object
			XMFLOAT3 pos, rot;
			xapp().world.path.getPos(*this, xapp().gametime.getTimeAbsSeconds(), pos, rot);
			pos.x = objectStartPos.x + pos.x * scale;
			pos.y = objectStartPos.y + pos.y * scale;
			pos.z = objectStartPos.z + pos.z * scale;
			float diff = getVLen(this->pos(), pos);
			this->pos() = pos;
			this->rot() = rot;
		}
		else {
			if (mesh->skinnedVertices.size() > 0 && !disableSkinning) {
				for (int skV = 0; skV < (int)mesh->skinnedVertices.size(); skV++) {
					WorldObjectVertex::VertexSkinned *v = &mesh->skinnedVertices[skV];
					XMVECTOR vfinal, normfinal;
					if (isNonKeyframeAnimated) xapp().world.path.skinNonKeyframe(vfinal, normfinal, v, pathDescBone);
					else xapp().world.path.skin(vfinal, normfinal, v, pathDescBone);
					// TODO handle cpu calculated normals after animation/skinning here
					XMFLOAT3 vfinal_flo;
					XMStoreFloat3(&vfinal_flo, vfinal);
					mesh->vertices[skV].Pos = vfinal_flo;
					XMFLOAT3 normfinal_flo;
					XMStoreFloat3(&normfinal_flo, normfinal);
					mesh->vertices[skV].Normal = normfinal_flo;
				}
				mesh->createVertexAndIndexBuffer(worldObjectEffect);
			}
			else {
				// no skinned vertices, no action/movement - nothing to do
			}
		}
	worldObjectEffect->draw(mesh, mesh->vertexBuffer, mesh->indexBuffer, finalWorld, mesh->numIndexes, info, material, objectNum, threadNum, alpha);
		//Log(" obj locked end" << objectNum << endl);
	} // lock end
*/}

void WorldObject::setAction(string name) {
	Log(" action == " << action << endl);
	Action *action = &mesh->actions[name];
	Log(" action == " << action << endl);
	assert(action->name.compare(name) == 0); // assert that action was found
	if (action->name.compare("non_keyframe") == 0) {
		this->isNonKeyframeAnimated = true;
		this->pathDescBone = new PathDesc();
		this->pathDescBone->clip = &mesh->clips["non_keyframe"];
		return;
	}
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
	d->pathMode = PathMode::Path_Reverse;
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
	pathDescBone = pathDescMove = nullptr;
	scale = 1.0f;
	drawBoundingBox = false;
	drawNormals = false;
	objectNum = count++;
	threadNum = 0;
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
void WorldObjectStore::loadObject(wstring filename, string id, float scale, XMFLOAT3 *displacement) {
	MeshLoader loader;
	wstring binFile = DXGlobal::getInstance()->util.findFile(filename.c_str(), Util::MESH);
	Mesh mesh;
	meshes[id] = mesh;
	//loader.loadBinaryAsset(binFile, &meshes[id], scale, displacement);
	//meshes[id].createVertexAndIndexBuffer(this->objectEffect);
}

void WorldObjectStore::loadObjects(wstring filename, vector<ObjectDef> &objectDefs)
{
	MeshLoader loader;
	wstring binFile = DXGlobal::getInstance()->util.findFile(filename.c_str(), Util::MESH);
	for (auto& od : objectDefs) {
		Mesh mesh;
		mesh.nameFromCollada = od.colladaName;
		meshes[od.objectStoreId] = mesh;
		od.mesh = &meshes[od.objectStoreId]; // store address
	}
	loader.loadBinaryAssets(binFile, objectDefs);
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

void WorldObjectStore::drawGroup(string groupname, size_t threadNum)
{
	//WorldObjectEffect *objectEffect = xapp().objectStore.getWorldObjectEffect();
	//objectEffect->beginBulkUpdate();
	//auto grp = xapp().objectStore.getGroup(groupname);
	////Log(" draw objects: " << grp->size() << endl);
	//if (threadNum >= 1) {
	//	objectEffect->divideBulk(grp->size(), threadNum, grp);
	//}
	//else {
	//	//Log("after bulk divide" << endl);
	//	for (auto & w : *grp) {
	//		w->draw();
	//	}
	//}
	//objectEffect->endBulkUpdate();
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

//void WorldObjectStore::setWorldObjectEffect(WorldObjectEffect *weff) {
//	this->objectEffect = weff;
//}

UINT WorldObject::count = 0;