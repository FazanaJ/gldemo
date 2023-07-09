#pragma once

#include "object.h"

typedef struct Camera {
	Object *parent;
	Object *target;
	float pos[3];
	float focus[3];
	float pan;
	float zoom;
	float intendedZoom;
	float targetZoom;
	float zoomAdd;
	short pitch;
	short lookPitch;
	short roll;
	short yaw;
	short yawTarget;
	short tilt;
	short flags;
	char mode;
	char moveTimer;
} Camera;

extern Camera *gCamera;

void camera_init(void);
void camera_loop(int updateRate, float updateRateF);