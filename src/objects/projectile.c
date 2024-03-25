#include <libdragon.h>

#include "../../include/global.h"

#include "../object.h"
#include "../object_data.h"
#include "../math_util.h"

void init(Object *obj) {
    ProjectileData *data = obj->data;

    data->life = timer_int(60);
}

void loop(Object *obj, int updateRate, float updateRateF) {
    ProjectileData *data = obj->data;

    data->life --;

    if (data->life <= 0) {
        delete_object(obj);
    }
}

ObjectEntry entry = {
    .initFunc = init,
    .loopFunc = loop,
    .name = "Projectile",
    .data = sizeof(ProjectileData),
    .flags = OBJ_FLAG_MOVE,
    .viewDist = OBJ_DIST(100),
    .viewWidth = 1,
    .viewHeight = 1,
};