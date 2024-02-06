#include <libdragon.h>

#include "../../include/global.h"

#include "../object.h"
#include "../object_data.h"
#include "../math_util.h"
#include "../audio.h"

int tempTimer = 0;

void init(Object *obj) {
    tempTimer = 0;
}

void loop(Object *obj, int updateRate, float updateRateF) {
    tempTimer += updateRate;
    if (tempTimer < 120) {
        obj->pos[0] += 0.05f * updateRateF;
    } else if (tempTimer < 240) {
        obj->pos[1] += 0.05f * updateRateF;
    } else if (tempTimer < 360) {
        obj->pos[1] -= 0.05f * updateRateF;
    } else {
        obj->pos[0] -= 0.05f * updateRateF;
        if (tempTimer >= 480) {
            tempTimer -= 480;
        }
    }
}

Hitbox bbox = {
    .type = HITBOX_BLOCK,
    .solid = true,
    .offsetY = 0,
    .width = 4.0f,
    .length = 4.0f,
    .weight = 2000.0f,
    .height = 8.0f,
    .moveSound = SOUND_SHELL1,
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