
typedef int TextureID;
struct TextureInfo
{
	string id;
	wstring filename;
	ComPtr<ID3D12Resource> texSRV;
};


// Texture Store:
class TextureStore {
public:
	// load texture upload to GPU, textures are referenced via id string
	void loadTexture(wstring filename, string id);
private:
	unordered_map<string, TextureInfo> textures;
};