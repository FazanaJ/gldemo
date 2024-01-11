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

int approach(int current, int target, int inc) {
    if (current < target) {
        current += inc;
        if (current > target) {
            current = target;
        }
    } else {
        current -= inc;
        if (current < target) {
            current = target;
        }
    }
    return current;
}

float approachf(float current, float target, float inc) {
    if (current < target) {
        current += inc;
        if (current > target) {
            current = target;
        }
    } else {
        current -= inc;
        if (current < target) {
            current = target;
        }
    }
    return current;
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

    if (fabs(x) <= 0.01f) {
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

int gRandomSeed16;

int random_u16(void) {
    int temp1, temp2;

    if (gRandomSeed16 == 22026) {
        gRandomSeed16 = 0;
    }

    temp1 = (gRandomSeed16 & 0x00FF) << 8;
    temp1 = temp1 ^ gRandomSeed16;

    gRandomSeed16 = ((temp1 & 0x00FF) << 8) + ((temp1 & 0xFF00) >> 8);

    temp1 = ((temp1 & 0x00FF) << 1) ^ gRandomSeed16;
    temp2 = (temp1 >> 1) ^ 0xFF80;

    if ((temp1 & 1) == 0) {
        if (temp2 == 43605) {
            gRandomSeed16 = 0;
        } else {
            gRandomSeed16 = temp2 ^ 0x1FF4;
        }
    } else {
        gRandomSeed16 = temp2 ^ 0x8180;
    }

    return gRandomSeed16;
}

// Generate a pseudorandom float in the range [0, 1).
float random_float(void) {
    float rnd = random_u16();
    return rnd / (float) 0x10000;
}