
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
	// load texture upload to GPU, textures are referenced via id string
	void loadTexture(wstring filename, string id, ID3D12GraphicsCommandList *commandList, TextureLoadResult &result);
	TextureInfo *getTexture(string id);
private:
	unordered_map<string, TextureInfo> textures;
};