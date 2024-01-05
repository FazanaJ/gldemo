#pragma once

extern char gScreenshotStatus;
extern surface_t gScreenshot;
extern sprite_t *gScreenshotSprite;

#define SCREENSHOT_SHOW -1
#define SCREENSHOT_NONE 0
#define SCREENSHOT_GENERATE 1

void init_hud(void);
void process_hud(int updateRate, float updateRateF);
void render_hud(int updateRate, float updateRateF);
void add_subtitle(char *text, int timer);
void screenshot_generate(void);
void screenshot_clear(void);
