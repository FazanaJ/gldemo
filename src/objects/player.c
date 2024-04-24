#include <libdragon.h>

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

static void player_grounded_common(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    if (d->input & PLAYER_INPUT_A_PRESSED) {
        if (d->playerID != -1) {
            input_clear(INPUT_A);
        }
        d->input &= ~PLAYER_INPUT_A_PRESSED;
        o->movement->vel[1] = 12.0f;
    }
}

static void player_attack(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    if (d->input & PLAYER_INPUT_B_PRESSED) {
        if (d->weaponOut == false) {
            d->weaponOut = true;
        }
        if (d->playerID != -1) {
            input_clear(INPUT_B);
        }
        d->input &= ~PLAYER_INPUT_B_PRESSED;
        d->velOffset = 16.0f;
    }
}

static void player_act_idle(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    o->movement->vel[0] = approachf(o->movement->vel[0], 0.0f, 2.0f * updateRateF);
    o->movement->vel[1] = 0.0f;
    o->movement->vel[2] = approachf(o->movement->vel[2], 0.0f, 2.0f * updateRateF);
    if (o->movement->vel[0] != 0.0f || o->movement->vel[2] != 0.0f) {
        o->movement->moveAngle[1] = atan2s(o->movement->vel[2], o->movement->vel[0]);
    }
    if ((d->input & PLAYER_INPUT_L_HELD) == false) {
        player_grounded_common(o, d, updateRate, updateRateF);
    }
    player_attack(o, d, updateRate, updateRateF);
}

static void player_act_move(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    short angle = d->intendedYaw + d->offsetYaw;
    float factor;
    o->movement->vel[0] = approachf(o->movement->vel[0], 32.0f * (d->intendedMag * sins(angle)), 2.0f * updateRateF);
    o->movement->vel[2] = approachf(o->movement->vel[2], 32.0f * (d->intendedMag * coss(angle)), 2.0f * updateRateF);
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
        o->faceAngle[1] = lerp_short(o->faceAngle[1], o->movement->moveAngle[1], factor * updateRateF);
        d->cameraAngle = o->faceAngle[1];
    }
    player_grounded_common(o, d, updateRate, updateRateF);
    player_attack(o, d, updateRate, updateRateF);
}

static void player_act_air(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    short angle = d->intendedYaw + d->offsetYaw;
    o->movement->vel[0] = approachf(o->movement->vel[0], 32.0f * (d->intendedMag * sins(angle)), 0.75f * updateRateF);
    o->movement->vel[2] = approachf(o->movement->vel[2], 32.0f * (d->intendedMag * coss(angle)), 0.75f * updateRateF);
    o->movement->moveAngle[1] = atan2s(o->movement->vel[2], o->movement->vel[0]);
    if (gZTargetTimer == 0) {
        d->cameraAngle = o->faceAngle[1];
    }

    if (o->movement->vel[1] < 0.0f) {
        float offset = o->hitbox->width + 3.0f;
        float posX = o->pos[0] + (offset * sins(o->faceAngle[1]));
        float posZ = o->pos[2] + (offset * coss(o->faceAngle[1]));
        float height = collision_floor(posX, o->pos[1] + o->hitbox->height + 2.0f, posZ, NULL, false);
        float heightDiff = o->pos[1] - height;
        if (heightDiff < -15.0f && heightDiff > -25.0f) {
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

static void player_act_action(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    o->movement->vel[0] = approachf(o->movement->vel[0], 0.0f, 2.0f * updateRateF);
    o->movement->vel[2] = approachf(o->movement->vel[2], 0.0f, 2.0f * updateRateF);
}

static void player_act_action_move(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    
}

static void player_act_swim(Object *o, PlayerData *d, int updateRate, float updateRateF) {
    
}

static void player_act_ledge(Object *o, PlayerData *d, int updateRate, float updateRateF) {
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

static void (*sPlayerFunctions[])(Object *, PlayerData *, int, float) = {
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
}

static void player_input(Object *o, PlayerData *d) {
    if (gCurrentController == -1 && d->playerID != -1) {
        return;
    }
    d->offsetYaw = gCamera->yawTarget;
    d->intendedMag = input_stick_mag(STICK_LEFT);
    d->intendedYaw = input_stick_angle(STICK_LEFT);
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
    int grounded = (o->pos[1] - o->collision->floorHeight < FLOOR_MARGIN) && (o->movement->vel[1] <= 0.0f);
    if (d->input & PLAYER_INPUT_L_HELD && grounded) {
        return PLAYER_ACT_IDLE;
    }
    if (d->intendedMag != 0.0f) {
        if (grounded) {
            return PLAYER_ACT_MOVE;
        } else {
            return PLAYER_ACT_AIR;
        }
    }
    if (grounded) {
        return PLAYER_ACT_IDLE;
    } else {
        return PLAYER_ACT_AIR;
    }
    return PLAYER_ACT_BRUH;
}

void loop(Object *obj, int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    PlayerData *data = (PlayerData *) obj->data;

    data->input = 0;
    data->intendedMag = 0.0f;
    data->intendedYaw = 0;

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

    obj->pos[0] += ((obj->movement->vel[0] + (data->velOffset * sins(obj->faceAngle[1]))) / 60.0f) * updateRateF;
    obj->pos[2] += ((obj->movement->vel[2] + (data->velOffset * coss(obj->faceAngle[1]))) / 60.0f) * updateRateF;
    data->velOffset = approachf(data->velOffset, 0.0f, 2.0f * updateRateF);
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
    .offset = -7.0f,
    .camPos = {-11.0f, 7.0f, 0.0f},
    .camFocus = {11.0f, 7.0f, 0.0f},
};

Hitbox bbox = {
    .type = HITBOX_CYLINDER,
    .offsetY = 0,
    .width = 3.0f,
    .length = 3.0f,
    .weight = 100.0f,
    .height = 14.0f,
};

ObjectEntry entry = {
    .initFunc = init,
    .loopFunc = loop,
    .name = "Player",
    .data = sizeof(PlayerData),
    .flags = OBJ_FLAG_MOVE | OBJ_FLAG_GRAVITY | OBJ_FLAG_COLLISION | OBJ_FLAG_SHADOW_DYNAMIC | OBJ_FLAG_TANGIBLE,
    .viewDist = OBJ_DIST(100),
    .viewWidth = 3,
    .viewHeight = 14,
    .dynamicShadow = &shadow,
    .hitbox = &bbox,
};