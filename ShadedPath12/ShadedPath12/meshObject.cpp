#include "stdafx.h"

MeshObject::MeshObject()
{
}

MeshObject::~MeshObject()
{
}

// object store:
void MeshObjectStore::loadObject(wstring filename, string id, float scale, XMFLOAT3 *displacement) {
	MeshLoader loader;
	wstring binFile = xapp().findFile(filename.c_str(), XApp::MESH);
	Mesh mesh;
	meshes[id] = mesh;
	loader.loadBinaryAsset(binFile, &meshes[id], scale, displacement);
	meshes[id].createVertexAndIndexBuffer(this->objectEffect);
}
