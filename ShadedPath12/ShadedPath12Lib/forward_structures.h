
// WorldObjectEffect vertex definitions need to be known before objecteffect.h can be included,
// so we define these Vertex structs here
class WorldObjectVertex {
public:
	struct VertexTextured {
		XMFLOAT3 Pos;
		XMFLOAT3 Normal;
		XMFLOAT2 Tex;
	};
	struct VertexSkinned {
		XMFLOAT3 Pos;
		XMFLOAT3 Normal;
		XMFLOAT2 Tex;
		XMFLOAT4 Weights;
		BYTE BoneIndices[4];
	};
};

