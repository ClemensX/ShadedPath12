#include "stdafx.h"

void Lights::createConstantBuffer(size_t s, wchar_t * name)
{
	UINT cbvSize = calcConstantBufferSize((UINT)s);
	ThrowIfFailed(xapp().device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE, // do not set - dx12 does this automatically depending on resource type
		&CD3DX12_RESOURCE_DESC::Buffer(cbvSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&cbvResource)));
	cbvResource.Get()->SetName(name);
	//Log("GPU virtual: " <<  cbvResource->GetGPUVirtualAddress(); << endl);
	ThrowIfFailed(cbvResource->Map(0, nullptr, reinterpret_cast<void**>(&cbvGPUDest)));
}

// constant defining the lights buffer
// ambient, directional, point, spot
// ALWAYS keep this header in sync with lights.hlsli !!!!


void Lights::init() {
	UINT size = (UINT) sizeof(lights);
	createConstantBuffer(size, L"Lights_cbv_resource");
}

void Lights::update() {
	memcpy(cbvGPUDest, &lights, sizeof(lights));
}
