#pragma once

/*
* Left handed camera system
*/
class Camera
{
public:
	Camera(World& w);
	~Camera();
	void lookAt(XMFLOAT4 pos, XMFLOAT4 target, XMFLOAT4 up);
	World& getWorld();
	void setWorld(World& w);
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
	void recalcOVR(XApp &xapp);

	// projection to 2d
	void projectionTransform();  // use current frustum and recalc projection matrix
	XMFLOAT4X4 projection;    // defaults to default frustum projection

	// world view projection transform - usually for passing to shaders
	XMMATRIX worldViewProjection();  // use frustum and camera to recalc wvp matrix - no need to update view first, but projection is expected to be up-to-date

	// visibility:
	int calculateVisibility(BoundingBox &box, XMMATRIX &toWorld);
	Camera& Camera::operator=(const Camera& other) {
		pos = other.pos;
		look = other.look;
		up = other.up;
		right = other.right;
		look_straight = other.look_straight;
		up_straight = other.up_straight;
		right_straight = other.right_straight;
		view = other.view;
		fieldOfViewAngleY = other.fieldOfViewAngleY;
		aspectRatio = other.aspectRatio;
		nearZ = other.nearZ;
		farZ = other.farZ;
		pitch = other.pitch;
		yaw = other.yaw;
		projection = other.projection;
		speed = other.speed;
		return *this;
	}
private:
	World& world;
	float speed;
};

