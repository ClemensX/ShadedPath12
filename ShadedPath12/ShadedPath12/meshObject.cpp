#include "stdafx.h"

MeshObject::MeshObject()
{
}

MeshObject::~MeshObject()
{
}

XMFLOAT3& MeshObject::pos() {
	return _pos;
}

XMFLOAT3& MeshObject::rot() {
	return _rot;
}

XMMATRIX MeshObject::calcToWorld() {
	// quaternion
	XMVECTOR q = XMQuaternionIdentity();
	//XMVECTOR q = XMVectorSet(rot().x, rot().y, rot().z, 0.0f);
	XMVECTOR q_origin = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMMATRIX rotateM = XMMatrixRotationRollPitchYaw(rot().y, rot().x, rot().z);
	q = XMQuaternionRotationMatrix(rotateM);
	if (useQuaternionRotation) {
		q = XMLoadFloat4(&rot_quaternion);
	}
	q = XMQuaternionNormalize(q);
	// scalar
	XMVECTOR s = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	// translation
	XMVECTOR t = XMVectorSet(pos().x, pos().y, pos().z, 0.0f);
	// toWorld matrix:
	return XMMatrixAffineTransformation(s, q_origin, q, t);
}


// object store:
void MeshObjectStore::loadObject(wstring filename, string id, float scale, XMFLOAT3 *displacement) {
	assert(inGpuUploadPhase);
	MeshLoader loader;
	wstring binFile = xapp().findFile(filename.c_str(), XApp::MESH);
	Mesh mesh;
	meshes[id] = mesh;
	loader.loadBinaryAsset(binFile, &meshes[id], scale, displacement);
	createAndUploadVertexAndIndexBuffer(&meshes[id]);
	//meshes[id].createVertexAndIndexBuffer(this->objectEffect);
}

void MeshObjectStore::setMaxObjectCount(unsigned int maxNum)
{
	assert(maxNum > 0);
	assert(initialized == false);	// only allowed to call before effect initialization
	maxObjects = maxNum;
}

/*
* Calculate WVP matrix from projection and world matrix
*/
XMMATRIX MeshObjectStore::calcWVP(XMMATRIX &toWorld, XMMATRIX &vp) {
	// optimization of commented code via transpose rule: (A*B)T = BT * AT
	//vp = XMMatrixTranspose(vp);
	//toWorld = XMMatrixTranspose(toWorld);
	//XMMATRIX wvp = toWorld * vp;
	//wvp = XMMatrixTranspose(wvp);
	//return wvp;
	return vp * toWorld;
}

void MeshObjectStore::updateOne(MeshObject *mo, MeshObjectStore *store, XMMATRIX vp, int frameIndex) {
	assert(mo->objectNum > 0); // not properly added to store
	CBV my_cbv;
	CBV *cbv = &my_cbv;
	//Log("  elem: " << mo->pos().x << endl);
	XMMATRIX toWorld = mo->calcToWorld(); // apply pos and rot
	XMMATRIX wvp = store->calcWVP(toWorld, vp);
	XMStoreFloat4x4(&cbv->wvp, wvp);
	XMStoreFloat4x4(&cbv->world, toWorld);
	//cbv->world = di.world;
	cbv->alpha = mo->alpha;
	memcpy(MeshObjectStore::getStore()->getCBVUploadAddress(frameIndex, 0, mo->objectNum, 0), cbv, sizeof(*cbv));

}

void MeshObjectStore::update()
{
	assert(this->maxObjects > 0);	// setting of max object count missing
	int frameIndex = xapp().getCurrentBackBufferIndex();

	frameEffectData[frameIndex].camera = xapp().camera;

	Camera *cam = &frameEffectData[frameIndex].camera;
	CBV my_cbv;
	CBV *cbv = &my_cbv;
	prepareDraw(&xapp().vr);
	if (!xapp().ovrRendering) {
		frameEffectData[frameIndex].vr_eyesm[0] = vr_eyes;
		XMMATRIX vp = cam->worldViewProjection();
		cbv->cameraPos.x = cam->pos.x;
		cbv->cameraPos.y = cam->pos.y;
		cbv->cameraPos.z = cam->pos.z;
		forAll([vp, frameIndex](MeshObject *mo) {
			//Log("  elem: " << mo->pos().x << endl);
			MeshObjectStore::updateOne(mo, MeshObjectStore::getStore(), vp, frameIndex);
		});
		//		XMMATRIX toWorld = XMLoadFloat4x4(&di.world);
//		XMMATRIX wvp = calcWVP(toWorld, vp);
//		XMStoreFloat4x4(&cbv->wvp, wvp);
//		cbv->world = di.world;
//		cbv->alpha = di.alpha;
//		if (inBulkOperation) {
//			memcpy(getCBVUploadAddress(frameIndex, di.threadNum, di.objectNum, 0), cbv, sizeof(*cbv));
//		}
//		else {
//			memcpy(cbvGPUDest, cbv, sizeof(*cbv));
//		}
		//memcpy(getCBVUploadAddress(frameIndex, di.threadNum, di.objectNum), &cbv, sizeof(cbv));
		//memcpy(cbvGPUDest + cbvAlignedSize, &cbv, sizeof(cbv));
		//memcpy(getCBVUploadAddress(frameIndex, 0), &cbv, sizeof(cbv));
//		drawInternal(di);
		return;
	}

}

void MeshObjectStore::draw()
{
	assert(this->maxObjects > 0);	// setting of max object count missing
}

// GPU Upload Phase
void MeshObjectStore::gpuUploadPhaseEnd()
{
	// TODO: actual upload with effect ??
	auto &f = updateFrameData;
	createSyncPoint(f, xapp().commandQueue);
	waitForSyncPoint(f);
	inGpuUploadPhase = false;
}

