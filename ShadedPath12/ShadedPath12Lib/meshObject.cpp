#include "stdafx.h"

#include "CompiledShaders/MObjectVS.h"
#include "CompiledShaders/MObjectPS.h"
#include "CompiledShaders/MObjectCS.h"

MeshObject::MeshObject()
{
	flagUploadToGPU();
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
	return calcToWorld(pos(), rot(), useQuaternionRotation, &rot_quaternion);
}

XMMATRIX MeshObject::calcToWorld(XMFLOAT3 pos, XMFLOAT3 rot, bool useQuaternionRotation, XMFLOAT4 *rot_quaternion) {
	// quaternion
	XMVECTOR q = XMQuaternionIdentity();
	//XMVECTOR q = XMVectorSet(rot.x, rot.y, rot.z, 0.0f);
	XMVECTOR q_origin = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMFLOAT3 p = XMFLOAT3(0.0f, 0.0f, 0.0f);//rot();
											//WegDamit2.lock();
	XMMATRIX rotateM = XMMatrixRotationRollPitchYaw(p.y, p.x, p.z);
	//WegDamit2.unlock();
	q = XMQuaternionRotationMatrix(rotateM);
	if (useQuaternionRotation) {
		q = XMLoadFloat4(rot_quaternion);
	}
	q = XMQuaternionNormalize(q);
	// scalar
	XMVECTOR s = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	// translation
	XMVECTOR t = XMVectorSet(pos.x, pos.y, pos.z, 0.0f);
	// toWorld matrix:
	XMMATRIX r = XMMatrixTranspose(XMMatrixAffineTransformation(s, q_origin, q, t));
	return r;
	//return XMMatrixTranspose(XMMatrixAffineTransformation(s, q_origin, q, t));
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


void MeshObjectStore::updateOne(CBV const *cbv_read, MeshObject *mo, XMMATRIX vp, int frameIndex, int eyeNum) {
	assert(mo->objectNum > 0); // not properly added to store
	//if (mo->uploadedToGPU[frameIndex]) return;
	//mo->uploadedToGPU[frameIndex] = true;
	//Log("  elem: " << mo->pos().x << endl);
	//WegDamit.lock();
	//XMMATRIX toWorld = XMMatrixIdentity();//mo->calcToWorld(); // apply pos and rot
	//XMMATRIX wvp = XMMatrixIdentity(); //calcWVP(toWorld, vp);
	XMMATRIX toWorld = mo->calcToWorld(); // apply pos and rot
	XMMATRIX wvp = calcWVP(toWorld, vp);
	CBV localcbv = *cbv_read;
	CBV *cbv = &localcbv;
	cbv->material = mo->material;
	XMStoreFloat4x4(&cbv->wvp, wvp);
	XMStoreFloat4x4(&cbv->world, toWorld);
	//XMStoreFloat4x4(&cbv->vp, vp);
	//XMStoreFloat4x4(&cbv->, toWorld);
	//cbv->world._11 = 1;// mo->pos().x;
	//cbv->world._12 = 2;// mo->pos().y;
	//cbv->world._13 = 3;// mo->pos().z;
	cbv->cameraPos.x = mo->pos().x;
	cbv->cameraPos.y = mo->pos().y;
	cbv->cameraPos.z = mo->pos().z;
				   //cbv->world = di.world;
	cbv->alpha = mo->alpha;
	dxManager.upload(mo->objectNum, eyeNum, cbv);
}

void MeshObjectStore::updatePart(BulkDivideInfo bi, CBV * cbv, vector<unique_ptr<MeshObject>>* mov, XMMATRIX vp, int frameIndex, int eyeNum)
{
	// enable for test data:
	/*
	XMFLOAT4 t = XMFLOAT4(1,2,3,0);
	XMVECTOR tv = XMLoadFloat4(&t);
	XMVECTOR qv = XMQuaternionRotationRollPitchYawFromVector(tv);
	//qv = XMVectorMultiply(tv, g_XMOneHalf);
	XMVECTOR SinAngles, CosAngles;
	XMVectorSinCos(&SinAngles, &CosAngles, qv);
	//XMStoreFloat4(&t, CosAngles);
	XMMATRIX m = XMMatrixRotationQuaternion(qv);
	XMStoreFloat4(&t, qv);
	frameEffectData[frameIndex].cbvCS.vp._11 = t.x;
	frameEffectData[frameIndex].cbvCS.vp._21 = t.y;
	frameEffectData[frameIndex].cbvCS.vp._31 = t.z;
	frameEffectData[frameIndex].cbvCS.vp._41 = t.w;
	XMStoreFloat4x4(&frameEffectData[frameIndex].cbvCS.vp, m);
	// another test for wvp calculation:
	XMFLOAT3 pos = XMFLOAT3(1, -2, 3);
	XMFLOAT3 rot = XMFLOAT3(0, 0, 0);
	XMMATRIX wvp = MeshObject::calcToWorld(pos, rot);
	XMStoreFloat4x4(&frameEffectData[frameIndex].cbvCS.vp, wvp); */

	// now the real thing:
	XMStoreFloat4x4(&frameEffectData[frameIndex].cbvCS.vp, vp);
	frameEffectData[frameIndex].cbvCS.num_objects = bi.end - bi.start + 1;
	frameEffectData[frameIndex].cbvCS.start_objects = bi.start;
	frameEffectData[frameIndex].cbvCS.start_objects = mov->at(bi.start).get()->objectNum;
	//if (frameIndex == 0) {
	//	Log("from_index len " << frameEffectData[frameIndex].cbvCS.start_objects << " " << frameEffectData[frameIndex].cbvCS.num_objects << endl);
	//}

	dxManager.uploadConstantBufferSet(0, sizeof(CBV_CS), &frameEffectData[frameIndex].cbvCS);

	//if (frameEffectData[frameIndex].updateConstBuffers == false)
	//	return;
	for (unsigned int i = bi.start; i <= bi.end; i++) {
		//updateOne(cbv, mov[i].get(), vp, frameIndex, eyeNum);
		MeshObject *mo = mov->at(i).get();
		if (!mo->uploadedToGPU[frameIndex]) {
			mo->uploadedToGPU[frameIndex] = true;
			updateOne(cbv, mo, vp, frameIndex, eyeNum);
			frameEffectData[frameIndex].updateConstBuffers = true;
		}
	}
	//frameEffectData[frameIndex].updateConstBuffers = false;
}

void MeshObjectStore::update()
{
	if (xapp().isShutdownMode()) return;
	xapp().stats.start("meshStoreUpdate");
	assert(this->maxObjects > 0);	// setting of max object count missing
	int frameIndex = xapp().getCurrentBackBufferIndex();
	dxManager.setCurrentFrame(frameIndex);

	frameEffectData[frameIndex].camera = xapp().camera;

	Camera *cam = &frameEffectData[frameIndex].camera; // copy global camera to local
	CBV my_cbv;
	CBV *cbv = &my_cbv;
	prepareDraw(&xapp().vr); // copy viewports and scissor rects to vr fields
	//xapp().lights.update();
	if (!vr || true) {
		frameEffectData[frameIndex].vr_eyesm[0] = vr_eyes;
		XMMATRIX vp = cam->worldViewProjection();
		cbv->cameraPos.x = cam->pos.x;
		cbv->cameraPos.y = cam->pos.y;
		cbv->cameraPos.z = cam->pos.z;
		XMFLOAT4X4 vpM;
		XMStoreFloat4x4(&vpM, vp);
		frameEffectData[frameIndex].cbvCS.vp = vpM;
		for (auto & group : this->groups) {
			//Log("group: " << group.first.c_str() << endl);
			divideBulk(group.second.size(), 1, bulkInfos);
			vector<unique_ptr<MeshObject>>* mov = &group.second;
			if (bulkInfos.size() == 1 && true) {
				//if (bulkInfos[0].end == 0) continue;
				//if (mov->at(0).get()->objectNum != 1) continue;
				// simple update of all
				//Log("  update: " << group.first.c_str() << " [" << bulkInfos[0].start << ".." << bulkInfos[0].end << "]" << endl);
				//bulkInfos[0].start = 1;
				//bulkInfos[0].end = 3;
				updatePart(bulkInfos[0], cbv, mov, vp, frameIndex);
				frameEffectData[frameIndex].updateMaterial = false;
				auto valid = dxManager.objectStateLists[frameIndex].isValid();
				if (!valid /*count < 14*/ || frameEffectData[frameIndex].updateConstBuffers) {
					Log("update const for frameIndex " << frameIndex << endl);
					Log("  update group: " << group.first.c_str() << " [" << bulkInfos[0].start << ".." << bulkInfos[0].end << "]" << endl);
					dxManager.copyToComputeBuffer(frameData[frameIndex]);
					frameEffectData[frameIndex].updateConstBuffers = false;
				}
				xapp().stats.start("compute");
				computeMethod(frameIndex);
				xapp().stats.end("compute");
				//return;
			} else {
				assert(false);
			}
		}
		xapp().stats.end("meshStoreUpdate");

		//forAll([this, cbv, vp, frameIndex](MeshObject *mo) {
		//	//Log("  elem: " << mo->pos().x << endl);
		//	updateOne(cbv, mo, vp, frameIndex);
		//});
	} else {
		for (int eyeNum = 0; eyeNum < 2; eyeNum++) {
			// adjust PVW matrix
			frameEffectData[frameIndex].vr_eyesm[eyeNum] = vr_eyes;
			XMMATRIX adjustedEyeMatrix;
			cam->eyeNumUse = true;
			cam->eyeNum = eyeNum;
			frameEffectData[frameIndex].vr_eyesm[eyeNum].adjustEyeMatrix(adjustedEyeMatrix, cam, eyeNum, &xapp().vr);
			XMMATRIX vp = adjustedEyeMatrix;
			cbv->cameraPos = frameEffectData[frameIndex].vr_eyesm[eyeNum].adjustedEyePos[eyeNum];
			forAll([this, cbv, vp, frameIndex, eyeNum](MeshObject *mo) {
				//Log("  elem: " << mo->pos().x << endl);
				updateOne(cbv, mo, vp, frameIndex, eyeNum); // add adjustedeyematrix
			});
		}
	}
	/*
	frameEffectData[frameIndex].updateMaterial = false;
	xapp().stats.start("compute");
	dxManager.copyToComputeBuffer(frameData[frameIndex]);
	computeMethod(frameIndex);
	xapp().stats.end("compute");
	xapp().stats.end("meshStoreUpdate");
	*/
}

void MeshObjectStore::draw()
{
	if (xapp().isShutdownMode()) return;
	assert(this->maxObjects > 0);	// setting of max object count missing
	prepareDraw(&xapp().vr);
	preDraw();
	if (!vr) {
		for (auto bi : drawBulkInfos) {
			drawInternal(&bi, 0);
		}
	} else {
		for (auto bi : drawBulkInfos) {
			drawInternal(&bi, 0);
			//drawInternal(&bi, 1);
		}
	}
	postDraw();
}

// GPU Upload Phase
void MeshObjectStore::gpuUploadPhaseEnd()
{
	// TODO: actual upload with effect ??
	auto &f = updateFrameData;
	createSyncPoint(f, xapp().commandQueue);
	waitForSyncPoint(f);
	inGpuUploadPhase = false;
	divideDrawBulks();
}

void MeshObjectStore::divideDrawBulks()
{
	assert(isSorted());

	unsigned int min = 0, max = 0, last = 0;
	Mesh *last_mesh = nullptr;
	forAll([this, &last_mesh, &min, &max, &last](MeshObject *mo) {
		//Log("  objectNum: " << mo->objectNum << " mesh" << mo->mesh << endl);
		if (mo->mesh != last_mesh) {
			// start new bulk
			BulkDivideInfoExt bi;
			bi.start = mo->objectNum;
			bi.end = mo->objectNum;
			bi.mo = mo;
			drawBulkInfos.push_back(bi);
			last_mesh = mo->mesh;
		} else {
			// extending current bulk
			auto bi = &drawBulkInfos[drawBulkInfos.size() - 1];
			bi->end = mo->objectNum;
			bi->mo = mo;
		}

	});
}

void MeshObjectStore::createGroup(string groupname) {
	if (groups.count(groupname) > 0) return;  // do not recreate groups
	const auto &newGroup = groups[groupname];
	Log(" ---groups size " << groups.size() << endl);
	Log(" ---newGroup vecor size " << newGroup.size() << endl);
}

const vector<unique_ptr<MeshObject>> *MeshObjectStore::getGroup(string groupname) {
	if (groups.count(groupname) == 0) return nullptr;
	return &groups[groupname];
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

MeshObject* MeshObjectStore::addObject(string groupname, string id, XMFLOAT3 pos, TextureID tid) {
	assert(groups.count(groupname) > 0);
	assert(used_objects + 1 < maxObjects);
	assert(tid->texSRV && tid->m_srvHeap);  // texture correctly loaded?
	auto &grp = groups[groupname];
	grp.push_back(unique_ptr<MeshObject>(new MeshObject()));
	MeshObject *w = grp[grp.size() - 1].get();
	w->pos() = pos;
	w->objectNum = ++this->used_objects;
	assert(meshes.count(id) > 0);
	Mesh &mesh = meshes.at(id);
	w->mesh = &mesh;
	w->textureID = tid;
	return w;
	//w.wireframe = false;
	//w->action = nullptr;
}


// effect methods:

void MeshObjectStore::init()
{
	if (initialized) return;
	initialized = true;
	if (xapp().ovrRendering) this->vr = true;
	assert(this->maxObjects > 0);	// setting of max object count missing
	dxManager.init(xapp().device.Get());

	// try to do all expensive operations like shader loading and PSO creation here
	// Create the pipeline state, which includes compiling and loading shaders.
	{
		createRootSigAndPSO(rootSignature, pipelineState);
		dxManager.createConstantBuffer(1, maxObjects+1, sizeof(cbv), L"mesheffect_cbvsingle_resource");
		dxManager.createConstantBufferSet(0, 1, 1, sizeof(CBV_CS), L"mesheffect_cs_cbv_set");
		dxManager.createGraphicsExecutionEnv(pipelineState.Get());
		dxManager.createUploadBuffers();
		setSingleCBVMode(1, maxObjects+1, sizeof(cbv), L"mesheffect_cbvsingle_resource", true);
		// XM test
		XMVECTOR v = g_XMOneHalf.v;
		XMFLOAT4 vF;
		XMStoreFloat4(&vF, v);
		Log("val " << vF.w << endl);
	}

	// Create command allocators and command lists for each frame.
	static LPCWSTR fence_names[XApp::FrameCount] = {
		L"fence_mesheffect_0", L"fence_mesheffect_1", L"fence_mesheffect_2"
	};
	for (UINT n = 0; n < XApp::FrameCount; n++)
	{
		// todo ThrowIfFailed(xapp().device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[n])));
		// todo ThrowIfFailed(xapp().device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[n].Get(), pipelineState.Get(), IID_PPV_ARGS(&commandLists[n])));
		// Command lists are created in the recording state, but there is nothing
		// to record yet. The main loop expects it to be closed, so close it now.
		// todo ThrowIfFailed(commandLists[n]->Close());
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
		// Create compute resources.
		D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_COMPUTE, 0, D3D12_COMMAND_QUEUE_FLAG_NONE };
		ThrowIfFailed(xapp().device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&computeCommandQueue[n])));
		ThrowIfFailed(xapp().device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&computeAllocator[n])));
		ThrowIfFailed(xapp().device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, computeAllocator[n].Get(), nullptr, IID_PPV_ARGS(&computeCommandList[n])));
		// immediately close the command list - it will be reset as first step in computeMethod
		ThrowIfFailed(computeCommandList[n]->Close());
		dxManager.objectStateLists[n].init(maxObjects);
		//ThrowIfFailed(xapp().device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&threadFences[threadIndex])));
	}

	//// Create the command signature used for indirect drawing.
	//{
	//	// Each command consists of a CBV update, SRV update (for texture) and a DrawInstanced call.
	//	D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[4] = {};
	//	argumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	//	argumentDescs[0].ConstantBufferView.RootParameterIndex = Cbv;
	//	argumentDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
	//	argumentDescs[1].ConstantBufferView.RootParameterIndex = CbvLights;
	//	argumentDescs[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
	//	argumentDescs[2].ShaderResourceView.RootParameterIndex = Srv;
	//	argumentDescs[3].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

	//	D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
	//	commandSignatureDesc.pArgumentDescs = argumentDescs;
	//	commandSignatureDesc.NumArgumentDescs = _countof(argumentDescs);
	//	commandSignatureDesc.ByteStride = sizeof(IndirectCommand);

	//	ThrowIfFailed(xapp().device->CreateCommandSignature(&commandSignatureDesc, rootSignature.Get(), IID_PPV_ARGS(&commandSignature)));
	//	NAME_D3D12_OBJECT(commandSignature);
	//}
	// indirect drawing end

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
	updateMaterialOnNextFrame();
}

