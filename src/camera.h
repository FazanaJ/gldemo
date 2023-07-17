#pragma once

#include "object.h"

typedef struct Camera {
	Object *parent;
	Object *target;
	float pos[3];
	float focus[3];
	float lookFocus[3];
	short pan;
	short zoom;
	short intendedZoom;
	short targetZoom;
	short zoomAdd;
	short pitch;
	short viewPitch;
	short lookPitch;
	short roll;
	short yaw;
	short yawTarget;
	short tilt;
	short flags;
	char mode;
	char moveTimer;
} Camera;

#define CAMERA_CUTSCENE 0
#define CAMERA_TARGET 1

extern Camera *gCamera;

void camera_init(void);
void camera_loop(int updateRate, float updateRateF);