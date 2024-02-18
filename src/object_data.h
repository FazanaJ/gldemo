#pragma once

#include "../include/global.h"
#include "object.h"

typedef struct PlayerData {
	short health;
	short healthBase;
	short healthMax;
	short cameraAngle;
	struct Object *zTarget;
	struct Object *heldObj;
	struct Object *riddenObj;
	char action;


	unsigned char walkTimer;
} PlayerData;

typedef struct ProjectileData {
	short life;
} ProjectileData;