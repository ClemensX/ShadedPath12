#include "stdafx.h"

#include "CompiledShaders/MObjectVS.h"
#include "CompiledShaders/MObjectPS.h"
#include "CompiledShaders/MObjectCS.h"

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
	XMFLOAT3 p = XMFLOAT3(0.0f, 0.0f, 0.0f);//rot();
	//WegDamit2.lock();
	XMMATRIX rotateM = XMMatrixRotationRollPitchYaw(p.y, p.x, p.z);
	//WegDamit2.unlock();
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

unordered_map<UINT8*, UINT8*> adds;
unordered_map<unsigned int, unsigned int> objNums;

void MeshObjectStore::updateOne(CBV const *cbv_read, MeshObject *mo, XMMATRIX vp, int frameIndex, int eyeNum) {
	assert(mo->objectNum > 0); // not properly added to store
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
	//cbv->world = di.world;
	cbv->alpha = mo->alpha;
	// only update material if necessary
	size_t size = frameEffectData[frameIndex].updateMaterial ? sizeof(*cbv) : sizeof(*cbv) - sizeof(Material);
	memcpy(getCBVUploadAddress(frameIndex, 0, mo->objectNum, eyeNum), cbv, size);
	dxManager.upload(mo->objectNum, eyeNum, cbv);
	if (false && mo->pos().y > 50) {
		auto pos = mo->mesh->vertices.at(0).Pos;
		XMVECTOR p = XMVectorSet(pos.x, pos.y, pos.z, 0.0f);
		p = XMVector3Transform(p, XMMatrixTranspose(toWorld));
		if (XMVectorGetY(p) < 50.0f) {
			Log("assert(false) " << pos.y << " " << XMVectorGetY(p) << endl);
			DebugBreak();
		}

	}
	if (false) {
		// non-threadsave checks - use with 1 thread only
		assert(objNums.count(mo->objectNum) == 0);  // ensure no double obj id found
		objNums[mo->objectNum] = 1;
		UINT8 *mem = getCBVUploadAddress(frameIndex, 0, mo->objectNum, eyeNum);
		assert(adds.count(mem) == 0); // ensure no double mem pointers used
		adds[mem] = nullptr;
	}
}

void MeshObjectStore::updatePart(BulkDivideInfo bi, CBV * cbv, vector<unique_ptr<MeshObject>>* mov, XMMATRIX vp, int frameIndex, int eyeNum)
{
	for (unsigned int i = bi.start; i <= bi.end; i++) {
		//updateOne(cbv, mov[i].get(), vp, frameIndex, eyeNum);
		updateOne(cbv, mov->at(i).get(), vp, frameIndex, eyeNum);
	}
}

