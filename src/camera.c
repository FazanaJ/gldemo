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
    gCamera->zoom = 4.0f;
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
        INCREASE_VAR(gZTargetTimer, updateRate * 4, timer_int(20))
    } else {
        DECREASE_VAR(gZTargetTimer, updateRate * 2, 0)
    }

    if (get_input_held(INPUT_L)) {
        c->yawTarget -= (float) (stickX * ((8.0f * updateRateF)));
        c->pan = lerpf(c->pan, 0.0f, 0.025f * updateRateF);
        c->zoomAdd = lerpf(c->zoomAdd, 0.0f, 0.05f * updateRateF);
        c->lookPitch = lerp(c->lookPitch, stickY * 100.0f, 0.1f * updateRateF);
    } else {
        if (stickX != 0.0f || fabs(c->pan) > 0.001f) {
            c->pan = lerpf(c->pan, (-stickX / 75.0f), 0.025f * updateRateF);
        }
        if (stickY != 0.0f || fabs(c->zoomAdd) > 0.001f) {
            c->zoomAdd = lerpf(c->zoomAdd, (stickY / 75.0f), 0.05f * updateRateF);
        }
        c->lookPitch = lerp(c->lookPitch, 0.0f, 0.1f * updateRateF);
    }

    c->yaw = lerp_short(c->yaw, c->yawTarget, 0.25f * updateRateF);

    pitch = c->pitch + c->lookPitch;
    zoom = c->zoomAdd + c->zoom;
    
    c->focus[0] = c->parent->pos[0] + (c->pan * sins(c->yaw - 0x4000));
    c->focus[1] = c->parent->pos[1] - (c->pan * coss(c->yaw - 0x4000));
    c->focus[2] = c->parent->pos[2] + 2.0f;

    c->pos[0] = c->focus[0] + ((zoom) * coss(c->yaw - 0x4000));
    c->pos[1] = c->focus[1] + ((zoom) * sins(c->yaw - 0x4000));
    c->pos[2] = c->focus[2] + (3.5f * sins(pitch + 0x4000));
    get_time_snapshot(PP_CAMERA, DEBUG_SNAPSHOT_1_END);
}