void MeshObjectStore::updateMaterialOnNextFrame()
{
	for (int i = 0; i < XApp::FrameCount; i++) {
		frameEffectData[i].updateMaterial = true;
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
		dxManager.getGraphicsCommandAllocatorComPtr(),//commandAllocators[frameIndex],
		dxManager.getGraphicsCommandListComPtr(),//updateCommandList,
		mesh->vertexBufferView
	);
	// upload index
	bufferSize = sizeof(mesh->indexes[0]) * mesh->indexes.size();
	data = &mesh->indexes[0];
	EffectBase::createAndUploadIndexBuffer(bufferSize, data,
		pipelineState.Get(), L"MeshIB",
		mesh->indexBuffer,
		mesh->indexBufferUpload,
		dxManager.getGraphicsCommandAllocatorComPtr(),//commandAllocators[frameIndex],
		dxManager.getGraphicsCommandListComPtr(),//updateCommandList,
		mesh->indexBufferView
	);
	// Close the command list and execute it to begin the vertex buffer copy into
	// the default heap.
	//ThrowIfFailed(updateCommandList->Close());
	//ID3D12CommandList* ppCommandLists[] = { updateCommandList.Get() };
	//xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	ThrowIfFailed(dxManager.getGraphicsCommandListComPtr()->Close());
	ID3D12CommandList* ppCommandLists[] = { dxManager.getGraphicsCommandListComPtr().Get() };
	xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void MeshObjectStore::createRootSigAndPSO(ComPtr<ID3D12RootSignature> &sig, ComPtr<ID3D12PipelineState> &pso)
{
	// Define the vertex input layout. Has to match struct VSInput in MObject.hlsli
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SV_InstanceID", 0, DXGI_FORMAT_R32_UINT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
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

	psoDesc.VS = { binShader_MObjectVS, sizeof(binShader_MObjectVS) };
	psoDesc.PS = { binShader_MObjectPS, sizeof(binShader_MObjectPS) };
	ThrowIfFailed(xapp().device->CreateRootSignature(0, binShader_MObjectVS, sizeof(binShader_MObjectVS), IID_PPV_ARGS(&sig)));
	sig.Get()->SetName(L"MObject_root_signature");
	psoDesc.pRootSignature = sig.Get();
	ThrowIfFailed(xapp().device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
	pso.Get()->SetName(L"state_mobjecteffect_init");

	// Describe and create the compute pipeline state object (PSO).
	D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
	computePsoDesc.CS = { binShader_MObjectCS, sizeof(binShader_MObjectCS) };
	ThrowIfFailed(xapp().device->CreateRootSignature(0, binShader_MObjectCS, sizeof(binShader_MObjectCS), IID_PPV_ARGS(&computeRootSignature)));
	computeRootSignature.Get()->SetName(L"MObjectCS_root_signature");
	computePsoDesc.pRootSignature = computeRootSignature.Get();

	ThrowIfFailed(xapp().device->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&computePipelineState)));
	computePipelineState.Get()->SetName(L"computestate_mobject");
}

