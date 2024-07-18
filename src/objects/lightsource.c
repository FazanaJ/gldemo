#include <libdragon.h>

#include "../../include/global.h"

#include "../object.h"
#include "../object_data.h"
#include "../math_util.h"
#include "../audio.h"

int tempTimer = 0;

void init(Object *obj) {
    obj->scale[0] = 3.0f;
    obj->scale[1] = 3.0f;
    obj->scale[2] = 3.0f;
    tempTimer = 0;
}

void loop(Object *obj, int updateRate, float updateRateF) {
}

ObjectEntry entry = {
    .initFunc = init,
    .loopFunc = loop,
    .name = "Light",
    .flags = OBJ_FLAG_MOVE | OBJ_FLAG_TANGIBLE,
    .viewDist = OBJ_DIST(2000),
    .viewWidth = 4,
    .viewHeight = 8,
    .hitbox = NULL,
};