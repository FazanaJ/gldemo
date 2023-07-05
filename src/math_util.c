#include <libdragon.h>
#include <math.h>

#include "math_util.h"
#include "../include/global.h"
#include "../include/trig_tables.h"

#include "main.h"

int lerp(int current, int target, float factor) {
    return current + ((float) (target - current) * factor);
}

int lerp_short(int current, int target, float factor) {
    return target - (((short)(target - current)) + ((float) (0 - ((short)(target - current))) * factor));
}

float lerpf(float current, float target, float factor) {
    return current + ((target - current) * factor);
}

/**
 * Normalise a set timer to play nice with both PAL and NTSC.
 * PAL timers will be 20% lower.
*/
int timer_int(int timer) {
	if (gConfig.regionMode == TV_PAL) {
		return timer / (60 / 50);
	} else {
		return timer;
	}
}

/**
 * Normalise a set timer to play nice with both PAL and NTSC.
 * PAL timers will be 20% lower.
*/
float timer_float(float timer) {
	if (gConfig.regionMode == TV_PAL) {
		return timer / (60.0f / 50.0f);
	} else {
		return timer;
	}
}

static int atan2_lookup(float y, float x) {
    int ret;

    if (x == 0) {
        ret = gArctanTable[0];
    } else {
        ret = gArctanTable[(int)(y / x * 1024 + 0.5f)];
    }
    return (short) ret;
}

int atan2s(float y, float x) {
    int ret;

    if (x >= 0) {
        if (y >= 0) {
            if (y >= x) {
                ret = atan2_lookup(x, y);
            } else {
                ret = 0x4000 - atan2_lookup(y, x);
            }
        } else {
            y = -y;
            if (y < x) {
                ret = 0x4000 + atan2_lookup(y, x);
            } else {
                ret = 0x8000 - atan2_lookup(x, y);
            }
        }
    } else {
        x = -x;
        if (y < 0) {
            y = -y;
            if (y >= x) {
                ret = 0x8000 + atan2_lookup(x, y);
            } else {
                ret = 0xC000 - atan2_lookup(y, x);
            }
        } else {
            if (y < x) {
                ret = 0xC000 + atan2_lookup(y, x);
            } else {
                ret = -atan2_lookup(x, y);
            }
        }
    }
    return (short) ret;
}