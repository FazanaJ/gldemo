#pragma once

#include "../include/global.h"
#include "object.h"

typedef struct PlayerData {
	struct Object *zTarget;
	struct Object *heldObj;
	struct Object *riddenObj;
	float climbPos[3];
	float intendedMag;
	float forwardVel;
	float velOffset;
	short input;
	short intendedYaw;
	short offsetYaw;
	short health;
	short healthBase;
	short healthMax;
	short cameraAngle;
	char action;
	char actionFlags;
	char playerID;
	char attackCombo;
	char weaponOut;


	unsigned char walkTimer;
} PlayerData;

typedef struct ProjectileData {
	short life;
} ProjectileData;