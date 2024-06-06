
#include <libdragon.h>
#include <malloc.h>

#include "screenshot.h"
#include "../include/global.h"

#include "main.h"

char gScreenshotStatus;
char gScreenshotType;
surface_t gScreenshot;

void screenshot_on(int type) {
    gScreenshotStatus = SCREENSHOT_GENERATE;
    gScreenshotType = type;
}

void screenshot_generate(void) {
    if (gScreenshot.buffer) {
        surface_free(&gScreenshot);
    }
    gScreenshot = surface_alloc(gScreenshotType, display_get_width(), display_get_height());
    rdpq_attach_clear(&gScreenshot, &gZBuffer);
    
}

void screenshot_clear(void) {
    surface_free(&gScreenshot);
    gScreenshotStatus = SCREENSHOT_NONE;
}