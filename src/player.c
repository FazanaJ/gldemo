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
    data->health = data->healthBase;
    data->cameraAngle = 0;
    obj->weight = 1.0f;
}

void player_loop(Object *obj, int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    PlayerData *data = (PlayerData *) obj->data;
    Camera *c = gCamera;
	float stickX = get_stick_x(STICK_LEFT);
    float intendedMag = get_stick_mag(STICK_LEFT);
    int moveTicks = timer_int(60);
    if (gCurrentController == -1) {
        get_time_snapshot(PP_PLAYER, DEBUG_SNAPSHOT_1_END);
        return;
    }

    if (get_input_pressed(INPUT_DLEFT, 0)) {
        data->health--;
    }
    if (get_input_pressed(INPUT_DRIGHT, 0)) {
        data->health++;
    }

    if (get_input_pressed(INPUT_DDOWN, 0)) {
        data->healthMax--;
        if (data->health > data->healthMax) {
            data->health = data->healthMax;
        }
    }
    if (get_input_pressed(INPUT_DUP, 0)) {
        data->healthMax++;
    }

    Object *targetObj = find_nearest_object_facing(obj, OBJ_NPC, 30.0f, 0x3000, obj->faceAngle[2]);
    if (targetObj) {
        gCamera->target = targetObj;
    } else {
        gCamera->target = NULL;
    }

    if (targetObj && get_input_pressed(INPUT_Z, 3)) {
        clear_input(INPUT_Z);
        data->zTarget = targetObj;
    } else if (get_input_released(INPUT_Z, 0)) {
        data->zTarget = NULL;
    }

    if (get_input_pressed(INPUT_B, 0) && gGameTimer > 120) {
        play_sound_spatial(SOUND_LASER, obj->pos);
        Object *bullet = spawn_object_pos(OBJ_PROJECTILE, obj->pos[0], obj->pos[1], obj->pos[2]);
        bullet->forwardVel = 20.0f;
        bullet->moveAngle[2] = obj->faceAngle[2];
        rumble_set(3);
    }

    if (get_input_pressed(INPUT_A, 0) && gGameTimer > 120) {
        if (get_input_held(INPUT_Z)) {
            if (obj->yVel == 0.0f) {
                obj->yVel = 10.0f;
            }
        } else {
            Particle *part;
            play_sound_spatial_pitch(SOUND_CANNON, obj->pos, 1.0f);
            for (int i = 0; i < 7; i++) {
                part = spawn_particle(OBJ_NULL, obj->pos[0], obj->pos[1], obj->pos[2] + 5.0f);
                part->zVel = 0.0f;
                part->zVelIncrease = 0.01f;
                part->moveAngle = random_float() * 0x10000;
                part->forwardVel = 0.5f;
                part->forwardVelIncrease = -0.025f;
                part->timer = 120;
                part->scale[0] = 0.2f;
                part->scale[1] = 0.2f;
                part->scale[2] = 0.2f;
                part->scaleIncrease[0] = 0.005f;
                part->scaleIncrease[1] = 0.005f;
                part->scaleIncrease[2] = 0.005f;
            }
        }
    }

    if (intendedMag > 0.01f && get_input_held(INPUT_L) == false) {
        if (intendedMag < 0.25f) {
            intendedMag = 0.25f;
        }
        data->walkTimer += updateRateF * (intendedMag * 12.0f);
        if (data->walkTimer >= 200) {
            data->walkTimer = 0;
            float pitchBend = random_float() / 16.0f;
            play_sound_spatial_pitch(SOUND_STEP_STONE, obj->pos, 0.25f + pitchBend);
        }
        short intendedYaw = get_stick_angle(STICK_LEFT);
        float moveLerp;
        INCREASE_VAR(c->moveTimer, updateRate, moveTicks);
        moveLerp = 1.0f - (((float) (moveTicks - c->moveTimer)) / (float) moveTicks);
        obj->moveAngle[2] = lerp_short(obj->moveAngle[2], intendedYaw + c->yawTarget, 0.25f * updateRateF);
        if (gZTargetTimer == 0) {
            c->yawTarget -= (float) (stickX * ((2.0f * updateRateF) * moveLerp));
            obj->faceAngle[2] = lerp_short(obj->faceAngle[2], obj->moveAngle[2], 0.1f * updateRateF);
            data->cameraAngle = obj->faceAngle[2];
        } else {
            if (data->zTarget) {
                short intendedYaw = atan2s(obj->pos[0] - data->zTarget->pos[0], obj->pos[1] - data->zTarget->pos[1]) - 0x4000;
                obj->faceAngle[2] = lerp_short(obj->faceAngle[2], intendedYaw, 0.1f * updateRateF);
            }
        }
    } else {
        DECREASE_VAR(c->moveTimer, updateRate * 2, 0);
        intendedMag = 0.0f;
    }

    if (obj->forwardVel < 8.0f * intendedMag) {
        obj->forwardVel += (updateRateF * 1.0f) * intendedMag;
    }
    if (obj->forwardVel > 8.0f * intendedMag) {
        DECREASE_VAR(obj->forwardVel, updateRateF * 0.5f, 0);
    }

    if (obj->forwardVel > 0.0f) {
        DECREASE_VAR(obj->forwardVel, updateRateF * 0.25f, 0);
    }

    get_time_snapshot(PP_PLAYER, DEBUG_SNAPSHOT_1_END);
}