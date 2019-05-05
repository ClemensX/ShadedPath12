
struct BezTriple
{
	//float vec[3][3];
	float h1[2], cp[2], h2[2];
	//float poseMatrix[16];
	//XMMATRIX transformationMatrix;
	float transMatrix[16];
	bool isBoneAnimation;
};

struct Curve
{
public:
	Curve(void) {};
	~Curve(void) {};
	const char *name;
	// rna_path is of this form:
	// for bone anim: pose.bones["Bone.003"].rotation_quaternion (or other ending, see below)
	// for object: rotation_quaternion, rotation_euler, location, scale
	const char *rna_path;
	//int totvert;						// number of beztriples  --> not needed, use vector.size
	vector<BezTriple> bezTriples;  // vector of BezTriples belonging to this curve
};

struct Action
{
public:
	Action(void) {};
	~Action(void) {};
	//const char *name;
	string name;
	vector<Curve> curves;  // vector of curves for this action
};