// drawing
void MeshObjectStore::preDraw()
{
	int frameIndex = xapp().getCurrentBackBufferIndex();
	// assert that objects are well sorted (with regards to used mesh) and no holes exist with unused object ids
	assert(isSorted());
	ThrowIfFailed(dxManager.getGraphicsCommandAllocatorComPtr()->Reset());
	ThrowIfFailed(dxManager.getGraphicsCommandListComPtr()->Reset(dxManager.getGraphicsCommandAllocatorComPtr().Get(), pipelineState.Get()));
	// Set necessary state.
	dxManager.getGraphicsCommandListComPtr()->SetGraphicsRootSignature(rootSignature.Get());

	// TODO adapt for 2 eyes:
	dxManager.getGraphicsCommandListComPtr()->RSSetViewports(1, &vr_eyes.viewports[0]);
	dxManager.getGraphicsCommandListComPtr()->RSSetScissorRects(1, &vr_eyes.scissorRects[0]);
	// TODO check
	//commandLists[frameIndex]->RSSetViewports(1, &vr_eyes.viewports[eyeNum]);
	//commandLists[frameIndex]->RSSetScissorRects(1, &vr_eyes.scissorRects[eyeNum]);
	//commandLists[frameIndex]->RSSetViewports(1, xapp().vr.getViewport());
	//commandLists[frameIndex]->RSSetScissorRects(1, xapp().vr.getScissorRect());

	// Set CBVs
	//commandLists[frameIndex]->SetGraphicsRootConstantBufferView(0, getCBVVirtualAddress(frameIndex, 0, di.objectNum, 0));
	dxManager.getGraphicsCommandListComPtr()->SetGraphicsRootConstantBufferView(0, xapp().lights.cbvResource->GetGPUVirtualAddress());
	dxManager.getGraphicsCommandListComPtr()->SetGraphicsRootUnorderedAccessView(2, dxManager.getCBVVirtualAddress(0, 0));

	// Indicate that the back buffer will be used as a render target.
	//	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(xapp().rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, xapp().rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = xapp().getRTVHandle(frameIndex);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(xapp().dsvHeaps[frameIndex]->GetCPUDescriptorHandleForHeapStart());
	//m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	dxManager.getGraphicsCommandListComPtr()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	ID3D12Resource *resource;
	if (!vr || xapp().vr.texResource.size() == 0) resource = xapp().renderTargets[frameIndex].Get();
	else resource = xapp().vr.texResource[frameIndex];
	xapp().handleRTVClearing(dxManager.getGraphicsCommandListComPtr().Get(), rtvHandle, dsvHandle, resource);

}

