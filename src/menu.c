#include <libdragon.h>

#include "menu.h"
#include "../include/global.h"

#include "input.h"
#include "main.h"
#include "audio.h"

char gMenuStatus = 0;
static char sMenuSwapTimer = 0;
char gIsPal = false;
static short gMenuSelection[2];

void menu_reset_display(void) {
    reset_display();
}

void menu_change_pal60(void) {
    if (gConfig.regionMode == PAL60) {
        gConfig.regionMode = NTSC60;
    }
    *(uint32_t*) 0x80000300 = gConfig.regionMode; // Writes to osTvType
    gResetDisplay = true;
}

void menu_set_sound(void) {
    sMusicVolume = (float) gConfig.musicVolume / (float) 9.0f;
    sSoundVolume = (float) gConfig.soundVolume / (float) 9.0f;
    set_music_volume(sMusicVolume);
}

static MenuOption sMenuOptions[] = {
    {"Anti Aliasing", &gConfig.antiAliasing, -1, 1, OPTION_WRAP | OPTION_STRING, NULL, 0},
    {"Dedither", &gConfig.dedither, 0, 1, OPTION_WRAP | OPTION_STRING, NULL, 3},
    {"Screen Mode", &gConfig.screenMode, 0, 2, OPTION_WRAP | OPTION_STRING | OPTION_STUB, NULL, 5},
    {"PAL", &gConfig.regionMode, 0, 1, OPTION_WRAP | OPTION_PAL_ONLY | OPTION_STRING, menu_change_pal60, 8},
    {"Sound Mode", &gConfig.soundMode, 0, 2, OPTION_WRAP | OPTION_STRING, NULL, 10},
    {"Screen Pos X", &gConfig.screenPosX, -8, 8, OPTION_STUB | OPTION_BAR, NULL, 0},
    {"Screen Pos Y", &gConfig.screenPosY, -8, 8, OPTION_STUB | OPTION_BAR, NULL, 0},
    {"Sound Volume", &gConfig.soundVolume, 0, 9, OPTION_BAR, menu_set_sound, 0},
    {"Music Volume", &gConfig.musicVolume, 0, 9, OPTION_BAR, menu_set_sound, 0},
    {"Frame Cap", &gConfig.frameCap, 0, 1, OPTION_STRING | OPTION_PAL_OFFSET | OPTION_STUB, NULL, 13},
};

static char *sMenuOptionStrings[] = {
    "Off",
    "Fast",
    "Fancy",
    "Off",
    "On",
    "4:3",
    "16:10",
    "16:9",
    "50Hz",
    "60Hz",
    "Mono",
    "Stereo",
    "Surround",
    "60",
    "30",
    "50",
    "25"
};

void render_menu_options(int updateRate, float updateRateF) {
    int posY;
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(RGBA32(0, 0, 0, 96));
    rdpq_fill_rectangle(0, 0, 128, display_get_height());
    rdpq_font_begin(RGBA32(255, 255, 255, 255));
    posY = 22;
    for (int i = 0; i < sizeof(sMenuOptions) / sizeof(MenuOption); i++) {
        MenuOption *m = &sMenuOptions[i];
        if (m->flags & OPTION_STUB || (m->flags & OPTION_PAL_ONLY && gIsPal == false)) {
            continue;
        }
        if (i == gMenuSelection[1]) {
            rdpq_set_prim_color(RGBA32(255, 0, 0, 255));
        } else {
            rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
        }
        rdpq_font_position(16, posY);
        if (m->flags & OPTION_STRING) {
            int stringOffset = m->string + (*m->valuePtr - m->minValue);
            if (gConfig.regionMode == PAL50 && m->flags & OPTION_PAL_OFFSET) {
                stringOffset += (m->maxValue - m->minValue) + 1;
            }
            rdpq_font_printf(gCurrentFont, "%s: %s", m->name, sMenuOptionStrings[stringOffset]);
        } else if (m->flags & OPTION_BAR) {
            rdpq_font_printf(gCurrentFont, "%s: %d", m->name, *m->valuePtr);
        } else {
            rdpq_font_printf(gCurrentFont, "%s: %d", m->name, *m->valuePtr);
        }
        posY += 12;
    }
    rdpq_font_end();
}

void process_option_menu(int updateRate) {
    if (get_input_pressed(INPUT_DUP, 3)) {
        gMenuSelection[1]--;
        while (gMenuSelection[1] > -1 && (sMenuOptions[gMenuSelection[1]].flags & OPTION_STUB || (sMenuOptions[gMenuSelection[1]].flags & OPTION_PAL_ONLY && gIsPal == false))) {
            gMenuSelection[1]--;
        }
        if (gMenuSelection[1] == -1) {
            gMenuSelection[1] = (sizeof(sMenuOptions) / sizeof(MenuOption)) -1;
        }
        clear_input(INPUT_DUP);
    } else if (get_input_pressed(INPUT_DDOWN, 3)) {
        gMenuSelection[1]++;
        while (gMenuSelection[1] <= (sizeof(sMenuOptions) / sizeof(MenuOption) -1) && (sMenuOptions[gMenuSelection[1]].flags & OPTION_STUB || (sMenuOptions[gMenuSelection[1]].flags & OPTION_PAL_ONLY && gIsPal == false))) {
            gMenuSelection[1]++;
        }
        if (gMenuSelection[1] >= sizeof(sMenuOptions) / sizeof(MenuOption)) {
            gMenuSelection[1] = 0;
        }
        clear_input(INPUT_DDOWN);
    }

    if (get_input_pressed(INPUT_DLEFT, 3)) {
        MenuOption *m = &sMenuOptions[gMenuSelection[1]];
        clear_input(INPUT_DLEFT);
        *m->valuePtr = *m->valuePtr - 1;
        if (*m->valuePtr < m->minValue) {
            if (m->flags & OPTION_WRAP) {
                *m->valuePtr = m->maxValue;
            } else {
                *m->valuePtr = m->minValue;
            }
        }
        if (m->func) {
            (m->func)();
        }
    }
    if (get_input_pressed(INPUT_DRIGHT, 3)) {
        MenuOption *m = &sMenuOptions[gMenuSelection[1]];
        clear_input(INPUT_DRIGHT);
        *m->valuePtr = *m->valuePtr + 1;
        if (*m->valuePtr > m->maxValue) {
            if (m->flags & OPTION_WRAP) {
                *m->valuePtr = m->minValue;
            } else {
                *m->valuePtr = m->maxValue;
            }
        }
        if (m->func) {
            (m->func)();
        }
    }
}

void process_menus(int updateRate, float updateRateF) {
    switch (gMenuStatus) {
    case MENU_CLOSED:
        if (get_input_pressed(INPUT_START, 3) && sMenuSwapTimer == 0) {
            clear_input(INPUT_START);
            gMenuStatus = MENU_OPTIONS;
        }
        return;
    case MENU_OPTIONS:
        process_option_menu(updateRate);
        if (get_input_pressed(INPUT_START, 3) && sMenuSwapTimer == 0) {
            clear_input(INPUT_START);
            gMenuStatus = MENU_CLOSED;
        }
        return;
    }
}

void render_menus(int updateRate, float updateRateF) {
    switch (gMenuStatus) {
    case MENU_CLOSED:
        return;
    case MENU_OPTIONS:
        render_menu_options(updateRate, updateRateF);
        return;
    }
}