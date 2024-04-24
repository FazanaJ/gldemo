#pragma once

#include "../include/global.h"

enum LogicRates {
    LOGIC_60FPS = 1,
    LOGIC_30FPS,
    LOGIC_20FPS,
    LOGIC_15FPS
};

enum GraphicsSetting {
    G_PERFORMANCE = -1,
    G_DEFAULT,
    G_BEAUTIFUL
};

enum AntiAliasingNames {
    AA_GEO,
    AA_ACTOR
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

enum SoundMode {
    SOUND_MONO,
    SOUND_STEREO,
    SOUND_SURROUND
};

typedef struct Config {
    char graphics;
    char regionMode;
    char screenMode;
    char soundMode;
    char screenPosX;
    char screenPosY;
    char soundVolume;
    char musicVolume;
    char vsync;
    char subtitles;
    char language;
} Config;

typedef struct ConfigBits {
    signed graphics : 2;
    unsigned regionMode : 2;
    unsigned screenMode : 2;
    signed soundMode : 2;
    signed screenPosX : 5;
    signed screenPosY : 5;
    unsigned soundVolume : 4;
    unsigned musicVolume : 4;
    unsigned vsync : 1;
    unsigned magic : 6;
    unsigned subtitles : 1;
    unsigned language : 3;
} ConfigBits;

extern surface_t gZBuffer;
extern surface_t *gFrameBuffers;
extern Config gConfig;
extern char gZTargetTimer;
extern unsigned int gGlobalTimer;
extern unsigned int gGameTimer;
extern char gResetDisplay;

void reset_display(void);