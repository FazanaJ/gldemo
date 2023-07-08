#pragma once

#include <math.h>

extern float gSineTable[];

#define gCosineTable (gSineTable + 0x400)
#define sins(x) gSineTable[(unsigned short) (x) >> 4]
#define coss(x) gCosineTable[(unsigned short) (x) >> 4]
#define SHORT_TO_DEGREES(x) (x / 182)
#define INCREASE_VAR(x, amt, max) {x += amt; if (x > max) x = max;}
#define DECREASE_VAR(x, amt, min) {x -= amt; if (x < min) x = min;}
#define SQR(x) (x * x)
#define DIST2(x, y) (SQR(x[0] - y[0]) + SQR(x[1] - y[1]))
#define DIST3(x, y) (SQR(x[0] - y[0]) + SQR(x[1] - y[1]) + SQR(x[2] - y[2]))

int atan2s(float y, float x);
int timer_int(int timer);
float timer_float(float timer);
int lerp(int current, int target, float factor);
int lerp_short(int current, int target, float factor);
float lerpf(float current, float target, float factor);