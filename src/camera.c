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

Camera *gCamera;

void camera_init(void) {
    gCamera = malloc(sizeof(Camera));
    bzero(gCamera, sizeof(Camera));
    
    gCamera->pitch = 0x3400;
    gCamera->zoom = 8.0f;
    gCamera->intendedZoom = 4.0f;
    if (gPlayer) {
        gCamera->parent = gPlayer;
    }
}

void camera_loop(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    Camera *c = gCamera;
    float zoom;
    float stickX = get_stick_x();
    float stickY = get_stick_y();
    short pitch;

    if (get_input_held(INPUT_Z)) {
        c->yawTarget = c->parent->faceAngle[2] + 0x8000;
        INCREASE_VAR(gZTargetTimer, updateRate * 4, timer_int(20));
        if (c->targetZoom < 0.99f) {
            c->targetZoom = lerpf(c->targetZoom, 1.0f, 0.05f * updateRateF);
        }
    } else {
        DECREASE_VAR(gZTargetTimer, updateRate * 2, 0);
        if (c->targetZoom > 0.01f) {
            c->targetZoom = lerpf(c->targetZoom, 0.0f, 0.05f * updateRateF);
        }
    }

    if (get_input_held(INPUT_L)) {
        float intendedPitch = stickY * 100.0f;
        c->yawTarget -= (float) (stickX * ((8.0f * updateRateF)));
        if (c->pan > 0.0f) {
            c->pan = lerpf(c->pan, 0.0f, 0.025f * updateRateF);
        }
        if (c->zoomAdd > 0.0f) {
            c->zoomAdd = lerpf(c->zoomAdd, 0.0f, 0.05f * updateRateF);
        }
        if (fabs(c->lookPitch - intendedPitch) > 0.01f) {
            c->lookPitch = lerp(c->lookPitch, intendedPitch, 0.1f * updateRateF);
        }
    } else {
        if (stickX != 0.0f || fabs(c->pan) > 0.001f) {
            c->pan = lerpf(c->pan, (-stickX / 75.0f), 0.025f * updateRateF);
        }
        if (stickY != 0.0f || fabs(c->zoomAdd) > 0.001f) {
            c->zoomAdd = lerpf(c->zoomAdd, (stickY / 75.0f), 0.05f * updateRateF);
        }
        if (c->lookPitch > 0.0f) {
            c->lookPitch = lerp(c->lookPitch, 0.0f, 0.1f * updateRateF);
        }
    }

    c->yaw = lerp_short(c->yaw, c->yawTarget, 0.25f * updateRateF);
    if (fabs(c->zoom - c->intendedZoom) > 0.01f) {
        c->zoom = lerpf(c->zoom, c->intendedZoom, 0.05f * updateRateF);
    }

    pitch = c->pitch + c->lookPitch;
    zoom = c->zoomAdd + c->zoom + c->targetZoom;
    float intendedFocus[3];

    intendedFocus[0] = c->parent->pos[0] + (c->pan * sins(c->yaw - 0x4000));
    intendedFocus[1] = c->parent->pos[1] - (c->pan * coss(c->yaw - 0x4000));
    intendedFocus[2] = c->parent->pos[2];

    if (c->target) {
        float targetMag;
        if (get_input_held(INPUT_Z)) {
            targetMag = 1.0f;
        } else {
            targetMag = 1.0f - (DIST3(c->parent->pos, c->target->pos) / SQR(3.0f));
            if (targetMag > 0.5f) {
                targetMag = 0.5f;
            }
        }
        c->lookFocus[0] = lerpf(c->lookFocus[0], (c->target->pos[0] - intendedFocus[0]) * targetMag, 0.035f * updateRateF);
        c->lookFocus[1] = lerpf(c->lookFocus[1], (c->target->pos[1] - intendedFocus[1]) * targetMag, 0.035f * updateRateF);
        c->lookFocus[2] = lerpf(c->lookFocus[2], (c->target->pos[2] - intendedFocus[2]) * targetMag, 0.035f * updateRateF);
    } else {
        c->lookFocus[0] = lerpf(c->lookFocus[0], 0.0f, 0.01f * updateRateF);
        c->lookFocus[1] = lerpf(c->lookFocus[1], 0.0f, 0.01f * updateRateF);
        c->lookFocus[2] = lerpf(c->lookFocus[2], 0.0f, 0.01f * updateRateF);
    }

    

    c->focus[0] = intendedFocus[0] + c->lookFocus[0];
    c->focus[1] = intendedFocus[1] + c->lookFocus[1];
    c->focus[2] = intendedFocus[2] + c->lookFocus[2] + 2.0f;

    c->pos[0] = intendedFocus[0] + ((zoom) * coss(c->yaw - 0x4000));
    c->pos[1] = intendedFocus[1] + ((zoom) * sins(c->yaw - 0x4000));
    c->pos[2] = intendedFocus[2] + 2.0f + (3.5f * sins(pitch + 0x4000));
    get_time_snapshot(PP_CAMERA, DEBUG_SNAPSHOT_1_END);
}