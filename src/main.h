#pragma once

enum LogicRates {
    LOGIC_60FPS = 1,
    LOGIC_30FPS,
    LOGIC_20FPS,
    LOGIC_15FPS
};

enum AntiAliasing {
    AA_OFF = -1,
    AA_FAST,
    AA_FANCY
};

enum RegionMode {
    PAL50,
    NTSC60,
    PALM,
    PAL60
};

enum ScreenMode {
    SCREEN_4_3,
    SCREEN_16_10,
    SCREEN_16_9
};

typedef struct Config {
    signed antiAliasing : 2;
    unsigned dedither : 1;
    unsigned regionMode : 2;
    unsigned screenMode : 2;
} Config;

extern surface_t gZBuffer;
extern surface_t *gFrameBuffers;
extern rdpq_font_t *gCurrentFont;
extern Config gConfig;
extern char gZTargetTimer;
extern unsigned int gGlobalTimer;
extern unsigned int gGameTimer;