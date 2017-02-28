class MeshObject {
public:
	MeshObject();
	virtual ~MeshObject();
	//XMFLOAT4 pos;
	XMFLOAT3& pos();
	XMFLOAT4 rot_quaternion;
	float scale = 1.0f;
	bool drawBoundingBox = false;
	bool drawNormals = false;
	Material material;
};