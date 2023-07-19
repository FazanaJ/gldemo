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
    char antiAliasing;
    char dedither;
    char regionMode;
    char screenMode;
    char soundMode;
    char screenPosX;
    char screenPosY;
    char soundVolume;
    char musicVolume;
    char frameCap;
} Config;

extern surface_t gZBuffer;
extern surface_t *gFrameBuffers;
extern Config gConfig;
extern char gZTargetTimer;
extern unsigned int gGlobalTimer;
extern unsigned int gGameTimer;
extern char gResetDisplay;

void reset_display(void);