void MeshObjectStore::update()
{
	if (xapp().isShutdownMode()) return;
	adds.clear();
	objNums.clear();
	xapp().stats.start("meshStoreUpdate");
	assert(this->maxObjects > 0);	// setting of max object count missing
	int frameIndex = xapp().getCurrentBackBufferIndex();
	dxManager.setCurrentFrame(frameIndex);

	frameEffectData[frameIndex].camera = xapp().camera;

	Camera *cam = &frameEffectData[frameIndex].camera;
	CBV my_cbv;
	CBV *cbv = &my_cbv;
	prepareDraw(&xapp().vr);
	xapp().lights.update();
	xapp().stats.start("compute");
	//computeMethod(frameIndex);
	xapp().stats.end("compute");
	if (!xapp().ovrRendering) {
		frameEffectData[frameIndex].vr_eyesm[0] = vr_eyes;
		XMMATRIX vp = cam->worldViewProjection();
		cbv->cameraPos.x = cam->pos.x;
		cbv->cameraPos.y = cam->pos.y;
		cbv->cameraPos.z = cam->pos.z;
		for (auto & group : this->groups) {
			//Log("group: " << group.first.c_str() << endl);
			divideBulk(group.second.size(), 1, bulkInfos);
			vector<unique_ptr<MeshObject>>* mov = &group.second;
			if (bulkInfos.size() == 1 && true) {
				// simple update of all
				updatePart(bulkInfos[0], cbv, mov, vp, frameIndex);
			} else {
				vector<thread> threads;
				//thread t(&MeshObjectStore::updatePart, this, bulkInfos[0], cbv, group.second, vp, frameIndex);
				for (int i = 0; i < bulkInfos.size(); i++) {
					thread t([=] { this->updatePart(bulkInfos[i], cbv, mov, vp, frameIndex); });
					threads.push_back(move(t));
				}
				for (auto &t : threads) {
					t.join();
				}
			}
		}

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
	frameEffectData[frameIndex].updateMaterial = false;
	dxManager.copyToComputeBuffer(frameData[frameIndex]);
	computeMethod(frameIndex);
	xapp().stats.end("meshStoreUpdate");
}

void MeshObjectStore::draw()
{
	if (xapp().isShutdownMode()) return;
	assert(this->maxObjects > 0);	// setting of max object count missing
	prepareDraw(&xapp().vr);
	preDraw();
	if (!xapp().ovrRendering) {
		forAll([this](MeshObject *mo) {
			//Log("  elem: " << mo->pos().x << endl);
			drawInternal(mo);
		});
	} else {
		forAll([this](MeshObject *mo) {
			//Log("  elem: " << mo->pos().x << endl);
			drawInternal(mo, 0);
			drawInternal(mo, 1);
		});
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
	assert(this->maxObjects > 0);	// setting of max object count missing
	dxManager.init(xapp().device.Get());

	// try to do all expensive operations like shader loading and PSO creation here
	// Create the pipeline state, which includes compiling and loading shaders.
	{
		createRootSigAndPSO(rootSignature, pipelineState);
		dxManager.createConstantBuffer(1, maxObjects, sizeof(cbv), L"mesheffect_cbvsingle_resource");
		dxManager.createGraphicsExecutionEnv(pipelineState.Get());
		dxManager.createUploadBuffers();
		setSingleCBVMode(1, maxObjects, sizeof(cbv), L"mesheffect_cbvsingle_resource", true);
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
		// Create compute resources.
		D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_COMPUTE, 0, D3D12_COMMAND_QUEUE_FLAG_NONE };
		ThrowIfFailed(xapp().device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&computeCommandQueue[n])));
		ThrowIfFailed(xapp().device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&computeAllocator[n])));
		ThrowIfFailed(xapp().device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, computeAllocator[n].Get(), nullptr, IID_PPV_ARGS(&computeCommandList[n])));
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
	ThrowIfFailed(commandAllocators[frameIndex]->Reset());
	ThrowIfFailed(commandLists[frameIndex]->Reset(commandAllocators[frameIndex].Get(), pipelineState.Get()));
	// Set necessary state.
	commandLists[frameIndex]->SetGraphicsRootSignature(rootSignature.Get());

	// TODO adapt for 2 eyes:
	commandLists[frameIndex]->RSSetViewports(1, &vr_eyes.viewports[0]);
	commandLists[frameIndex]->RSSetScissorRects(1, &vr_eyes.scissorRects[0]);
	// TODO check
	//commandLists[frameIndex]->RSSetViewports(1, &vr_eyes.viewports[eyeNum]);
	//commandLists[frameIndex]->RSSetScissorRects(1, &vr_eyes.scissorRects[eyeNum]);
	//commandLists[frameIndex]->RSSetViewports(1, xapp().vr.getViewport());
	//commandLists[frameIndex]->RSSetScissorRects(1, xapp().vr.getScissorRect());

	// Set CBVs
	//commandLists[frameIndex]->SetGraphicsRootConstantBufferView(0, getCBVVirtualAddress(frameIndex, 0, di.objectNum, 0));
	commandLists[frameIndex]->SetGraphicsRootConstantBufferView(1, xapp().lights.cbvResource->GetGPUVirtualAddress());

	// Indicate that the back buffer will be used as a render target.
	//	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(xapp().rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, xapp().rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = xapp().getRTVHandle(frameIndex);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(xapp().dsvHeaps[frameIndex]->GetCPUDescriptorHandleForHeapStart());
	//m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	commandLists[frameIndex]->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	ID3D12Resource *resource;
	if (!xapp().ovrRendering) resource = xapp().renderTargets[frameIndex].Get();
	else resource = xapp().vr.texResource[frameIndex];
	xapp().handleRTVClearing(commandLists[frameIndex].Get(), rtvHandle, dsvHandle, resource);

}

