#pragma once

extern char gScreenshotStatus;
extern surface_t gScreenshot;
extern sprite_t *gScreenshotSprite;

#define SCREENSHOT_SHOW -1
#define SCREENSHOT_NONE 0
#define SCREENSHOT_GENERATE 1

enum TransitionType {
    TRANSITION_NONE,
    TRANSITION_FULLSCREEN_IN,
    TRANSITION_FULLSCREEN_OUT,

    TRANSITION_COUNT
};

void init_hud(void);
void process_hud(int updateRate, float updateRateF);
void render_hud(int updateRate, float updateRateF);
void add_subtitle(char *text, int timer);
void screenshot_generate(void);
void screenshot_clear(void);
void transition_clear(void);
void transition_set(int type, int timer);
void transition_into_scene(int sceneID, int transitionType, int timer, int transitionOut);