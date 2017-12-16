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

## Control Flow

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

| TextureInfo *House*   |  GPU Mem   |
| ------------- | -----:|
| texSRV      |  100 |
| descriptorTable      |    500 |

| TextureInfo *Meteor*   |  GPU Mem   |
| ------------- | -----:|
| texSRV      |  200 |
| descriptorTable      |    600 |


| GPU Memory   |               |       | # Entries|GPU Mem Ref
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
