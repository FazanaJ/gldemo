#pragma once

extern float gSineTable[];

#define gCosineTable (gSineTable + 0x400)
#define sins(x) gSineTable[(unsigned short) (x) >> 4]
#define coss(x) gCosineTable[(unsigned short) (x) >> 4]
#define SHORT_TO_DEGREES(x) (x / 182)
#define INCREASE_VAR(x, amt, max) {if (x + amt < max) x += amt; else x = max;}
#define DECREASE_VAR(x, amt, min) {if (x - amt > min) x -= amt; else x = min;}
#define SQR(x) (x * x)
#define DIST2(x, y) (SQR(x[0] - y[0]) + SQR(x[1] - y[1]))
#define DIST3(x, y) (SQR(fabs(x[0] - y[0])) + SQR(fabs(x[1] - y[1])) + SQR(fabs(x[2] - y[2])))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

int atan2s(float y, float x);
int timer_int(int timer);
float timer_float(float timer);
int lerp(int current, int target, float factor);
int lerp_short(int current, int target, float factor);
float lerpf(float current, float target, float factor);
int approach(int current, int target, int inc);
float approachf(float current, float target, float inc);
float random_float(void);
