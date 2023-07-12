#pragma once

#include "object.h"

typedef struct PlayerData {
	short health;
	short healthBase;
	short healthMax;
	short cameraAngle;
	struct Object *zTarget;
	unsigned char walkTimer;
} PlayerData;

typedef struct ProjectileData {
	short life;
} ProjectileData;