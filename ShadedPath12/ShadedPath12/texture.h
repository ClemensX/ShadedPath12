
struct TextureInfo
{
	string id;
	wstring filename;
	ComPtr<ID3D12Resource> texSRV;
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	// new apparoach: pre define a descriptor table for each texture, just set this with SetGraphicsRootDescriptorTable
	D3D12_GPU_DESCRIPTOR_HANDLE descriptorTable;
	bool available; // true if this texture is ready for use in shader code
};
typedef TextureInfo* TextureID;

// Texture Store:
class TextureStore {
public:
	// init d3d resources needed to initialize/upload textures later
	void init(XApp *xapp);
	// load texture upload to GPU, textures are referenced via id string
	void loadTexture(wstring filename, string id);
	TextureInfo *getTexture(string id);
private:
	unordered_map<string, TextureInfo> textures;
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	FrameResourceSimple updateFrameData;
	XApp* xapp = nullptr;
};