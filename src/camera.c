#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <malloc.h>

#include "camera.h"
#include "../include/global.h"

#include "object.h"
#include "math_util.h"
#include "input.h"
#include "main.h"
#include "debug.h"
#include "menu.h"
#include "hud.h"

Camera *gCamera = NULL;

void camera_reset(void) {
    gCamera->pitch = 0x3400;
    gCamera->zoom = 275;
    gCamera->intendedZoom = 275;
    gCamera->yawTarget = 0x8000;
    gCamera->yaw = 0x8000;
    gCamera->fov = 50.0f;
    if (gPlayer) {
        gCamera->parent = gPlayer;
        gCamera->mode = CAMERA_TARGET;
    } else {
        gCamera->mode = CAMERA_CUTSCENE;
    }
    gCamera->focus[0] = 0;
    gCamera->focus[2] = 0;
    gCamera->focus[1] = 1;

    gCamera->pos[0] = 5.0f;
    gCamera->pos[2] = 0.0f;
    gCamera->pos[1] = 1.0f;
}

void camera_init(void) {
    gCamera = malloc(sizeof(Camera));
    bzero(gCamera, sizeof(Camera));
    camera_reset();
}

void camera_shake(Camera *c, int updateRate, float updateRateF) {
    if (c->shakeTimer > 0) {
        c->shakeTimer -= updateRate;
        float strength = (float) c->shakeTimer / (float) c->shakeLength;
        float shakeSine = ((float) c->shakeStrength * 0.025f) * sins(gGlobalTimer * (0x400 * c->shakeSpeed));

        c->shakePos = shakeSine * strength;
    } else {
        c->shakePos = 0.0f;
    }
}

static void camera_update_target(Camera *c, int updateRate, float updateRateF) {
    float zoom;
    float stickX = input_stick_x(STICK_LEFT);
    float stickY = input_stick_y(STICK_LEFT);
    float pan;
    PlayerData *data = (PlayerData *) c->parent->data;

    if (gMenuStatus == MENU_CLOSED) {
        camera_shake(c, updateRate, updateRateF);
        if (input_held(INPUT_Z)) {
            c->yawTarget = data->cameraAngle + 0x8000;
            INCREASE_VAR(gZTargetTimer, updateRate * 4, timer_int(20));
            c->targetZoom = lerp(c->targetZoom, 1, 0.05f * updateRateF);
        } else {
            DECREASE_VAR(gZTargetTimer, updateRate * 2, 0);
            c->targetZoom = lerp(c->targetZoom, 0, 0.05f * updateRateF);
        }

        if (input_type() == CONTROLLER_N64) {
            if (input_held(INPUT_L)) {
                letMeUseL:
                float intendedPitch = stickY * 100.0f;
                c->yawTarget -= (float) (stickX * ((8.0f * updateRateF)));
                c->pan = lerp(c->pan, 0.0f, 0.025f * updateRateF);
                c->zoomAdd = lerp(c->zoomAdd, 0, 0.05f * updateRateF);
                c->lookPitch = lerp(c->lookPitch, intendedPitch, 0.1f * updateRateF);
            } else {
                c->pan = lerp(c->pan, -stickX, 0.025f * updateRateF);
                c->zoomAdd = lerp(c->zoomAdd, stickY, 0.05f * updateRateF);
                c->lookPitch = lerp(c->lookPitch, 0.0f, 0.1f * updateRateF);
            }
        } else {
            letMeUseC:
            float stickRX = input_stick_x(STICK_RIGHT);
            float stickRY = input_stick_y(STICK_RIGHT);
            float intendedPitch = stickRY * 100.0f;
            c->yawTarget -= (float) (stickRX * ((8.0f * updateRateF)));
            c->pan = lerp(c->pan, -stickX, 0.025f * updateRateF);
            c->zoomAdd = lerp(c->zoomAdd, stickY, 0.05f * updateRateF);
            c->lookPitch = lerp(c->lookPitch, intendedPitch, 0.1f * updateRateF);
        }
    } else if (gMenuStatus == MENU_CONFIG) {
        if (input_type() == CONTROLLER_N64) {
            if (input_held(INPUT_L)) {
                goto letMeUseL;
            }
        } else {
            goto letMeUseC;
        }
    }

    c->yaw = lerp_short(c->yaw, c->yawTarget, 0.25f * updateRateF);
    c->zoom = lerp(c->zoom, c->intendedZoom, 0.05f * updateRateF);

    c->viewPitch = c->pitch + c->lookPitch;
    zoom = (float) (c->zoomAdd + c->zoom + c->targetZoom) / 20.0f;
    pan = (float) c->pan / 20.0f;

    float intendedFocus[3];

    intendedFocus[0] = c->parent->pos[0] + (pan * sins(c->yaw - 0x4000));
    intendedFocus[2] = c->parent->pos[2] + (pan * coss(c->yaw - 0x4000));
    intendedFocus[1] = c->parent->pos[1];

    if (c->target || data->zTarget) {
        Object *tar;
        if (data->zTarget) {
            tar = data->zTarget;
        } else {
            tar = c->target;
        }
        float targetMag;
        if (input_held(INPUT_Z)) {
            targetMag = 0.5f;
        } else {
            targetMag = 1.0f - (DIST3(c->parent->pos, tar->pos) / SQR(30.0f));
            if (targetMag > 0.25f) {
                targetMag = 0.25f;
            }
        }
        c->lookFocus[0] = lerpf(c->lookFocus[0], (tar->pos[0] - intendedFocus[0]) * targetMag, 0.035f * updateRateF);
        c->lookFocus[1] = lerpf(c->lookFocus[1], (tar->pos[1] - intendedFocus[1]) * targetMag, 0.035f * updateRateF);
        c->lookFocus[2] = lerpf(c->lookFocus[2], (tar->pos[2] - intendedFocus[2]) * targetMag, 0.035f * updateRateF);
    } else {
        if (fabsf(c->lookFocus[0]) > 0.01f) {
            c->lookFocus[0] = lerpf(c->lookFocus[0], 0.0f, 0.05f * updateRateF);
        }
        if (fabsf(c->lookFocus[1]) > 0.01f) {
            c->lookFocus[1] = lerpf(c->lookFocus[1], 0.0f, 0.05f * updateRateF);
        }
        if (fabsf(c->lookFocus[2]) > 0.01f) {
            c->lookFocus[2] = lerpf(c->lookFocus[2], 0.0f, 0.05f * updateRateF);
        }
    }

    c->focus[0] = intendedFocus[0] + c->lookFocus[0];
    c->focus[2] = intendedFocus[2] + c->lookFocus[2];
    c->focus[1] = intendedFocus[1] + c->lookFocus[1] + 10.0f + c->shakePos;

    c->pos[0] = intendedFocus[0] + ((zoom) * coss(c->yaw - 0x4000));
    c->pos[2] = intendedFocus[2] - ((zoom) * sins(c->yaw - 0x4000));
    c->pos[1] = intendedFocus[1] + 10.0f + (11.5f * sins(c->viewPitch + 0x4000)) + (c->shakePos * 1.25f);

    if (input_held(INPUT_R)) {
        input_clear(INPUT_R);
        c->shakeSpeed = 15;
        c->shakeStrength = 10;
        c->shakeTimer = 90;
        c->shakeLength = 90;
    }
}

