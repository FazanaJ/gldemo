#include <libdragon.h>
#include <t3d/t3danim.h>
#include <t3d/t3dskeleton.h>

#include "../../include/global.h"

#include "../object.h"
#include "../object_data.h"
#include "../math_util.h"
#include "../camera.h"
#include "../audio.h"
#include "../input.h"
#include "../debug.h"
#include "../render.h"
#include "../collision.h"
#include "../main.h"
#include "../talk.h"

#define ANIM_IDLE 0
#define ANIM_RUN 1
#define ANIM_WALK 2

enum PlayerAction {
    PLAYER_ACT_IDLE,
    PLAYER_ACT_MOVE,
    PLAYER_ACT_AIR,
    PLAYER_ACT_ACTION,
    PLAYER_ACT_ACTION_MOVE,
    PLAYER_ACT_SWIM,
    PLAYER_ACT_LEDGE,
    PLAYER_ACT_BRUH,
};

enum PlayerInput {
    PLAYER_INPUT_NONE,
    PLAYER_INPUT_A_PRESSED = (1 << 0),
    PLAYER_INPUT_A_HELD = (1 << 1),
    PLAYER_INPUT_B_PRESSED = (1 << 2),
    PLAYER_INPUT_B_HELD = (1 << 3),
    PLAYER_INPUT_R_PRESSED = (1 << 4),
    PLAYER_INPUT_R_HELD = (1 << 5),
    PLAYER_INPUT_Z_PRESSED = (1 << 6),
    PLAYER_INPUT_Z_HELD = (1 << 7),
    PLAYER_INPUT_L_HELD = (1 << 8),
};

static void player_anim_idle(Object *o, PlayerData *d) {
    ObjectAnimation *a = o->animation;
    float vel = MIN(d->intendedMag, 1.0f) * 24.0f;
    int stepSound = false;
    if (vel > 1.0f) {
        float spd;
        float clamp2 = MIN(vel, 16.0f);
        spd = 0.02f + (clamp2 / 300.0f);
        if (vel > 8.0f) {
            a->id[0] = ANIM_WALK;
            a->id[1] = ANIM_RUN;
            float clamp = MIN(vel - 8.0f, 16.0f);
            a->animBlend = (clamp) / 16.0f;
            if (a->framePrev[0] >= 0.0f && a->stage[0] == 0) {
                a->stage[0] = 1;
                stepSound = true;
            } else if (a->framePrev[0] >= 1.0f && a->stage[0] == 1) {
                a->stage[0] = 2;
                stepSound = true;
            }
        } else {
            a->id[0] = ANIM_IDLE;
            a->id[1] = ANIM_WALK;
            float clamp = MIN(vel, 8.0f);
            a->animBlend = (clamp) / 8.0f;
            if (a->framePrev[1] >= 0.0f && a->stage[1] == 0) {
                a->stage[1] = 1;
                stepSound = true;
            } else if (a->framePrev[1] >= 1.0f && a->stage[1] == 1) {
                a->stage[1] = 2;
                stepSound = true;
            }
        }
        a->speed[0] = spd;
        a->speed[1] = spd;
    } else {
        a->id[0] = ANIM_IDLE;
        a->id[1] = ANIM_NONE;
        a->animBlend = 0.0f;
        a->speed[0] = 0.04f;
    }
    
    if (stepSound) {
        object_footsteps(COL_GET_SOUND(o->collision->floorFlags), o->pos);
    }
}

tstatic void player_forwardvel(Object *o, PlayerData *d, float vel, float updateRateF) {
    short angle = d->intendedYaw + d->offsetYaw;
    float grip;
    if (o->collision->grounded) {
        grip = (float) (COL_GET_GRIP(o->collision->floorFlags));
        grip /= 8.0f;
    } else {
        grip = 0.25f;
    }
    vel *= grip;
    o->movement->vel[0] += (vel * sins(angle)) * updateRateF;
    o->movement->vel[2] += (vel * coss(angle)) * updateRateF;
}

