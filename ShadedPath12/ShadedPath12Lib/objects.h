//class WorldObjectEffect;

struct AnimationClip {
	std::string name;
	std::vector<XMFLOAT4X4> invBindMatrices;
	std::vector<XMFLOAT4X4> boneBindPoseMatrices;
	int numBones;
	int parents[128];
};

class Mesh
{
public:
	//void createVertexAndIndexBuffer(WorldObjectEffect *worldObjectEffect);
	vector<WorldObjectVertex::VertexTextured> vertices;
	vector<WorldObjectVertex::VertexSkinned> skinnedVertices;
	vector<DWORD> indexes;
	unordered_map<string, AnimationClip> clips;
	long numVertices;
	long numIndexes;
	// all objects based on this mesh:
	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> vertexBufferUpload;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	ComPtr<ID3D12Resource> indexBuffer;
	ComPtr<ID3D12Resource> indexBufferUpload;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	//vector<Action> actions;
	unordered_map<string, Action> actions;
	void initBoundigBox();
	void addToBoundingBox(XMFLOAT3 p);
	void getBoundingBox(BoundingBox &b);
private:
	// for calculating bounding box:
	XMFLOAT3 bboxVertexMin;
	XMFLOAT3 bboxVertexMax;
};

class MeshLoader {
public:
	// load asset from full path/filename
	void loadBinaryAsset(wstring filename, Mesh *mesh, float scale = 1.0f, XMFLOAT3 *displacement = nullptr);
};


class WorldObject {
public:
	static UINT count; // count all objects
	WorldObject();
	virtual ~WorldObject();

	XMFLOAT3& pos();
	XMFLOAT3& rot();
	XMFLOAT4 quaternion;
	bool useQuaternionRotation = false;
	void update(); // only relevant for bone objects
	// draw skeleton with lines
	void drawSkeleton(XMFLOAT4 color, Path* path, LinesEffect *linesEffect, float time, unsigned long user);
	// draw a mesh by going through vertices list
	// each triangel is made up of 3 points from the list
	//void drawMeshFromTriangleLines(vector<WorldObjectVertex::VertexSkinned>* vertices, LinesEffect* linesEffect, unsigned long user);
	void drawMeshFromTriangleLines(vector<WorldObjectVertex::VertexTextured>* vertices, LinesEffect* linesEffect, float time, unsigned long user);
	void drawSkeletonFromLines(PathDesc* pathDescBone, vector<WorldObjectVertex::VertexTextured>* vertices, LinesEffect* linesEffect, float time, unsigned long user);
	// find root bone, then multiply all bone pose matrices together to get final transform
	// for going from root bone space to bone space
	XMMATRIX bonePoseTransform(int poseIndex, PathDesc* pathDescBone, float time, unsigned long user);
	// transform an array of points along bones of a skeleton
	template<size_t sz>
	void transformAlongBones(array<XMFLOAT3,sz> &points, int poseIndex, PathDesc* pathDescBone, float time, unsigned long user);
	void draw();
	Mesh *mesh;
	TextureID textureID;
	float alpha;
	Action *action; // move action
	Action *boneAction; // action involving bone calcualtions
	PathDesc* pathDescMove;  // moving object
	PathDesc* pathDescBone;  // animating object
	bool disableSkinning = false; // set to true for animated object to use as static meshes
	bool isNonKeyframeAnimated = false; // signal that poses are not interpoalted by Path, but computed outside and set in update()

	int visible; // visible in current view frustrum: 0 == no, 1 == intersection, 2 == completely visible
	void setAction(string name);
	// return current bounding box by scanning all vertices, used for bone animated objects
	// if maximise is true bounding box may increase with each call, depending on current animation
	// (used to get max bounding box for animated objects)
	void calculateBoundingBox(BoundingBox &box, bool maximise = false);
	// override the bounding box from mesh data, useful for bone animated objects
	void forceBoundingBox(BoundingBox box);
	// get bounding box either from mesh data, or the one overridden by forceBoundingBox()
	void getBoundingBox(BoundingBox &box);
	XMFLOAT3 objectStartPos;
	// 3d sound 
	//int soundListIndex;  // index into audibleWorldObjects, used to get the 3d sound settings for this object, see Sound.h
	bool stopped; // a running cue may temporarily stopped
	bool playing; // true == current cue is actually running
				  //D3DXVECTOR3 soundPos;
				  //IXACT3Cue* cue;
	SoundDef *soundDef = nullptr;
	int maxListeningDistance; // disable sound if farther away than this
							  //int soundBankIndex;  // sound bank to use for this object
							  //WORD cueIndex; // cue index
	float scale;
	bool drawBoundingBox;
	bool drawNormals;
	Material material;
	XMMATRIX calcToWorld();
	UINT objectNum; // used for single CBV buffer calculations - must be unique for all objects
	int threadNum; // only used in multi thread drawing
private:
	XMFLOAT3 _pos;
	XMFLOAT3 _rot;
	XMFLOAT3 bboxVertexMin = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
	XMFLOAT3 bboxVertexMax = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	bool boundingBoxAlreadySet = false;
	BoundingBox boundingBox;
};

// WorldObject Store:
class WorldObjectStore {
public:
	// objects
	// load object definition from .b file, save under given hash name
	void loadObject(wstring filename, string id, float scale = 1.0f, XMFLOAT3 *displacement = nullptr);
	// add loaded object to scene
	void addObject(string groupname, string id, XMFLOAT3 pos, TextureID tid = 0);
	void addObject(WorldObject &w, string id, XMFLOAT3 pos, TextureID tid = 0);
	// obbject groups: give fast access to specific objects (e.g. all worm NPCs)
	void createGroup(string groupname);
	const vector<unique_ptr<WorldObject>> *getGroup(string groupname);
	// draw all objects within a group (all have same mesh), set threadNum > 1 to draw with multiple threads
	void drawGroup(string groupname, size_t threadNum = 0);
	//void setWorldObjectEffect(WorldObjectEffect *objectEffect);
	//WorldObjectEffect*  getWorldObjectEffect() { return objectEffect; };
private:
	unordered_map<string, vector<unique_ptr<WorldObject>>> groups;
	unordered_map<string, Mesh> meshes;
	void addObjectPrivate(WorldObject *w, string id, XMFLOAT3 pos, TextureID tid);
	//WorldObjectEffect *objectEffect = nullptr;
};