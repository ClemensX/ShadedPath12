#pragma once

struct Curve;
struct AnimationClip;
class WorldObject;
class Terrain;
//class WorldObjectEffect;
//struct WorldObjectEffect::VertexSkinned;

struct SegmentInfo {
	float length;
	float cumulated_length;
	float cumulated_start;
};

enum PathMode
{
	Path_SimpleMode,	// use path as is: stop at last pos forever
	Path_Reverse,		// reverse path once end reached, play back and forth
	Path_Closure,		// move from last pos to start pos and start again
	Path_Loop,			// use path as is but jump to start pos after reaching end pos and play again
	Path_Random			// NPC pathing: move freely in defined area
};

enum AnimationState
{
	Anim_Move,			// move linearily along pre computed vector
	Anim_Turn,			// rotate around x axis
	Anim_Stopped			// stand still - do nothing
};

struct PathDesc {
	float speed;
	XMFLOAT3 *pos;
	XMFLOAT3 *look;
	int curSegment;
	int numSegments;
	LONGLONG starttime;
	XMFLOAT3 currentPos;
	double starttime_f;
	float fps;
	XMFLOAT3 segment_start_pos;
	SegmentInfo *segments;
	PathMode pathMode;
	bool currentReverseRun;
	bool isLastPos;
	bool isBoneAnimation;
	double now;
	double percentage;
	// bone animation specific fields
	const AnimationClip *clip;
	const std::vector<Curve> *lastCurves;
	double lastPercentage;
	int lastSegment;
	AnimationState animState;
	float yawSourceAngle; // store start angle during turns
	float yawTargetAngle; // store final angle during turns
	LONGLONG num_ticks; // time for current movement/turn in game ticks
	XMFLOAT3 sourcePos; // start position for current linear move (only used in npc pathing)
	XMFLOAT3 targetPos; // end pos for current move
	float minturn, maxturn, turnspeed, minPathLen, maxPathLen;
	bool disableDraw;
	bool handleRotation; // no auto rotation along path movement
	XMFLOAT4X4 interpolationMatrices[100];
	XMFLOAT4X4 interpolationMatricesChained[100];  // all children already premultiplied by parent
};


class Path
{
public:
	static const int PATHID_TITLEPATH = 0;
	static const int PATHID_CAM_INTRO = 1;
	Path(void);
	~Path(void);
	XMFLOAT3& getPos(int pathID, LONGLONG now, LONGLONG ticks_per_second);
	void getPos(WorldObject &o, double nowf, XMFLOAT3 &pos, XMFLOAT3 &rot);
	void updateTime(WorldObject *o, double nowf);
	void recalculateBoneAnimation(PathDesc *pathDesc, WorldObject *wo, double time);
	void skin(XMVECTOR &pos, XMVECTOR &norm, const WorldObjectVertex::VertexSkinned *v, PathDesc *pd);
	void updateScene(PathDesc *pathDesc, WorldObject *wo, double time);
	void addRandomNPC(WorldObject *wo, char *name);
	PathDesc *createNpcPath(char *name);
	void moveNpc(WorldObject *wo, LONGLONG now, LONGLONG ticks_per_second, Terrain *terrain);

	// calculate the timing component w of control points in a xmfloat4 vector.
	// Path takes totalTime seconds at constant speed to move from start to end pos
	void adjustTimings(vector<XMFLOAT4>& p, float totalTime);

	// define movement path on-the-fly instead from blender/.b file
	// ctrlPoints define the control points in world coords. w holds the frame number/fraction (always 25 fps assumed)
	// rot defines rotation for each control point, may be left out for no rotation
	void defineAction(char *name, WorldObject &wo, vector<XMFLOAT4> &ctrlPoints, vector<XMFLOAT3> *rot = nullptr);
private:
	XMMATRIX getInterpolationMatrix(int i, PathDesc *pd);
	void saveInterpolationMatrix(int i, PathDesc *pd, XMMATRIX *m);
	XMMATRIX getInterpolationMatrixChained(int i, PathDesc *pd);
	void saveInterpolationMatrixChained(int i, PathDesc *pd, XMMATRIX *m);
	bool isSlopeForPathOK(float maxSlope, XMVECTOR *start, XMVECTOR *normalizedToVector, float pathLen, Terrain *terrain);
	void calculateInterpolationChain(const AnimationClip *clip, PathDesc *pd);
	unordered_map<string, PathDesc> npcPaths;
};

