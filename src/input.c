#include <libdragon.h>
#include <math.h>

#include "input.h"
#include "../include/global.h"

#include "object.h"
#include "camera.h"
#include "math_util.h"
#include "debug.h"
#include "main.h"

int gCurrentController = -1;
Input gInputData;
static char sPakDetectionTimer = 0;
static short sRumbleTimer = 0;

static void input_set_id(void) {
    for (int i = 0; i < 4; i++) {
        joypad_inputs_t pad = joypad_get_inputs(i);
        if (joypad_is_connected(i) && pad.btn.start) {
            gCurrentController = i;
            return;
        }
    }
}

void input_reset(void) {
    bzero(&gInputData, sizeof(Input));
    for (int i = 0; i < INPUT_TOTAL; i++) {
        gInputData.button[INPUT_PRESSED][i] = 250;
        gInputData.button[INPUT_RELEASED][i] = 250;
    }
}

void input_update(int updateRate) {
    DEBUG_SNAPSHOT_1();
    joypad_poll();
    int p = gCurrentController;
    if (p == -1) {
        input_set_id();
        input_reset();
        get_time_snapshot(PP_INPUT, DEBUG_SNAPSHOT_1_END);
        return;
    }

    sPakDetectionTimer -= updateRate;
    if (sPakDetectionTimer <= 0) {
        gInputData.pak = joypad_get_accessory_type(p);
        sPakDetectionTimer = timer_int(120);
    }

    if (sRumbleTimer > 0) {
        sRumbleTimer -= updateRate;
        if (sRumbleTimer <= 0) {
            joypad_set_rumble_active(p, false);
        }
    }

    Input *controller = &gInputData;
    joypad_buttons_t pad;
    for (int i = 0; i < 3; i++) {
        switch (i) {
        // Interestingly, the order here matters.
        // Since I read the stick data, I need the last one to be held, otherwise the stick input is unusable.
        case INPUT_PRESSED:
            pad = joypad_get_buttons_pressed(p);
            break;
        case INPUT_RELEASED:
            pad = joypad_get_buttons_released(p);
            break;
        case INPUT_HELD:
            pad = joypad_get_buttons_held(p);
            break;
        }
        unsigned int btn = pad.raw;
        for (int j = 0; j < INPUT_TOTAL; j++) {
            if (btn & (1 << j)) {
                controller->button[i][j] = 0;
            } else {
                INCREASE_VAR(controller->button[i][j], updateRate, 250);
            }
        }
    }
    joypad_inputs_t stick = joypad_get_inputs(p);
    // This is a little bit wordy but it's worth it.
    // Take the raw stick inputs.
    // The deadzone is 10 units, so zero out anything less.
    // Take the deadzone away from the stick reading. This eliminates the sudden increase in the curve.
    // Cap the reading at 75, which is around three quarters of the way, then normalise the stick mag from 0-1.
    // The end result is a smooth increase from neutral to furthest, with plenty of leeway for degraded sticks.
    controller->stickX[0] = stick.stick_x;
    if (fabs(controller->stickX[0]) < (int) DEADZONE) {
        controller->stickX[0] = 0;
    } else {
        if (controller->stickX[0] > 0) {
            controller->stickX[0] -= DEADZONE;
            if (controller->stickX[0] > 75) {
                controller->stickX[0] = 75;
            }
        } else {
            controller->stickX[0] += DEADZONE;
            if (controller->stickX[0] < -75) {
                controller->stickX[0] = -75;
            }
        }
    }
    controller->stickY[0] = stick.stick_y;
    if (fabs(controller->stickY[0]) < (int) DEADZONE) {
        controller->stickY[0] = 0;
    } else {
        if (controller->stickY[0] > 0) {
            controller->stickY[0] -= DEADZONE;
            if (controller->stickY[0] > 75) {
                controller->stickY[0] = 75;
            }
        } else {
            controller->stickY[0] += DEADZONE;
            if (controller->stickY[0] < -75) {
                controller->stickY[0] = -75;
            }
        }
    }
    controller->type = CONTROLLER_N64;
    controller->stickMag[0] = fabsf(sqrtf((controller->stickX[0] * controller->stickX[0]) + (controller->stickY[0] * controller->stickY[0]))) / 75.0f;
    controller->stickAngle[0] = atan2s(-controller->stickY[0], controller->stickX[0]);

    //Temp
    controller->stickMag[1] = controller->stickMag[0];
    controller->stickAngle[1] = controller->stickAngle[0];
    controller->stickX[1] = controller->stickX[0];
    controller->stickY[1] = controller->stickY[0];
    get_time_snapshot(PP_INPUT, DEBUG_SNAPSHOT_1_END);
}

int input_pressed(int input, int numFrames) {
    return gInputData.button[INPUT_PRESSED][input] <= numFrames;
}

int input_held(int input) {
    return gInputData.button[INPUT_HELD][input] == 0;
}

int input_released(int input, int numFrames) {
    return gInputData.button[INPUT_RELEASED][input] <= numFrames;
}

int input_stick_x(int type) {
    return gInputData.stickX[type];
}

int input_stick_y(int type) {
    return gInputData.stickY[type];
}

short input_stick_angle(int type) {
    return gInputData.stickAngle[type];
}

float input_stick_mag(int type) {
    return gInputData.stickMag[type];
}

int input_type(void) {
    return gInputData.type;
}

void input_rumble(int timer) {
    if (gInputData.pak != ACCESSORY_RUMBLEPAK) {
        return;
    }

	if (gConfig.regionMode == TV_PAL) {
		timer /= (60 / 50);
	}

    joypad_set_rumble_active(gCurrentController, true);
    sRumbleTimer = timer;
}

/**
 * Sets the press timer to maximum, effectively making it so the button hasn't been touched.
*/
void input_clear(int input) {
    gInputData.button[INPUT_PRESSED][input] = 250;
    gInputData.button[INPUT_RELEASED][input] = 250;
}