tstatic void player_attack(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    if (d->input & PLAYER_INPUT_B_PRESSED) {
        if (d->weaponOut == false) {
            d->weaponOut = true;
        }
        if (d->playerID != -1) {
            input_clear(INPUT_B);
        }
        d->input &= ~PLAYER_INPUT_B_PRESSED;
        player_forwardvel(o, d, 32.0f, 1.0f);
    }
}

tstatic void player_grounded_common(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    if (d->input & PLAYER_INPUT_A_PRESSED) {
        if (d->playerID != -1) {
            input_clear(INPUT_A);
        }
        d->input &= ~PLAYER_INPUT_A_PRESSED;
        o->movement->vel[1] = 12.0f;
        o->collision->grounded = false;
    } else {
        player_attack(o, d, 32.0f, 1.0f);
    }
    player_anim_idle(o, d);
}

tstatic void player_act_idle(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    o->movement->vel[1] = 0.0f;
    if (o->movement->vel[0] != 0.0f || o->movement->vel[2] != 0.0f) {
        o->movement->moveAngle[1] = atan2s(o->movement->vel[2], o->movement->vel[0]);
    }
    if ((d->input & PLAYER_INPUT_L_HELD) == false) {
        player_grounded_common(o, d, updateRate, updateRateF);
    }
}

tstatic void player_act_move(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    float factor;
    player_forwardvel(o, d, 3.0f * d->intendedMag, updateRateF);
    if (d->forwardVel == 0.0f) {
        factor = 0.75f;
    } else {
        factor = 0.25f;
    }
    o->movement->moveAngle[1] = atan2s(o->movement->vel[2], o->movement->vel[0]);
    if (gZTargetTimer == 0) {
        if (d->forwardVel != 0.0f) {
            factor = 0.1f * updateRateF;
        }
        short intendedAngle = d->intendedYaw + d->offsetYaw;
        o->faceAngle[1] = lerp_short(o->faceAngle[1], intendedAngle, factor * updateRateF);
        d->cameraAngle = o->faceAngle[1];
    }
    player_grounded_common(o, d, updateRate, updateRateF);
}

tstatic void player_act_air(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    player_forwardvel(o, d, 1.0f * d->intendedMag, updateRateF);
    o->movement->moveAngle[1] = atan2s(o->movement->vel[2], o->movement->vel[0]);
    if (gZTargetTimer == 0) {
        d->cameraAngle = o->faceAngle[1];
    }

    if (o->movement->vel[1] < 0.0f) {
        int grabbed = false;
        float offset = o->hitbox->width + 3.0f;
        float posX = o->pos[0] + (offset * sins(o->faceAngle[1]));
        float posZ = o->pos[2] + (offset * coss(o->faceAngle[1]));
        float height = collision_floor(posX, o->pos[1] + o->hitbox->height + 2.0f, posZ, NULL, false);
        float heightDiff = o->pos[1] - height;
        if (heightDiff < -15.0f && heightDiff > -25.0f) {
            grabbed = true;
        } else {
            height = collision_floor_hitbox(o, posX, o->pos[1] + o->hitbox->height + 2.0f, posZ);
            float heightDiff = o->pos[1] - height;
            if (heightDiff < -15.0f && heightDiff > -25.0f) {
                grabbed = true;
            }
        }

        if (grabbed) {
            d->action = PLAYER_ACT_LEDGE;
            d->climbPos[0] = posX;
            d->climbPos[1] = height;
            d->climbPos[2] = posZ;
            o->movement->vel[0] = 0.0f;
            o->movement->vel[1] = 0.0f;
            o->movement->vel[2] = 0.0f;
            d->weaponOut = false;
        }
    }
}

tstatic void player_act_action(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    o->movement->vel[0] = approachf(o->movement->vel[0], 0.0f, 2.0f * updateRateF);
    o->movement->vel[2] = approachf(o->movement->vel[2], 0.0f, 2.0f * updateRateF);
}

