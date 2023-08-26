#include <libdragon.h>

#include "../../include/global.h"

#include "../object.h"
#include "../object_data.h"

void init(Object *obj) {

}

void loop(Object *obj, int updateRate, float updateRateF) {

}

ObjectEntry entry = {
    init,
    loop,
    sizeof(ProjectileData),
    OBJ_FLAG_MOVE,
    OBJ_DIST(100),
    1,
    1
};