void MeshObjectStore::createGroup(string groupname) {
	if (groups.count(groupname) > 0) return;  // do not recreate groups
	const auto &newGroup = groups[groupname];
	Log(" ---groups size " << groups.size() << endl);
	Log(" ---newGroup vecor size " << newGroup.size() << endl);
}

void MeshObjectStore::forAll(std::function<void(MeshObject *mo)> func)
{
	for (auto & group : this->groups) {
		//Log("group: " << group.first.c_str() << endl);
		for (auto & w : group.second) {
			//Log("  group el: " << w.get()->pos().x << endl);
			func(w.get());
		}
	}
}

void MeshObjectStore::addObject(string groupname, string id, XMFLOAT3 pos, TextureID tid) {
	assert(groups.count(groupname) > 0);
	assert(used_objects + 1 < maxObjects);
	auto &grp = groups[groupname];
	grp.push_back(unique_ptr<MeshObject>(new MeshObject()));
	MeshObject *w = grp[grp.size() - 1].get();
	w->pos() = pos;
	w->objectNum = ++this->used_objects;
	//w->mesh = &mesh;
	//w->textureID = tid;
	////w.wireframe = false;
	//w->alpha = 1.0f;
	//addObjectPrivate(w, id, pos, tid);
}


// effect methods:

void MeshObjectStore::init()
{
	if (initialized) return;
	initialized = true;
	assert(this->maxObjects > 0);	// setting of max object count missing

	// try to do all expensive operations like shader loading and PSO creation here
	// Create the pipeline state, which includes compiling and loading shaders.
	{
		createRootSigAndPSO(rootSignature, pipelineState);
		setSingleCBVMode(1, maxObjects, sizeof(cbv), L"mesheffect_cbvsingle_resource");
	}

	// Create command allocators and command lists for each frame.
	static LPCWSTR fence_names[XApp::FrameCount] = {
		L"fence_mesheffect_0", L"fence_mesheffect_1", L"fence_mesheffect_2"
	};
	for (UINT n = 0; n < XApp::FrameCount; n++)
	{
		ThrowIfFailed(xapp().device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[n])));
		ThrowIfFailed(xapp().device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[n].Get(), pipelineState.Get(), IID_PPV_ARGS(&commandLists[n])));
		// Command lists are created in the recording state, but there is nothing
		// to record yet. The main loop expects it to be closed, so close it now.
		ThrowIfFailed(commandLists[n]->Close());
		// init fences:
		//ThrowIfFailed(xapp().device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(frameData[n].fence.GetAddressOf())));
		ThrowIfFailed(xapp().device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameData[n].fence)));
		frameData[n].fence->SetName(fence_names[n]);
		frameData[n].fenceValue = 0;
		frameData[n].fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (frameData[n].fenceEvent == nullptr) {
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
		// init frame resources of this effect: 
		frameEffectData[n].initialized = true;
	}
	// init resources for update thread:
	ThrowIfFailed(xapp().device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&updateCommandAllocator)));
	ThrowIfFailed(xapp().device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, updateCommandAllocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&updateCommandList)));
	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	ThrowIfFailed(updateCommandList->Close());
	// init fences:
	//ThrowIfFailed(xapp().device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(frameData[n].fence.GetAddressOf())));
	ThrowIfFailed(xapp().device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&updateFrameData.fence)));
	updateFrameData.fence->SetName(L"fence_objecteffect_update");
	updateFrameData.fenceValue = 0;
	updateFrameData.fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	if (updateFrameData.fenceEvent == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}

void MeshObjectStore::createAndUploadVertexAndIndexBuffer(Mesh * mesh)
{
	int frameIndex = xapp().getCurrentBackBufferIndex();
	size_t vertexSize = sizeof(WorldObjectVertex::VertexTextured);
	size_t bufferSize = vertexSize * mesh->numVertices;
	void *data = &mesh->vertices[0];
	EffectBase::createAndUploadVertexBuffer(bufferSize, vertexSize, data,
		pipelineState.Get(), L"MeshVB",
		mesh->vertexBuffer,
		mesh->vertexBufferUpload,
		commandAllocators[frameIndex],
		updateCommandList,
		mesh->vertexBufferView
	);
	// upload index
	bufferSize = sizeof(mesh->indexes[0]) * mesh->indexes.size();
	data = &mesh->indexes[0];
	EffectBase::createAndUploadIndexBuffer(bufferSize, data,
		pipelineState.Get(), L"MeshIB",
		mesh->indexBuffer,
		mesh->indexBufferUpload,
		commandAllocators[frameIndex],
		updateCommandList,
		mesh->indexBufferView
	);
	// Close the command list and execute it to begin the vertex buffer copy into
	// the default heap.
	ThrowIfFailed(updateCommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { updateCommandList.Get() };
	xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

#include "CompiledShaders/ObjectVS.h"
#include "CompiledShaders/ObjectPS.h"

void MeshObjectStore::createRootSigAndPSO(ComPtr<ID3D12RootSignature> &sig, ComPtr<ID3D12PipelineState> &pso)
{
	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		//{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Describe and create the graphics pipeline state object (PSO).
	CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;//D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = blendDesc;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	psoDesc.VS = { binShader_ObjectVS, sizeof(binShader_ObjectVS) };
	psoDesc.PS = { binShader_ObjectPS, sizeof(binShader_ObjectPS) };
	ThrowIfFailed(xapp().device->CreateRootSignature(0, binShader_ObjectVS, sizeof(binShader_ObjectVS), IID_PPV_ARGS(&sig)));
	sig.Get()->SetName(L"Object_root_signature");
	psoDesc.pRootSignature = sig.Get();
	ThrowIfFailed(xapp().device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
	pso.Get()->SetName(L"state_objecteffect_init");
}