void MeshObjectStore::postDraw()
{
	int frameIndex = xapp().getCurrentBackBufferIndex();

	ThrowIfFailed(dxManager.getGraphicsCommandListComPtr()->Close());
	ID3D12CommandQueue* pCommandQueue = dxManager.commandQueues[frameIndex].Get();
	// TODO: in non-vr mode we draw directly to the back buffer which prevents using another queue
	// so we need to switch to the main queue for executing the command list
	// D3D12 ERROR: ID3D12CommandQueue::ExecuteCommandLists: A command list, which writes to a swapchain back buffer,
	// may only be executed on the command queue associated with that buffer. [ STATE_SETTING ERROR #907: EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE]
	if (!vr || xapp().vr.texResource.size() == 0) pCommandQueue = xapp().commandQueue.Get();
	//Log("queue = " << pCommandQueue << endl);
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { dxManager.getGraphicsCommandListComPtr().Get() };
	pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void MeshObjectStore::drawInternal(BulkDivideInfoExt * bi, int eyeNum)
{
	//if (bi->start > 1) return;
	int frameIndex = xapp().getCurrentBackBufferIndex();
	MeshObject *mo = bi->mo;
	dxManager.getGraphicsCommandListComPtr()->RSSetViewports(1, &vr_eyes.viewports[eyeNum]);
	dxManager.getGraphicsCommandListComPtr()->RSSetScissorRects(1, &vr_eyes.scissorRects[eyeNum]);
	dxManager.getGraphicsCommandListComPtr()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dxManager.getGraphicsCommandListComPtr()->IASetVertexBuffers(0, 1, &mo->mesh->vertexBufferView);
	dxManager.getGraphicsCommandListComPtr()->IASetIndexBuffer(&mo->mesh->indexBufferView);
	ID3D12DescriptorHeap* ppHeaps[] = { mo->textureID->m_srvHeap.Get() };
	dxManager.getGraphicsCommandListComPtr()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	dxManager.getGraphicsCommandListComPtr()->SetGraphicsRootDescriptorTable(1, mo->textureID->descriptorTable);
	dxManager.getGraphicsCommandListComPtr()->SetGraphicsRoot32BitConstant(3, bi->start, 0);
	
	unsigned int instanceCount = bi->end - bi->start + 1;
	// DrawIndexedInstanced() will always produce 0 based instances, regardless of last parameter, we store start index and length in cbv[0].cameraPos x and y:
	CBV cbv = {};
	cbv.alpha = 0.1f; // regognize this in shader debug
	cbv.cameraPos.x = bi->start + 0.5f; // add 0.5 to get easy revert to int by floor in shader code
	cbv.cameraPos.y = instanceCount + 0.5f;
	// manual override for test:
	//instanceCount += 5;
	//cbv.cameraPos.x = /*bi->start*/1.0f + 0.5f; // add 0.5 to get easy revert to int by floor in shader code
	//cbv.cameraPos.y = instanceCount + 0.5f;

	dxManager.upload(0, 0, &cbv);

	dxManager.getGraphicsCommandListComPtr()->DrawIndexedInstanced(mo->mesh->numIndexes, instanceCount, 0, 0, 0 /*bi->start*/);
	//dxManager.getGraphicsCommandListComPtr()->DrawIndexedInstanced(mo->mesh->numIndexes, 3, 0, 0, 1);
}

void MeshObjectStore::createDrawBundle(MeshObject * meshObject)
{
	for (UINT n = 0; n < XApp::FrameCount; n++)
	{
		// Create one Bundle and Allocator for each frame:
		ThrowIfFailed(xapp().device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&meshObject->bundleCommandAllocators[n])));
		ThrowIfFailed(xapp().device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, meshObject->bundleCommandAllocators[n].Get(), pipelineState.Get(), IID_PPV_ARGS(&meshObject->bundleCommandLists[n])));
		meshObject->bundleCommandLists[n]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		meshObject->bundleCommandLists[n]->IASetVertexBuffers(0, 1, &meshObject->mesh->vertexBufferView);
		meshObject->bundleCommandLists[n]->IASetIndexBuffer(&meshObject->mesh->indexBufferView);
		ID3D12DescriptorHeap* ppHeaps[] = { meshObject->textureID->m_srvHeap.Get() };
		//meshObject->bundleCommandLists[frameIndex]->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		//meshObject->bundleCommandLists[frameIndex]->SetGraphicsRootDescriptorTable(2, meshObject->textureID->m_srvHeap->GetGPUDescriptorHandleForHeapStart());
		meshObject->bundleCommandLists[n]->DrawIndexedInstanced(meshObject->mesh->numIndexes, 1, 0, 0, 0);

		ThrowIfFailed(meshObject->bundleCommandLists[n]->Close());
		// do we need to synchronize? --> Hope not!
	}
	//meshObject->drawBundleAvailable = true;
}

