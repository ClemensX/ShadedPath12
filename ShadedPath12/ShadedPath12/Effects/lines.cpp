#include "stdafx.h"
#include "lines.h"

void LinesEffect::init()
{
}

void LinesEffect::add(vector<LineDef> &linesToAdd) {
	if (linesToAdd.size() == 0 && lines.size() == 0)
		return;
	lines.insert(lines.end(), linesToAdd.begin(), linesToAdd.end());
	dirty = true;
}

void LinesEffect::update() {
	// handle fixed lines:
	if (dirty) {
		// recreate vertex input buffer
		dirty = false;
		vector<Vertex> all;
		for (LineDef line : lines) {
			Vertex v1, v2;
			v1.color = line.color;
			v1.pos = line.start;
			v2.color = line.color;
			v2.pos = line.end;
			all.push_back(v1);
			all.push_back(v2);
		}
		UINT vertexBufferSize = sizeof(Vertex) * lines.size() * 2; //sizeof(triangleVertices);

		ThrowIfFailed(xapp().device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&vertexBuffer)));
		vertexBuffer.Get()->SetName(L"vertexBuffer_lines");

		ThrowIfFailed(xapp().device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexBufferUpload)));
		vertexBufferUpload.Get()->SetName(L"vertexBufferUpload_lines");

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the vertex buffer.
		D3D12_SUBRESOURCE_DATA vertexData = {};
		vertexData.pData = reinterpret_cast<UINT8*>(&(all.at(0)));
		//vertexData.pData = &(all.at(0));
		//vertexData.pData = reinterpret_cast<UINT8*>(triangleVertices);
		vertexData.RowPitch = vertexBufferSize;
		vertexData.SlicePitch = vertexData.RowPitch;

		UpdateSubresources<1>(xapp().commandList.Get(), vertexBuffer.Get(), vertexBufferUpload.Get(), 0, 0, 1, &vertexData);
		xapp().commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		// Initialize the vertex buffer view.
		vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = sizeof(Vertex);
		vertexBufferView.SizeInBytes = vertexBufferSize;
	}
}

void LinesEffect::draw()
{
	xapp().commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	xapp().commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	xapp().commandList->DrawInstanced(6, 1, 0, 0);
}

