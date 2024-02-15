#include <libdragon.h>

#include "../../include/global.h"

#include "../object.h"
#include "../object_data.h"
#include "../math_util.h"
#include "../input.h"
#include "../talk.h"
#include "../audio.h"


void loop(Object *obj, int updateRate, float updateRateF) {
    if (input_pressed(INPUT_A, 0)) {
        if (DIST3(obj->pos, gPlayer->pos) < 10.0f * 10.0f) {
            input_clear(INPUT_A);
            talk_open(0);
            //voice_play(VOICE_NECROMANCY, true);
        }
    }
}

Hitbox bbox = {
    .type = HITBOX_CYLINDER,
    .offsetY = 0,
    .width = 3.0f,
    .length = 3.0f,
    .weight = 90.0f,
    .height = 10.0f,
};

ObjectEntry entry = {
    .loopFunc = loop,
    .data = 0,
    .name = "NPC",
    .flags = OBJ_FLAG_SHADOW | OBJ_FLAG_TANGIBLE,
    .viewDist = OBJ_DIST(200),
    .viewWidth = 3,
    .viewHeight = 14,
    .hitbox = &bbox,
};