bool MeshObjectStore::isSorted()
{
	// object numbers run from 1 to used_objects
	//Log("usedObjects " << used_objects << endl);
	unsigned int min = 0, max = 0, last = 0;
	forAll([this,&min,&max,&last](MeshObject *mo) {
		//Log("  objectNum: " << mo->objectNum << " mesh" << mo->mesh << " x " << mo->pos().x << endl);
		unsigned int cur = mo->objectNum;
		if (cur  < 1 || cur > this->used_objects)
			return false;
		if (cur != last + 1) return false;
		last = cur;
	});
	//for (auto & group : this->groups) {
	//	Log("isSorted() group: " << group.first.c_str() << endl);
	//	divideBulk(group.second.size(), 1, bulkInfos);
	//	vector<unique_ptr<MeshObject>>* mov = &group.second;
	//}
	return true;
}

void MeshObjectStore::divideBulk(size_t numObjects, size_t numParallel, vector<BulkDivideInfo>& subProblems)
{
	assert(numParallel >= 1);
	subProblems.clear();
	BulkDivideInfo bi;
	UINT totalNum = (UINT)numObjects;
	UINT perThread = totalNum / (UINT)numParallel;
	// adjust for rounding error: better to increase the count per thread by one instead of starting a new thread with very few elements:
	if (perThread * (UINT)numParallel < totalNum)
		perThread++;
	UINT count = 0;
	while (count < totalNum) {
		bi.start = count;
		bi.end = count + perThread - 1;
		if (bi.end >(totalNum - 1))
			bi.end = totalNum - 1;
		count += perThread;
		subProblems.push_back(bi);
	}
}

