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


	short walkTimer;
} PlayerData;

typedef struct ProjectileData {
	short life;
} ProjectileData;