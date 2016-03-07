
#define float4 XMFLOAT4
#define float3 XMFLOAT4
#define float2 XMFLOAT2

#include "Shaders\lights_basic.hlsi"

#undef float4
#undef float3
#undef float2

class Lights {
public:
	void createConstantBuffer(size_t s, wchar_t * name);
	ComPtr<ID3D12Resource> cbvResource;
	UINT8* cbvGPUDest;  // mempy changed cbv data to this address before draw()
	void update();
	void init();
	CBVLights lights;
};