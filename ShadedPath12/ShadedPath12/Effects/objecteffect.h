// world object effect
class WorldObjectEffect : public EffectBase {
public:
	struct ConstantBufferFixed {
		XMFLOAT4 color;
	};
	struct ConstantBufferObject {
		XMFLOAT4X4 wvp;
		float    alpha;
	};

	void prepare();
	// update cbuffer and vertex buffer
	void update();
	//void draw();
	void draw(ComPtr<ID3D11Buffer> &vertexBuffer, ComPtr<ID3D11Buffer> indexBuffer, XMFLOAT4X4 wvp, long numIndexes, ID3D11ShaderResourceView **srv, float alpha = 1.0f);

private:
	ConstantBufferFixed cb;
	ConstantBufferObject cbo;
	// globally enable wireframe display of objects
	bool wireframe = false;
};
