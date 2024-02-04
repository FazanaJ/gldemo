#include <libdragon.h>

#include "../../include/global.h"

#include "../object.h"
#include "../object_data.h"
#include "../math_util.h"

void init(Object *obj) {
}

void loop(Object *obj, int updateRate, float updateRateF) {
}

Hitbox bbox = {
    .type = HITBOX_CYLINDER,
    .offsetY = 0,
    .width = 4.0f,
    .length = 4.0f,
    .weight = 50.0f,
    .height = 8.0f,
};

ObjectEntry entry = {
    .initFunc = init,
    .loopFunc = loop,
    .name = "Crate",
    .flags = OBJ_FLAG_MOVE | OBJ_FLAG_TANGIBLE,
    .viewDist = OBJ_DIST(100),
    .viewWidth = 4,
    .viewHeight = 8,
    .hitbox = &bbox,
};