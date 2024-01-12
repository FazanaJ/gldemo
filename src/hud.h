#pragma once

extern char gCameraHudToggle;

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
void transition_set(int type, int timer);
void transition_into_scene(int sceneID, int transitionType, int timer, int transitionOut);
void transition_clear(void);