static void camera_update_photo(Camera *c, int updateRate, float updateRateF) {
    float stickMag = input_stick_mag(STICK_LEFT);
    u_uint16_t stickAngle = input_stick_angle(STICK_LEFT);
    
    c->pos[0] += ((stickMag * sins(c->yaw + stickAngle)) / 2.0f) * updateRateF;
    c->pos[2] += ((stickMag * coss(c->yaw + stickAngle)) / 2.0f) * updateRateF;

    if (input_held(INPUT_CLEFT)) {
        c->yawTarget += 0x100 * updateRate;
    } else if (input_held(INPUT_CRIGHT)) {
        c->yawTarget -= 0x100 * updateRate;
    }

    if (input_held(INPUT_CUP)) {
        c->pitch += 0x80 * updateRate;
        if (c->pitch > 0x3F00) {
            c->pitch = 0x3F00;
        }
    } else if (input_held(INPUT_CDOWN)) {
        c->pitch -= 0x80 * updateRate;
        if (c->pitch < -0x3F00) {
            c->pitch = -0x3F00;
        }
    }

    if (input_held(INPUT_DUP)) {
        c->fov -= 0.5f * updateRateF;
        if (c->fov < 15.0f) {
            c->fov = 15.0f;
        }
    } else if (input_held(INPUT_DDOWN)) {
        c->fov += 0.5f * updateRateF;
        if (c->fov > 90.0f) {
            c->fov = 90.0f;
        }
    }

    if (input_held(INPUT_Z) || input_held(INPUT_L)) {
        c->pos[1] -= 0.5f * updateRateF;
    } else if (input_held(INPUT_R)) {
        c->pos[1] += 0.5f * updateRateF;
    }

    if (input_pressed(INPUT_A, 3)) {
        gCameraHudToggle ^= 1;
        input_clear(INPUT_A);
    }

    
    c->yaw = lerp_short(c->yaw, c->yawTarget, 0.25f * updateRateF);

    c->focus[0] = c->pos[0] + (coss(-c->yaw - 0x4000) * coss(c->pitch));
    c->focus[1] = c->pos[1] + (sins(c->pitch));
    c->focus[2] = c->pos[2] + (sins(-c->yaw - 0x4000) * coss(c->pitch));
}

void camera_loop(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    Camera *c = gCamera;

    switch (c->mode) {
    case CAMERA_TARGET:
        camera_update_target(c, updateRate, updateRateF);
        break;
    case CAMERA_PHOTO:
        camera_update_photo(c, updateRate, updateRateF);
        break;
    
    }
    get_time_snapshot(PP_CAMERA, DEBUG_SNAPSHOT_1_END);
}