#include <libdragon.h>

#include "../../include/global.h"

#include "../object.h"
#include "../object_data.h"
#include "../math_util.h"
#include "../input.h"
#include "../talk.h"
#include "../audio.h"


void loop(Object *obj, int updateRate, float updateRateF) {
    if (get_input_pressed(INPUT_A, 0)) {
        if (DIST3(obj->pos, gPlayer->pos) < 10.0f * 10.0f) {
            clear_input(INPUT_A);
            talk_open(0);
            //voice_play(VOICE_NECROMANCY, true);
        }
    }
}

ObjectEntry entry = {
    .loopFunc = loop,
    .data = 0,
    .flags = OBJ_FLAG_SHADOW,
    .viewDist = OBJ_DIST(200),
    .viewWidth = 3,
    .viewHeight = 5,
};