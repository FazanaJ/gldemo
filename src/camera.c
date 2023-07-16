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

Camera *gCamera;

void camera_init(void) {
    gCamera = malloc(sizeof(Camera));
    bzero(gCamera, sizeof(Camera));
    
    gCamera->pitch = 0x3400;
    gCamera->zoom = 800;
    gCamera->intendedZoom = 400;
    if (gPlayer) {
        gCamera->parent = gPlayer;
    }
}

void camera_loop(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    Camera *c = gCamera;
    float zoom;
    float stickX = get_stick_x(STICK_LEFT);
    float stickY = get_stick_y(STICK_LEFT);
    float pan;
    PlayerData *data = (PlayerData *) c->parent->data;

    if (gMenuStatus == MENU_CLOSED) {
        if (get_input_held(INPUT_Z)) {
            c->yawTarget = data->cameraAngle + 0x8000;
            INCREASE_VAR(gZTargetTimer, updateRate * 4, timer_int(20));
            c->targetZoom = lerp(c->targetZoom, 1, 0.05f * updateRateF);
        } else {
            DECREASE_VAR(gZTargetTimer, updateRate * 2, 0);
            c->targetZoom = lerp(c->targetZoom, 0, 0.05f * updateRateF);
        }

        if (get_controller_type() == CONTROLLER_N64) {
            if (get_input_held(INPUT_L)) {
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
            float stickRX = get_stick_x(STICK_RIGHT);
            float stickRY = get_stick_y(STICK_RIGHT);
            float intendedPitch = stickRY * 100.0f;
            c->yawTarget -= (float) (stickRX * ((8.0f * updateRateF)));
            c->pan = lerp(c->pan, -stickX, 0.025f * updateRateF);
            c->zoomAdd = lerp(c->zoomAdd, stickY, 0.05f * updateRateF);
            c->lookPitch = lerp(c->lookPitch, intendedPitch, 0.1f * updateRateF);
        }
    }

    c->yaw = lerp_short(c->yaw, c->yawTarget, 0.25f * updateRateF);
    c->zoom = lerp(c->zoom, c->intendedZoom, 0.05f * updateRateF);

    c->viewPitch = c->pitch + c->lookPitch;
    zoom = (float) (c->zoomAdd + c->zoom + c->targetZoom) / 20.0f;
    pan = (float) c->pan / 20.0f;

    float intendedFocus[3];

    intendedFocus[0] = c->parent->pos[0] + (pan * sins(c->yaw - 0x4000));
    intendedFocus[1] = c->parent->pos[1] - (pan * coss(c->yaw - 0x4000));
    intendedFocus[2] = c->parent->pos[2];

    if (c->target || data->zTarget) {
        Object *tar;
        if (data->zTarget) {
            tar = data->zTarget;
        } else {
            tar = c->target;
        }
        float targetMag;
        if (get_input_held(INPUT_Z)) {
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
        if (fabs(c->lookFocus[0]) > 0.01f) {
            c->lookFocus[0] = lerpf(c->lookFocus[0], 0.0f, 0.05f * updateRateF);
        }
        if (fabs(c->lookFocus[1]) > 0.01f) {
            c->lookFocus[1] = lerpf(c->lookFocus[1], 0.0f, 0.05f * updateRateF);
        }
        if (fabs(c->lookFocus[2]) > 0.01f) {
            c->lookFocus[2] = lerpf(c->lookFocus[2], 0.0f, 0.05f * updateRateF);
        }
    }

    c->focus[0] = intendedFocus[0] + c->lookFocus[0];
    c->focus[1] = intendedFocus[1] + c->lookFocus[1];
    c->focus[2] = intendedFocus[2] + c->lookFocus[2] + 10.0f;

    c->pos[0] = intendedFocus[0] + ((zoom) * coss(c->yaw - 0x4000));
    c->pos[1] = intendedFocus[1] + ((zoom) * sins(c->yaw - 0x4000));
    c->pos[2] = intendedFocus[2] + 10.0f + (17.5f * sins(c->viewPitch + 0x4000));
    get_time_snapshot(PP_CAMERA, DEBUG_SNAPSHOT_1_END);
}