tstatic void player_act_action_move(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    
}

tstatic void player_act_swim(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    
}

tstatic void player_act_ledge(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    o->flags &= ~OBJ_FLAG_GRAVITY;
    if (d->input & PLAYER_INPUT_A_PRESSED) {
        if (d->playerID != -1) {
            input_clear(INPUT_A);
        }
        o->pos[0] = d->climbPos[0];
        o->pos[1] = d->climbPos[1];
        o->pos[2] = d->climbPos[2];
        d->action = PLAYER_ACT_IDLE;
        o->flags |= OBJ_FLAG_GRAVITY;
    }
}

tstatic void (*sPlayerFunctions[])(Object *, PlayerData *, int, float) = {
    player_act_idle,
    player_act_move,
    player_act_air,
    player_act_action,
    player_act_action_move,
    player_act_swim,
    player_act_ledge,
    NULL,
};

void init(Object *obj) {
    PlayerData *data = (PlayerData *) obj->data;

    data->healthBase = 12;
    data->healthMax = data->healthBase;
    data->health = data->healthBase;
    data->cameraAngle = 0;
    data->playerID = 0;
    obj->movement->weight = 5.0f;

    obj->animation->id[0] = ANIM_IDLE;
    obj->animation->speed[0] = 0.04f;
}

tstatic void player_input(Object *o, PlayerData *d) {
    if (gCurrentController == -1 && d->playerID != -1) {
        return;
    }
    d->offsetYaw = gCamera->yawTarget;
    d->intendedMag = input_stick_mag(STICK_LEFT);
    if (d->intendedMag != 0.0f) {
        d->intendedYaw = input_stick_angle(STICK_LEFT);
    }
    if (input_pressed(INPUT_A, 5)) {
        d->input |= PLAYER_INPUT_A_PRESSED;
        d->input |= PLAYER_INPUT_A_HELD;
    } else if (input_held(INPUT_A)) {
        d->input |= PLAYER_INPUT_A_HELD;
    }
    if (input_pressed(INPUT_B, 5)) {
        d->input |= PLAYER_INPUT_B_PRESSED;
        d->input |= PLAYER_INPUT_B_HELD;
    } else if (input_held(INPUT_B)) {
        d->input |= PLAYER_INPUT_B_HELD;
    }
    if (input_pressed(INPUT_R, 5)) {
        d->input |= PLAYER_INPUT_R_PRESSED;
        d->input |= PLAYER_INPUT_R_HELD;
    } else if (input_held(INPUT_R)) {
        d->input |= PLAYER_INPUT_R_HELD;
    }
    if (input_pressed(INPUT_Z, 5)) {
        d->input |= PLAYER_INPUT_Z_PRESSED;
        d->input |= PLAYER_INPUT_Z_HELD;
    } else if (input_held(INPUT_Z)) {
        d->input |= PLAYER_INPUT_Z_HELD;
    }
    if (input_held(INPUT_L)) {
        d->input |= PLAYER_INPUT_L_HELD;
    }
}

static int player_determine_action(Object *o, PlayerData *d) {
    if (d->action == PLAYER_ACT_ACTION) {
        return PLAYER_ACT_ACTION;
    }
    if (d->action == PLAYER_ACT_ACTION_MOVE) {
        return PLAYER_ACT_ACTION_MOVE;
    }
    if (d->action == PLAYER_ACT_LEDGE) {
        return PLAYER_ACT_LEDGE;
    }
    if (d->input & PLAYER_INPUT_L_HELD && o->collision->grounded) {
        return PLAYER_ACT_IDLE;
    }
    if (d->intendedMag != 0.0f) {
        if (o->collision->grounded) {
            return PLAYER_ACT_MOVE;
        } else {
            return PLAYER_ACT_AIR;
        }
    }
    if (o->collision->grounded) {
        return PLAYER_ACT_IDLE;
    } else {
        return PLAYER_ACT_AIR;
    }
    return PLAYER_ACT_BRUH;
}

