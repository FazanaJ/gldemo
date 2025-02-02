#pragma once

#include "../include/global.h"

extern char gScreenshotStatus;
extern surface_t gScreenshot;
extern char gScreenshotType;

#define SCREENSHOT_SHOW -1
#define SCREENSHOT_NONE 0
#define SCREENSHOT_GENERATE 1

void screenshot_on(int type);
void screenshot_generate(void);
void screenshot_clear(void);