void MeshObjectStore::postDraw()
{
	int frameIndex = xapp().getCurrentBackBufferIndex();

	ThrowIfFailed(commandLists[frameIndex]->Close());
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { commandLists[frameIndex].Get() };
	xapp().commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void MeshObjectStore::drawInternal(MeshObject *mo, int eyeNum)
{
	int frameIndex = xapp().getCurrentBackBufferIndex();
	if (mo->drawBundleAvailable) {
		commandLists[frameIndex]->RSSetViewports(1, &vr_eyes.viewports[eyeNum]);
		commandLists[frameIndex]->RSSetScissorRects(1, &vr_eyes.scissorRects[eyeNum]);
		commandLists[frameIndex]->SetGraphicsRootConstantBufferView(0, getCBVVirtualAddress(frameIndex, 0, mo->objectNum, eyeNum));  // set to beginning of all object buffer
																																	 // Set SRV
		ID3D12DescriptorHeap* ppHeaps[] = { mo->textureID->m_srvHeap.Get() };
		commandLists[frameIndex]->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		commandLists[frameIndex]->SetGraphicsRootDescriptorTable(2, mo->textureID->m_srvHeap->GetGPUDescriptorHandleForHeapStart());
		commandLists[frameIndex]->ExecuteBundle(mo->bundleCommandLists[frameIndex].Get());
		return;
	}
	commandLists[frameIndex]->RSSetViewports(1, &vr_eyes.viewports[eyeNum]);
	commandLists[frameIndex]->RSSetScissorRects(1, &vr_eyes.scissorRects[eyeNum]);
	commandLists[frameIndex]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandLists[frameIndex]->IASetVertexBuffers(0, 1, &mo->mesh->vertexBufferView);
	commandLists[frameIndex]->IASetIndexBuffer(&mo->mesh->indexBufferView);
	commandLists[frameIndex]->SetGraphicsRootConstantBufferView(0, getCBVVirtualAddress(frameIndex, 0, mo->objectNum, eyeNum));  // set to beginning of all object buffer
	// Set SRV
	ID3D12DescriptorHeap* ppHeaps[] = { mo->textureID->m_srvHeap.Get() };
	commandLists[frameIndex]->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	commandLists[frameIndex]->SetGraphicsRootDescriptorTable(2, mo->textureID->m_srvHeap->GetGPUDescriptorHandleForHeapStart());
	commandLists[frameIndex]->DrawIndexedInstanced(mo->mesh->numIndexes, 1, 0, 0, 0);
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
	meshObject->drawBundleAvailable = true;
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

	//ID3D12Resource *resource = singleCBVResourcesGPU_RW[frameNum].Get();
	ID3D12Resource *resource = dxManager.getConstantBuffer();
	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	//pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));// ,
		//D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_BARRIER_FLAG_NONE));
	pCommandList->SetPipelineState(computePipelineState.Get());
	pCommandList->SetComputeRootSignature(computeRootSignature.Get());

	pCommandList->Dispatch(1, 1, 1);

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
	ThrowIfFailed(pCommandAllocator->Reset());
	ThrowIfFailed(pCommandList->Reset(pCommandAllocator, computePipelineState.Get()));
}