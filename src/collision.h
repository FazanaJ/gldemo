#pragma once

#include "../include/global.h"
#include "object.h"

void object_collide(Object *obj);
float collision_floor(float x, float y, float z, float *norm, int w);