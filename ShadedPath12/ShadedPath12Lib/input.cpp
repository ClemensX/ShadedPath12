#include "stdafx.h"

void Input::updateKeyboardState()
{
	BOOL result = GetKeyboardState(key_state);
	if (result == 0) {
		Log("ERROR GetKeyboardState");
		return;
	}
	for (BYTE b = 1; b < 255; b++) {
		if (keyDown(b)) {
			//Log("key down");
			switch (b) {
				// prevent toggle keys from triggering keydown state
			case VK_CAPITAL:
			case VK_NUMLOCK:
			case VK_SCROLL:
				// nothing to do
				break;
			default: anyKeyDown = true;
			}
		}
	}
	// handle keyboard input
	if (keyDown('W') || keyDown(VK_UP))
		kticks.tickUp++;
	if (keyDown('S') || keyDown(VK_DOWN))
		kticks.tickDown++;
	if (keyDown('D') || keyDown(VK_RIGHT))
		kticks.tickRight++;
	if (keyDown('A') || keyDown(VK_LEFT))
		kticks.tickLeft++;


}

bool Input::keyDown(BYTE key) {
	return (key_state[key] & 0x80) != 0;
}

void Input::getAndClearKeyTicks(KeyTicks& keyTicks)
{
	keyTicks = kticks;
	kticks.clear();
}

void Input::applyTicksToCameraPosition(KeyTicks& k, Camera* c, float dt)
{
	if (k.tickUp > 0) {
		c->walk(dt * k.tickUp);
	}
	if (k.tickDown > 0) {
		c->walk(-dt * k.tickDown);
	}
	if (k.tickLeft > 0) {
		c->strafe(-dt * k.tickLeft);
	}
	if (k.tickRight > 0) {
		c->strafe(dt * k.tickRight);
	}
}

void Input::applyMouseEvents(Camera* camera, float dt)
{
	if (mouseTodo /*&& !ovrRendering*/) {
		mouseTodo = false;
		// mouse input
		float ROTATION_GAIN = dt;
		float pitch = camera->pitch;
		float yaw = camera->yaw;
		XMFLOAT2 rotationDelta;
		rotationDelta.x = mouseDx * ROTATION_GAIN;   // scale for control sensitivity
		rotationDelta.y = mouseDy * ROTATION_GAIN;
		//Log(callnum++ << "mouse dx dy == " << mouseDx << " " << mouseDy);
		//Log(" delta x y == " << rotationDelta.x << " " << rotationDelta.y << "\n");

		// Update our orientation based on the command.
		pitch -= rotationDelta.y;
		yaw += rotationDelta.x;
		//Log("pich " << pitch);

		// Limit pitch to straight up or straight down.
		float limit = XM_PI / 2.0f - 0.01f;
		pitch = __max(-limit, pitch);
		pitch = __min(+limit, pitch);

		//Log(" " << pitch << endl);
		// Keep longitude in same range by wrapping.
		if (yaw > XM_PI)
		{
			yaw -= XM_PI * 2.0f;
		}
		else if (yaw < -XM_PI)
		{
			yaw += XM_PI * 2.0f;
		}
		camera->pitch = pitch;
		camera->yaw = yaw;
		camera->apply_pitch_yaw();
		//camera.apply_yaw(camera.yaw);
	}
}
