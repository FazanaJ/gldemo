#include <libdragon.h>
#include <malloc.h>

#include "player.h"
#include "../include/global.h"

#include "math_util.h"
#include "object.h"
#include "camera.h"
#include "main.h"
#include "input.h"
#include "audio.h"
#include "debug.h"

Object *gPlayer;

void player_init(Object *obj) {
    PlayerData *data = (PlayerData *) obj->data;

    data->healthBase = 12;
    data->healthMax = data->healthBase;
    data->health = data->healthMax;
}

void player_loop(Object *obj, int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    Camera *c = gCamera;
	float stickX = get_stick_x();
    float intendedMag = get_stick_mag();
    int moveTicks = timer_int(60);
    if (gCurrentController == -1) {
        get_time_snapshot(PP_PLAYER, DEBUG_SNAPSHOT_1_END);
        return;
    }

    Object *targetObj = find_nearest_object(obj, OBJ_NPC, 3.0f);
    if (targetObj) {
        gCamera->target = targetObj;
    } else {
        gCamera->target = NULL;
    }

    if (get_input_pressed(INPUT_B, 0) && gGameTimer > 120) {
        play_sound_spatial(SOUND_LASER, obj->pos);
        Object *bullet = spawn_object_pos(OBJ_PROJECTILE, obj->pos[0], obj->pos[1], obj->pos[2]);
        bullet->forwardVel = 20.0f;
        bullet->moveAngle[2] = obj->faceAngle[2];
        rumble_set(3);
    }

    if (get_input_pressed(INPUT_A, 0) && gGameTimer > 120) {
        play_sound_spatial(SOUND_CANNON, obj->pos);
    }

    if (intendedMag > 0.01f && get_input_held(INPUT_L) == false) {
        short intendedYaw = get_stick_angle();
        float moveLerp;
        INCREASE_VAR(c->moveTimer, updateRate, moveTicks);
        moveLerp = 1.0f - (((float) (moveTicks - c->moveTimer)) / (float) moveTicks);
        obj->moveAngle[2] = lerp_short(obj->moveAngle[2], intendedYaw + c->yawTarget, 0.25f * updateRateF);
        if (gZTargetTimer == 0) {
            c->yawTarget -= (float) (stickX * ((2.0f * updateRateF) * moveLerp));
            obj->faceAngle[2] = lerp_short(obj->faceAngle[2], obj->moveAngle[2], 0.1f * updateRateF);
        }
    } else {
        DECREASE_VAR(c->moveTimer, updateRate * 2, 0);
        intendedMag = 0.0f;
    }

    if (obj->forwardVel < 10.0f * intendedMag) {
        obj->forwardVel += (updateRateF * 3.0f) * intendedMag;
    }
    if (obj->forwardVel > 10.0f * intendedMag) {
        DECREASE_VAR(obj->forwardVel, updateRate * 0.5f, 0);
    }

    if (obj->forwardVel > 0.0f) {
        DECREASE_VAR(obj->forwardVel, updateRate * 0.75f, 0);
    }

    if (obj->forwardVel != 0.0f) {
        obj->pos[0] += (obj->forwardVel * sins(obj->moveAngle[2])) / 100.0f;
        obj->pos[1] -= (obj->forwardVel * coss(obj->moveAngle[2])) / 100.0f;
    }

    get_time_snapshot(PP_PLAYER, DEBUG_SNAPSHOT_1_END);
}