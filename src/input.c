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
struct controller_data gController;
static Input sInputData;
static char sPakDetectionTimer = 0;
static short sRumbleTimer = 0;

static void set_player_controller_id(void) {
    struct controller_data pad = get_keys_pressed();
    for (int i = 0; i < 4; i++) {
        if (get_controllers_present() & (CONTROLLER_1_INSERTED >> (i * 4)) && pad.c[i].start) {
            gCurrentController = i;
            return;
        }
    }
}

void init_controller(void) {
    controller_init();
    bzero(&sInputData, sizeof(Input));
    sInputData.pak = 0;
}

void reset_controller_inputs(void) {
    bzero(&sInputData, sizeof(Input));
    for (int i = 0; i < INPUT_TOTAL; i++) {
        sInputData.button[INPUT_PRESSED][i] = 250;
        sInputData.button[INPUT_RELEASED][i] = 250;
    }
}

void update_inputs(int updateRate) {
    DEBUG_SNAPSHOT_1();
    controller_scan();
    int p = gCurrentController;
    if (p == -1) {
        set_player_controller_id();
        reset_controller_inputs();
        get_time_snapshot(PP_INPUT, DEBUG_SNAPSHOT_1_END);
        return;
    }

    sPakDetectionTimer -= updateRate;
    if (sPakDetectionTimer <= 0) {
        sInputData.pak = identify_accessory(p);
        sPakDetectionTimer = timer_int(120);
    }

    if (sRumbleTimer > 0) {
        sRumbleTimer -= updateRate;
        if (sRumbleTimer <= 0) {
            rumble_stop(p);
        }
    }

    Input *controller = &sInputData;
    struct controller_data pad;
    for (int i = 0; i < 3; i++) {
        switch (i) {
        // Interestingly, the order here matters.
        // Since I read the stick data, I need the last one to be held, otherwise the stick input is unusable.
        case INPUT_PRESSED:
            pad = get_keys_down();
            break;
        case INPUT_RELEASED:
            pad = get_keys_up();
            break;
        case INPUT_HELD:
            pad = get_keys_held();
            break;
        }
        // This is pretty mega cringe but a necessary evil.
        if (pad.c[p].A) controller->button[i][INPUT_A] = 0; else controller->button[i][INPUT_A] += updateRate;
        if (controller->button[i][INPUT_A] > 250) controller->button[i][INPUT_A] = 250;
        if (pad.c[p].B) controller->button[i][INPUT_B] = 0; else controller->button[i][INPUT_B] += updateRate;
        if (controller->button[i][INPUT_B] > 250) controller->button[i][INPUT_B] = 250;
        if (pad.c[p].start) controller->button[i][INPUT_START] = 0; else controller->button[i][INPUT_START] += updateRate;
        if (controller->button[i][INPUT_START] > 250) controller->button[i][INPUT_START] = 250;
        if (pad.c[p].L) controller->button[i][INPUT_L] = 0; else controller->button[i][INPUT_L] += updateRate;
        if (controller->button[i][INPUT_L] > 250) controller->button[i][INPUT_L] = 250;
        if (pad.c[p].R) controller->button[i][INPUT_R] = 0; else controller->button[i][INPUT_R] += updateRate;
        if (controller->button[i][INPUT_R] > 250) controller->button[i][INPUT_R] = 250;
        if (pad.c[p].Z) controller->button[i][INPUT_Z] = 0; else controller->button[i][INPUT_Z] += updateRate;
        if (controller->button[i][INPUT_Z] > 250) controller->button[i][INPUT_Z] = 250;
        if (pad.c[p].C_up) controller->button[i][INPUT_CUP] = 0; else controller->button[i][INPUT_CUP] += updateRate;
        if (controller->button[i][INPUT_CUP] > 250) controller->button[i][INPUT_CUP] = 250;
        if (pad.c[p].C_left) controller->button[i][INPUT_CLEFT] = 0; else controller->button[i][INPUT_CLEFT] += updateRate;
        if (controller->button[i][INPUT_CLEFT] > 250) controller->button[i][INPUT_CLEFT] = 250;
        if (pad.c[p].C_right) controller->button[i][INPUT_CRIGHT] = 0; else controller->button[i][INPUT_CRIGHT] += updateRate;
        if (controller->button[i][INPUT_CRIGHT] > 250) controller->button[i][INPUT_CRIGHT] = 250;
        if (pad.c[p].C_down) controller->button[i][INPUT_CDOWN] = 0; else controller->button[i][INPUT_CDOWN] += updateRate;
        if (controller->button[i][INPUT_CDOWN] > 250) controller->button[i][INPUT_CDOWN] = 250;
        if (pad.c[p].up) controller->button[i][INPUT_DUP] = 0; else controller->button[i][INPUT_DUP] += updateRate;
        if (controller->button[i][INPUT_DUP] > 250) controller->button[i][INPUT_DUP] = 250;
        if (pad.c[p].left) controller->button[i][INPUT_DLEFT] = 0; else controller->button[i][INPUT_DLEFT] += updateRate;
        if (controller->button[i][INPUT_DLEFT] > 250) controller->button[i][INPUT_DLEFT] = 250;
        if (pad.c[p].right) controller->button[i][INPUT_DRIGHT] = 0; else controller->button[i][INPUT_DRIGHT] += updateRate;
        if (controller->button[i][INPUT_DRIGHT] > 250) controller->button[i][INPUT_DRIGHT] = 250;
        if (pad.c[p].down) controller->button[i][INPUT_DDOWN] = 0; else controller->button[i][INPUT_DDOWN] += updateRate;
        if (controller->button[i][INPUT_DDOWN] > 250) controller->button[i][INPUT_DDOWN] = 250;
    }
    // This is a little bit wordy but it's worth it.
    // Take the raw stick inputs.
    // The deadzone is 10 units, so zero out anything less.
    // Take the deadzone away from the stick reading. This eliminates the sudden increase in the curve.
    // Cap the reading at 75, which is around three quarters of the way, then normalise the stick mag from 0-1.
    // The end result is a smooth increase from neutral to furthest, with plenty of leeway for degraded sticks.
    controller->stickX = pad.c[p].x;
    if (fabs(controller->stickX) < DEADZONE) {
        controller->stickX = 0;
    } else {
        if (controller->stickX > 0) {
            controller->stickX -= DEADZONE;
            if (controller->stickX > 75) {
                controller->stickX = 75;
            }
        } else {
            controller->stickX += DEADZONE;
            if (controller->stickX < -75) {
                controller->stickX = -75;
            }
        }
    }
    controller->stickY = pad.c[p].y;
    if (fabs(controller->stickY) < DEADZONE) {
        controller->stickY = 0;
    } else {
        if (controller->stickY > 0) {
            controller->stickY -= DEADZONE;
            if (controller->stickY > 75) {
                controller->stickY = 75;
            }
        } else {
            controller->stickY += DEADZONE;
            if (controller->stickY < -75) {
                controller->stickY = -75;
            }
        }
    }
    controller->stickMag = fabs(sqrtf((controller->stickX * controller->stickX) + (controller->stickY * controller->stickY))) / 75.0f;
    controller->stickAngle = atan2s(-controller->stickY, controller->stickX);
    get_time_snapshot(PP_INPUT, DEBUG_SNAPSHOT_1_END);
}

int get_input_pressed(int input, int numFrames) {
    return sInputData.button[INPUT_PRESSED][input] <= numFrames;
}

int get_input_held(int input) {
    return sInputData.button[INPUT_HELD][input] == 0;
}

int get_input_released(int input, int numFrames) {
    return sInputData.button[INPUT_RELEASED][input] <= numFrames;
}

int get_stick_x(void) {
    return sInputData.stickX;
}

int get_stick_y(void) {
    return sInputData.stickY;
}

short get_stick_angle(void) {
    return sInputData.stickAngle;
}

float get_stick_mag(void) {
    return sInputData.stickMag;
}

void rumble_set(int timer) {
    if (sInputData.pak != ACCESSORY_RUMBLEPAK) {
        return;
    }

	if (gConfig.regionMode == TV_PAL) {
		timer /= (60 / 50);
	}

    rumble_start(gCurrentController);
    sRumbleTimer = timer;
}

/**
 * Sets the press timer to maximum, effectively making it so the button hasn't been touched.
*/
void clear_input(int input) {
    sInputData.button[INPUT_PRESSED][input] = 250;
    sInputData.button[INPUT_RELEASED][input] = 250;
}