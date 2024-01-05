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
            //voice_play(VOICE_NECROMANCY, true);
            clear_input(INPUT_A);
            talk_open(0);
        }
    }
}

ObjectEntry entry = {
    NULL,
    loop,
    0,
    OBJ_FLAG_SHADOW,
    OBJ_DIST(200),
    3,
    5
};