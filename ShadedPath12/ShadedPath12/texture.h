
typedef int TextureID;
struct TextureInfo
{
	string id;
	wstring filename;
	ComPtr<ID3D12Resource> texSRV;
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	bool available; // true if this texture is ready for use in shader code
};

// Texture Store:
class TextureStore {
public:
	// init d3d resources needed to initialize/upload textures later
	void init();
	// load texture upload to GPU, textures are referenced via id string
	void loadTexture(wstring filename, string id);
	TextureInfo *getTexture(string id);
private:
	unordered_map<string, TextureInfo> textures;
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12RootSignature> rootSignature;
	FrameResource updateFrameData;

};