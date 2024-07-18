#include <libdragon.h>

#include "../../include/global.h"

#include "../object.h"
#include "../object_data.h"
#include "../math_util.h"
#include "../input.h"
#include "../talk.h"
#include "../audio.h"


void loop(Object *obj, int updateRate, float updateRateF) {
    obj->animation->id[0] = 0;
    obj->animation->speed[0] = 0.02f;
}

Hitbox bbox = {
    .type = HITBOX_CYLINDER,
    .offsetY = 0,
    .width = 24.0f,
    .length = 24.0f,
    .weight = 90.0f,
    .height = 80.0f,
};

ObjectEntry entry = {
    .loopFunc = loop,
    .data = 0,
    .name = "NPC",
    .flags = OBJ_FLAG_SHADOW | OBJ_FLAG_TANGIBLE | OBJ_FLAG_COLLISION,
    .viewDist = OBJ_DIST(1600),
    .viewWidth = 3,
    .viewHeight = 14,
    .hitbox = &bbox,
};