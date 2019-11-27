#pragma once

/*
* Left handed camera system (positive Z goes away from you - thumb / index are x / y)
*/
class Camera
{
public:
//	Camera(World& w);
	Camera();
	~Camera();
	void init();
	void lookAt(XMFLOAT4 pos, XMFLOAT4 target, XMFLOAT4 up);
	float getSpeed();
	void setSpeed(float speed);
	void walk(double dt);
	void strafe(double dt);

	XMFLOAT4 pos;  // defaults to (0,0,-2,0)
	XMFLOAT4 look; // defaults to origin
	XMFLOAT4 up;   // defaults to y-axis (0,1,0,0)
	XMFLOAT4 right;// defalts to x-axis (1,0,0,0)
	XMFLOAT4 look_straight; // without yaw/pitch effect
	XMFLOAT4 up_straight;   // without yaw/pitch effect
	XMFLOAT4 right_straight;// without yaw/pitch effect
	// view: tranform from world space to view/camera space
	XMFLOAT4X4 view;  // defaults to identity matrix
	void viewTransform(); // use current camera and recalc view matrix
	// apply pitch/yaw by specifying absolute pitch value in radians relative to original look vector
	void apply_pitch_yaw();
	void apply_pitch(float pitch_absolute);
	void apply_yaw(float yaw_absolute);

	// frustum
	float fieldOfViewAngleY;  // defaults to PI/4
	float aspectRatio;        // defaults to FullHD ratio (1.77777)
	float nearZ;              // defaults to 0.1
	float farZ;               // defaults to 1.1

	// orientation
	float pitch;	// radians 0 .. 2 Pi
	float yaw;		// radians 0 .. 2 Pi

	// oculus support
	bool ovrCamera = true;
	//XMFLOAT4X4 viewOVR[2], projOVR[2];
	int activeEye = 0;
#if defined(_OVR_)
	void recalcOVR(XApp &xapp);
#endif
	int eyeNum;
	bool eyeNumUse = false;


	// projection to 2d
	// use current frustum and recalc projection matrix
	void projectionTransform();
	XMFLOAT4X4 projection;    // defaults to default frustum projection

	// world view projection transform - usually for passing to shaders
	// use frustum and camera to recalc wvp matrix - no need to update view first, but projection is expected to be up-to-date
	XMMATRIX worldViewProjection();  
	XMMATRIX worldViewProjection(XMFLOAT4X4 & proj_ovr, XMFLOAT4X4 & view_ovr);
	// visibility:
	int calculateVisibility(BoundingBox &box, XMMATRIX &toWorld);

	// copy constructor
	Camera& operator=(const Camera& other) {
		activeEye = other.activeEye;
		aspectRatio = other.aspectRatio;
		eyeNum = other.eyeNum;
		eyeNumUse = other.eyeNumUse;
		farZ = other.farZ;
		fieldOfViewAngleY = other.fieldOfViewAngleY;
		look = other.look;
		look_straight = other.look_straight;
		nearZ = other.nearZ;
		ovrCamera = other.ovrCamera;
		pitch = other.pitch;
		pos = other.pos;
		projection = other.projection;
		right = other.right;
		right_straight = other.right_straight;
		speed = other.speed;
		up = other.up;
		up_straight = other.up_straight;
		view = other.view;
		//world = other.world;
		yaw = other.yaw;
		return *this;
	}

private:
	//World& world;
	float speed;
};