// compute shader

void MeshObjectStore::computeMethod(UINT frameNum)
{
	ID3D12CommandQueue* pCommandQueue = computeCommandQueue[frameNum].Get();
	ID3D12CommandAllocator* pCommandAllocator = computeAllocator[frameNum].Get();
	ID3D12GraphicsCommandList* pCommandList = computeCommandList[frameNum].Get();

	// Prepare for this next frame.
	ThrowIfFailed(pCommandAllocator->Reset());
	ThrowIfFailed(pCommandList->Reset(pCommandAllocator, computePipelineState.Get()));

	//ID3D12Resource *resource = singleCBVResourcesGPU_RW[frameNum].Get();
	ID3D12Resource *resource = dxManager.getConstantBuffer();
	resourceStateHelper->toState(resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, pCommandList);
	//pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	//pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));// ,
		//D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE));
	pCommandList->SetPipelineState(computePipelineState.Get());
	pCommandList->SetComputeRootSignature(computeRootSignature.Get());
	pCommandList->SetComputeRootUnorderedAccessView(0, dxManager.getCBVVirtualAddress(0, 0));
	pCommandList->SetComputeRootConstantBufferView(1, dxManager.getConstantBufferSetVirtualAddress(0, 0));
	UINT threadGroupCount = (UINT)ceil((maxObjects * 1.0f) / 1024);
	pCommandList->Dispatch(threadGroupCount, 1, 1);
	//resourceStateHelper->toState(resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, pCommandList);

	// Close and execute the command list.
	ThrowIfFailed(pCommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { pCommandList };

	PIXBeginEvent(pCommandQueue, 0, L"Frame %d: Iterate on CBV recalculation", frameNum);
	pCommandQueue->ExecuteCommandLists(1, ppCommandLists);
	PIXEndEvent(pCommandQueue);

	auto &f = frameData[frameNum];
	// Wait for the gpu to complete the draw.
	createSyncPoint(f, pCommandQueue);
	waitForSyncPoint(f); // ok, but not optimal

	// Prepare for the next frame.
	//ThrowIfFailed(pCommandAllocator->Reset());
	//ThrowIfFailed(pCommandList->Reset(pCommandAllocator, computePipelineState.Get()));
}