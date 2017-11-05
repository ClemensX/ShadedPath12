# Mesh Object Shader

## Details for non-VR rendering:

**TextureStore::getTexture()** creates a descriptor table as last step. That can be directly bound to a root descriptor table slot:

```C++
	ID3D12DescriptorHeap* ppHeaps[] = { mo->textureID->m_srvHeap.Get() };
	dxManager.getGraphicsCommandListComPtr()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	dxManager.getGraphicsCommandListComPtr()->SetGraphicsRootDescriptorTable(1, mo->textureID->descriptorTable);
```
```
	 "DescriptorTable(SRV(t0, space = 0)), "
```

