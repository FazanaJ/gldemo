#pragma once

#include "../include/global.h"
#include "object.h"

typedef struct CollisionData {
    float pos[3][3];
    float normal[3];
    float upperY;
    float lowerY;
    unsigned short flags;
    unsigned short pad;
} CollisionData;

typedef struct CollisionCell {
    CollisionData *data;
    unsigned short triangleCount;
    unsigned short cellCount;
} CollisionCell;

void object_collide(struct Object *obj);
float collision_floor(float x, float y, float z, float *norm, int w);
float collision_floor_hitbox(struct Object *obj, float x, float y, float z);