#pragma once

class Linetext;
class Dotcross;
//struct LineDef;
class XApp;
class Mesh;
class WorldObject;
//class Path;
class LinesEffect;

struct Grid {
	XMFLOAT4 center;
	float width; // total width (along x axis) of grid in units
	float depth; // total depth (along z axis) in units
	int widthCells; // number of cells to separate along x axis
	int depthCells; // number of cells to separate along z axis
	// store grid line endpoints in world coords, each pair of endpoints denotes one grid line to draw
	//vector<XMFLOAT4> zLineEndpoints; // parallel to z axis
	//vector<XMFLOAT4> xLineEndpoints; // parallel to x axis
	// lines mode:
	//vector<LineDef> lines;
	// triangle mode with vertices and indexes:
	vector<XMFLOAT3> vertices;
	vector<XMFLOAT2> tex;  //texture coords, stretch texture over entire area
	vector<UINT> indexes;
};

//struct PointLightStored {
//	PointLight l;
//	int id;
//};
//
//struct DirectionalLightStored {
//	DirectionalLight l;
//	int id;
//};

class World
{
public:
	World(XApp *xapp);
	~World();

	// effect utils

	// draw box (used for bounding box drawing)
	void drawBox(BoundingBox &box);
	// draw coordinate system at world position, each axis will be designated via labels
	void drawCoordinateSystem(XMFLOAT4 point, string label, Linetext &linetextEffect, Dotcross& dotcrossEffect, float charHeight = 0.01f);
	// create Grid in x (width) and z (depth) direction, linesmode means create simple long lines, otherwise create vertex/index vectors
	void createGridXZ(Grid& grid, bool linesmode = true);
	Grid* createWorldGrid(float lineGap, float verticalAdjust = 0.0f);
	void setWorldSize(float x, float y, float z) { sizex = x; sizey = y; sizez = z; };

	/*	// objects
	// load object definition from .b file, save under given hash name
	void loadObject(wstring filename, string id);
	// add loaded object to scene
	void addObject(WorldObject &w, string id, XMFLOAT3 pos, TextureID tid = NIL);
	// obbject groups: give fast access to specific objects (e.g. all worm NPCs)
	void createGroup(string groupname);
	vector<WorldObject> *getGroup(string groupname);
	void addObject(string groupname, string id, XMFLOAT3 pos, TextureID tid = NIL);
	*/

	// lights
	//vector<PointLightStored> pointLights;
	//vector<DirectionalLightStored> directionalLights;
	//// add light and give id to easily get it back (will be position in list if none given)
	//void addPointLight(PointLight pLight, int id = -1);
	//void addDirectionalLight(DirectionalLight dLight, int id = -1);
	//DirectionalLight& getDirectionalLight(int id);
	//PointLight& getPointLight(int id);

	// textures
	void setTextures(vector<TextureInfo> &ti);
	//TextureInfo *getTexture(TextureID id);
	//void createRandomTexture1DSRV(ComPtr<ID3D11ShaderResourceView> &randomTex);

	// one central Path instance needed:
	//Path path;
	//XApp* getApp() { return xapp; };

	//unique_ptr<WorldObjectStore> objectStore;

	// randomly generate ine position within the defined world coords
	XMFLOAT3 getRandomPos();
	// randomly generate ine position within the defined world coords with a given minimum height
	XMFLOAT3 getRandomPos(float minHeight);
	LinesEffect *linesEffect = nullptr;
private:
	// world size in absolute units around origin, e.g. x is from -x to x
	float sizex, sizey, sizez;
	XApp* xapp;
	//unordered_map<string, Mesh> meshes;
	//vector<TextureInfo> textureInfos;
	//unordered_map<TextureID, TextureInfo> textureMap;
	//unordered_map<string, vector<WorldObject>> groups;
	//void addObjectPrivate(WorldObject *w, string id, XMFLOAT3 pos, TextureID tid);
	Grid grid;
};

