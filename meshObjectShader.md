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


| TextureInfo *House*   |||
| ------------- | ---  | -----:|
|**Field Name** | **Type** | **GPU Address**  |
| texSRV      |  ShaderResourceView|100 |
| descriptorTable |  DescriptorTable   |    500 |

| TextureInfo *Meteor*  |||
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

### Constant Buffers

All buffers are created for each render frame [0..2], so real memory consumption is 3 times the sizes give below
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

**constantBufferUpload:**
 * big buffer for all singleCBVResources (all 3 frames)
 * used to memcpy() changed object data to GPU

## Update Phase

```C++
    MassTest2::update();
        xapp().lights.update();
        objStore->update();
```
Update in MeshObjectStore:
 * each group (*default, house, meteor*) is handled seprartely
 * all group objects are in one contiguous area 
 * prepare compute shader by uploading constants: view projection matrix, # of objects and first objectNum to be used
 * run compute shader

| MeshObject Layout|MeshObjectStore.groups | |
| ------------- | ---  | -----:|
|**Index in group** | **ObjectNum** | **Group Name**  |
| 0      |  1|default |
| 0      |  2|house |
| 1      |  3|house |
| 2      |  4|house |
| 3      |  5|house |
| 4      |  6|house |
| 0      |  7|meteor |
| 1      |  8|meteor |
| 2      |  9|meteor |
| 3      |  10|meteor |
| 4      |  11|meteor |

### Compute Shader

* copy changed object constant data to GPU for compute shader usage
* object data should only be copied once, then changed by compute shader as needed (currently not implemented - objects are fixed in pos and rot)
* currently the update from CPU mem to GPU mem for object data is a complete copy for **all** objects
* system keeps track if all objects in all groups are already copied to GPU. Then no more copy will be done