void loop(Object *obj, int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    PlayerData *data = (PlayerData *) obj->data;
    float grip;

    data->input = 0;
    data->intendedMag = 0.0f;
    //data->intendedYaw = 0;

    if (data->playerID != -1) {
        player_input(obj, data);
    }

    if (input_pressed(INPUT_DLEFT, 0)) {
        data->health--;
    }
    if (input_pressed(INPUT_DRIGHT, 0)) {
        data->health++;
    }
    if (input_pressed(INPUT_DDOWN, 0)) {
        data->healthMax--;
        if (data->health > data->healthMax) {
            data->health = data->healthMax;
        }
    }
    if (input_pressed(INPUT_DUP, 0)) {
        data->healthMax++;
    }
    Object *targetObj = find_nearest_object_facing(obj, OBJ_NPC, 30.0f, 0x3000, obj->faceAngle[1]);
    if (targetObj) {
        if (targetObj->objectID == OBJ_NPC && DIST3(obj->pos, targetObj->pos) < 10.0f * 10.0f) {
            if (data->input & PLAYER_INPUT_A_PRESSED) {
                data->weaponOut = false;
                talk_open(0);
                data->input &= ~PLAYER_INPUT_A_PRESSED;
                input_clear(INPUT_A);
            }
            targetObj->flags |= OBJ_FLAG_OUTLINE;
        } else {
            targetObj->flags &= ~OBJ_FLAG_OUTLINE;
        }
        if (data->playerID != -1) {
            gCamera->target = targetObj;
        }
    } else {
        if (gCamera->target) {
            gCamera->target->flags &= ~OBJ_FLAG_OUTLINE;
        }
        gCamera->target = NULL;
    }

    data->action = player_determine_action(obj, data);

    assertf(data->action != PLAYER_ACT_BRUH, "Bad player action.");

    (sPlayerFunctions[(int) data->action])(obj, obj->data, updateRate, updateRateF);
    if (obj->collision->grounded) {
        grip = (float) (COL_GET_GRIP(obj->collision->floorFlags));
        grip /= 8.0f;
    } else {
        grip = 0.25f;
    }
    for (int i = 0; i < 3; i += 2) {
        obj->pos[i] += (obj->movement->vel[i] / 7.5f) * updateRateF;
        obj->movement->vel[i] = approachf(obj->movement->vel[i], 0.0f, (fabsf(obj->movement->vel[i] * 0.1f) * grip) * updateRateF);
        if (fabsf(obj->movement->vel[i]) < 0.05f) {
            obj->movement->vel[i] = 0.0f;
        }
    }
    data->forwardVel = sqrtf(SQR(obj->movement->vel[0]) + SQR(obj->movement->vel[2]));

    if (data->playerID != -1) {
        get_time_snapshot(PP_PLAYER, DEBUG_SNAPSHOT_1_END);
    }
}

DynamicShadowData shadow = {
    .texW = 128,
    .texH = 192,
    .planeW = 20.0f,
    .planeH = 45.0f,
    .offset = 2.0f,
    .camPos = {-11.0f * 8, 7.0f * 8, 0.0f},
    .camFocus = {11.0f * 8, 7.0f * 8, 0.0f},
};

Hitbox bbox = {
    .type = HITBOX_CYLINDER,
    .offsetY = 0,
    .width = 24.0f,
    .length = 24.0f,
    .weight = 100.0f,
    .height = 112.0f,
};

ObjectEntry entry = {
    .initFunc = init,
    .loopFunc = loop,
    .name = "Player",
    .data = sizeof(PlayerData),
    .flags = OBJ_FLAG_MOVE | OBJ_FLAG_GRAVITY | OBJ_FLAG_COLLISION | OBJ_FLAG_SHADOW_DYNAMIC | OBJ_FLAG_TANGIBLE,
    .viewDist = OBJ_DIST(800),
    .viewWidth = 3,
    .viewHeight = 14,
    .dynamicShadow = &shadow,
    .hitbox = &bbox,
};