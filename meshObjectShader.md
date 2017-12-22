# Mesh Object Shader

## Textures

**TextureStore::getTexture()** creates a descriptor table as last step. That can be directly bound to a root descriptor table slot:

```C++
	ID3D12DescriptorHeap* ppHeaps[] = { mo->textureID->m_srvHeap.Get() };
	dxManager.getGraphicsCommandListComPtr()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	dxManager.getGraphicsCommandListComPtr()->SetGraphicsRootDescriptorTable(1, mo->textureID->descriptorTable);
```
```
	 "DescriptorTable(SRV(t0, space = 0)), "
```

## Compute Shader

View Matrices (later also object rotation and movement) are done in compute shader **MObjectCS.hlsl**

## Init Phase

Example App mass2.cpp/.h used:

```C++
	MeshObjectStore *objStore = nullptr;
```


```C++
    MassTest2::init();
        objStore->setMaxObjectCount(NUM_METEOR + 2);
        objStore->init();
        objStore->gpuUploadPhaseStart();
        //// prepare textures, groups and meshes
        // load texture from file
        xapp().textureStore.loadTexture(L"met1.dds", "meteor1");
        // create texture object
        TextureInfo *Meteor1Tex = xapp().textureStore.getTexture("meteor1");
        // prepare groups
        objStore->createGroup("default");
        // load mesh objects from file with optional size factor
        objStore->loadObject(L"house4_anim.b", "House");
        objStore->loadObject(L"meteor_single.b", "Meteor", 0.3f);
        // add objects to scene: set group, id, start position and texture
        o = objStore->addObject("default", "House", XMFLOAT3(100.0f, 1.0f, 1.0f), Meteor1Tex);
        objStore->gpuUploadPhaseEnd();
```
#### Texture Memory Layout after gpuUploadPhase

Example data for two textures loaded: *House* and *Meteor*
(all addresses are fake numbers)


| TextureInfo *House*   
| ------------- | ---  | -----:|
|**Field Name** | **Type** | **GPU Address**  |
| texSRV      |  ShaderResourceView|100 |
| descriptorTable |  DescriptorTable   |    500 |

| TextureInfo *Meteor*  
| ------------- | ---  | -----:|
|**Field Name** | **Type** | **GPU Address**  |
| texSRV        |ShaderResourceView|  200 |
| descriptorTable     |  DescriptorTable  |    600 |


| GPU Memory   |               |       | # Entries|GPU Reference
| -------------|:-------------:| -----:| --------:|--:
||||
||||
||||
| 100      | SRV *House*      |   Texture Data |
||||
| 200      | SRV *Meteor*      |   Texture Data |
||||
||||
| 500      | DescriptorTable      ||  1|100
||||
| 600      | DescriptorTable      ||  1|200

## Update Phase

```C++
    MassTest2::update();
        xapp().lights.update();
        objStore->update();
```
### Constant Buffers

```C++
    MeshObjectStore::init()
        dxManager.createConstantBuffer(1, maxObjects+1, sizeof(cbv), L"mesheffect_cbvsingle_resource");
        dxManager.createConstantBufferSet(0, 1, 1, sizeof(CBV_CS), L"mesheffect_cs_cbv_set");
        dxManager.createGraphicsExecutionEnv(pipelineState.Get());
        dxManager.createUploadBuffers();
        setSingleCBVMode(1, maxObjects+1, sizeof(cbv), L"mesheffect_cbvsingle_resource", true);
```

| ConstantBuffer mesheffect_cbvsingle_resource|```vector<ComPtr<ID3D12Resource>> singleCBVResources```|  class DXManager
| ------------- | ---  | -----:|
|**Attribute** | **Type and Remarks** | **Usage in MeshObject**  |
| singleObjectSize        |size_t, must be multiple of 16 Bytes|  256 |
| maxThreads        |# Threads, each needs its own constant buffer|  1 |
| slotSize | singleObjectSize --> multiple of 256|256
|totalSize | | slotSize * (maxObjects + 1)

| ConstantBuffer mesheffect_cs_cbv_set|```vector<ComPtr<ID3D12Resource>> singleCBVResources```|  class DXManager
| ------------- | ---  | -----:|
|**Attribute** | **Type and Remarks** | **Usage in MeshObject**  |
|totalSize | constant buffer for compute shader |80 (256)

<!---

| Tables        | Are           | Cool  |
| ------------- |-------------:| -----:|
| col 3 is      | right-aligned | $1600 |
| col 2 is      | centered      |   $12 |
| zebra stripes | are neat      |    $1 |

Markdown | Less | Pretty
--- | --- | ---
*Still* | `renders` | **nicely**
1 | 2 | 3

-->
