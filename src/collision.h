#pragma once

#include "../include/global.h"
#include "object.h"

typedef struct CollisionData {
    short pos[3][3];
    short upperY;
    short lowerY;
    short normalY;
} CollisionData;

typedef struct CollisionCell {
    unsigned short triangleCount;
    unsigned short cellCount;
    CollisionData *data;
} CollisionCell;

void object_collide(struct Object *obj);
float collision_floor(float x, float y, float z, float *norm, int w);
void collision_normals(int16_t *v0, int16_t *v1, int16_t *v2, float *normals, int w);
float collision_floor_hitbox(struct Object *obj, float x, float y, float z);
T3DVertPacked *t3d_model_verts_obj(const T3DModel *model, const T3DObject *obj, T3DObjectPart *part);