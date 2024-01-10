#include <libdragon.h>
#include <malloc.h>
#include <string.h>

#include "menu.h"
#include "../include/global.h"

#include "input.h"
#include "main.h"
#include "audio.h"
#include "math_util.h"
#include "scene.h"
#include "save.h"
#include "debug.h"
#include "talk.h"
#include "hud.h"
#include "camera.h"

#define NUM_MENU_PREVS 4

char gMenuStatus = MENU_TITLE;
char gMenuPrev[NUM_MENU_PREVS];
char gIsPal = false;
char sNumOptions = 0;
static char sMenuSwapTimer = 0;
static short sMenuSelection[2];
static short sMenuSelectionPrev[NUM_MENU_PREVS][2];
static char sMenuSelectionTimer[2] = {0, 0};
static char sMenuSelectionType[2] = {0, 0};
static unsigned char sMenuStackPos = 0;
static MenuListRoot *sMenuDisplay = NULL;

void free_menu_display(void) {
    if (sMenuDisplay) {
        if (sMenuDisplay->list) {
            MenuListEntry *curList = sMenuDisplay->tail;
            MenuListEntry *prev = sMenuDisplay->tail;
            while (prev) {
                prev = curList->prev;
                free(curList->text);
                free(curList);
                curList = prev;
            }
        }
        free(sMenuDisplay);
        sMenuDisplay = NULL;
    }

    if (gMenuStatus == MENU_CLOSED) {
        if (gScreenshotStatus == SCREENSHOT_SHOW) {
            screenshot_clear();
        }
    }
}

void init_menu_display(int x, int y) {
    if (sMenuDisplay == NULL) {
        sMenuDisplay = malloc(sizeof(MenuListRoot));
    }

    sMenuDisplay->x = x;
    sMenuDisplay->y = y;
    sMenuDisplay->tail = NULL;
    sMenuDisplay->list = NULL;
    sMenuDisplay->listCount = 0;
}

void add_menu_text(char *text, int index, unsigned int colour, int flags) {
    MenuListEntry *newList;
    newList = malloc(sizeof(MenuListEntry));
    if (sMenuDisplay->list != NULL) {
        MenuListEntry *list = sMenuDisplay->list;
        while (index > 0 && list) {
            list = list->next;
            index--;
        }
        if (list == NULL) {
            list = newList;
            sMenuDisplay->tail->next = newList;
            newList->prev = sMenuDisplay->tail;
            sMenuDisplay->tail = newList;
            newList->next = NULL;
        } else {
            if (list->next != NULL) {
                list->next->prev = newList;
                newList->next = list->next;
            } else {
                newList->next = NULL;
            }
            list->next = newList;
            newList->prev = list;
            if (sMenuDisplay->tail == list) {
                sMenuDisplay->tail = newList;
            }
        }
    } else {
        sMenuDisplay->list = newList;
        sMenuDisplay->tail = newList;
        newList->next = NULL;
        newList->prev = NULL;
    }
    int textLen = strlen(text);
    newList->text = malloc(textLen);
    sprintf(newList->text, "%s", text);
    newList->colour[0] = (colour >> 24) & 0xFF;
    newList->colour[1] = (colour >> 16) & 0xFF;
    newList->colour[2] = (colour >> 8) & 0xFF;
    newList->colour[3] = colour & 0xFF;
    newList->flags = flags;
    sMenuDisplay->listCount++;
}

void edit_menu_text(char *text, int index) {
    MenuListEntry *list = sMenuDisplay->list;
    while (index > 0 && list) {
        list = list->next;
        index--;
    }
    free(list->text);
    int textLen = strlen(text);
    list->text = malloc(textLen);
    strcpy(list->text, text);
}

void edit_menu_style(int index, unsigned int colour, int flagsOn, int flagsOff) {
    MenuListEntry *list = sMenuDisplay->list;
    while (index > 0 && list) {
        list = list->next;
        index--;
    }
    list->colour[0] = (colour >> 24) & 0xFF;
    list->colour[1] = (colour >> 16) & 0xFF;
    list->colour[2] = (colour >> 8) & 0xFF;
    list->colour[3] = colour & 0xFF;
    list->flags |= flagsOn;
    list->flags &= flagsOff;
}

void render_menu_list(void) {
    if (sMenuDisplay == NULL) {
        return;
    }
    int i = 0;
    MenuListEntry *list = sMenuDisplay->list;

    int x = sMenuDisplay->x;
    int y = sMenuDisplay->y;
    while (list != NULL) {
        if (i == sMenuSelection[1]) {
            int sineCol = 128 + (32 * sins(gGameTimer * 0x400));
            rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(255, sineCol, sineCol, 255),});
        } else {
            rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(list->colour[0], list->colour[1], list->colour[2], list->colour[3]),});
        }
        rdpq_text_printf(NULL, FONT_MVBOLI, x, y, list->text);
        y += 12;
        i++;
        list = list->next;
    }
}

void menu_set_forward(int menuID) {
    sMenuSelectionPrev[sMenuStackPos][0] = sMenuSelection[0];
    sMenuSelectionPrev[sMenuStackPos][1] = sMenuSelection[1];
    sMenuSelection[0] = 0;
    sMenuSelection[1] = 0;
    sMenuSelectionTimer[0] = 0;
    sMenuSelectionTimer[1] = 0;
    sMenuSelectionType[0] = 0;
    sMenuSelectionType[1] = 0;
    gMenuPrev[sMenuStackPos] = gMenuStatus;
    sMenuSwapTimer = 30;
    gMenuStatus = menuID;
    sMenuStackPos++;
    free_menu_display();
}

void menu_set_reset(int menuID) {
    sMenuSelection[0] = 0;
    sMenuSelection[1] = 0;
    sMenuSelectionTimer[0] = 0;
    sMenuSelectionTimer[1] = 0;
    sMenuSelectionType[0] = 0;
    sMenuSelectionType[1] = 0;
    sMenuSwapTimer = 30;
    gMenuStatus = menuID;
    sMenuStackPos = 0;
    free_menu_display();
}

void menu_set_backward(int menuID) {
    sMenuStackPos--;
    sMenuSelection[0] = sMenuSelectionPrev[sMenuStackPos][0];
    sMenuSelection[1] = sMenuSelectionPrev[sMenuStackPos][1];
    sMenuSelectionTimer[0] = 0;
    sMenuSelectionTimer[1] = 0;
    sMenuSelectionType[0] = 0;
    sMenuSelectionType[1] = 0;
    sMenuSwapTimer = 30;
    if (menuID == MENU_PREV) {
        gMenuStatus = gMenuPrev[sMenuStackPos];
    } else {
        gMenuStatus = menuID;
    }
    free_menu_display();
}

void menu_reset_display(void) {
    gResetDisplay = true;
}

extern int __boot_tvtype;

void menu_change_pal60(void) {
    if (gConfig.regionMode == PAL60) {
        gConfig.regionMode = NTSC60;
    }
    __boot_tvtype = gConfig.regionMode; // Writes to osTvType
    gResetDisplay = true;
}

void menu_set_sound(void) {
    gMusicVolume = (float) gConfig.musicVolume / (float) 9.0f;
    gSoundVolume = (float) gConfig.soundVolume / (float) 9.0f;
    set_music_volume(gMusicVolume);
}

static MenuOption sMenuOptions[] = {
    {"Graphics", &gConfig.graphics, -1, 1, OPTION_WRAP | OPTION_STRING, NULL, 0},
    {"Screen Mode", &gConfig.screenMode, 0, 2, OPTION_WRAP | OPTION_STRING | OPTION_STUB, menu_reset_display, 5},
    {"PAL", &gConfig.regionMode, 0, 1, OPTION_WRAP | OPTION_PAL_ONLY | OPTION_STRING, menu_change_pal60, 8},
    {"Sound Mode", &gConfig.soundMode, 0, 2, OPTION_WRAP | OPTION_STRING, NULL, 10},
    {"Screen Pos X", &gConfig.screenPosX, -8, 8, OPTION_STUB | OPTION_BAR, NULL, 0},
    {"Screen Pos Y", &gConfig.screenPosY, -8, 8, OPTION_STUB | OPTION_BAR, NULL, 0},
    {"Sound Volume", &gConfig.soundVolume, 0, 9, OPTION_BAR, menu_set_sound, 0},
    {"Music Volume", &gConfig.musicVolume, 0, 9, OPTION_BAR, menu_set_sound, 0},
    {"Frame Cap", &gConfig.frameCap, 0, 1, OPTION_STRING | OPTION_PAL_OFFSET | OPTION_STUB, NULL, 13},
};

static char *sMenuOptionStrings[] = {
    "Performance",
    "Default",
    "Beautiful",
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

void handle_menu_stick_input(int updateRate, int flags, short *selectionX, short *selectionY,  int minX, int minY, int maxX, int maxY) {
    int stickMag;
    int playSound = false;

    if (flags & MENUSTICK_STICKX) {
        stickMag = get_stick_x(STICK_LEFT);
        DECREASE_VAR(sMenuSelectionTimer[0], updateRate, 0);
        if (fabs(stickMag) > 25 || get_input_held(INPUT_DLEFT) || get_input_held(INPUT_DRIGHT)) {
            if (sMenuSelectionTimer[0] == 0) {
                if (sMenuSelectionType[0] == 0) {
                    sMenuSelectionTimer[0] = 30;
                    sMenuSelectionType[0] = 1;
                } else {
                    sMenuSelectionTimer[0] = 10;
                }
                if (stickMag < 0 || get_input_held(INPUT_DLEFT)) {
                    if (*selectionX > minX) {
                        playSound = true;
                        *selectionX = *selectionX - 1;
                    } else {
                        if (flags & MENUSTICK_WRAPX) {
                            playSound = true;
                            *selectionX = maxX;
                        }
                    }
                } else {
                    if (*selectionX < maxX) {
                        playSound = true;
                        *selectionX = *selectionX + 1;
                    } else {
                        if (flags & MENUSTICK_WRAPX) {
                            playSound = true;
                            *selectionX = minX;
                        }
                    }
                }
            }
        } else {
            sMenuSelectionType[0] = 0;
            sMenuSelectionTimer[0] = 0;
        }
    }
    if (flags & MENUSTICK_STICKY) {
        stickMag = get_stick_y(STICK_LEFT);
        DECREASE_VAR(sMenuSelectionTimer[1], updateRate, 0);
        if (fabs(stickMag) > 25 || get_input_held(INPUT_DUP) || get_input_held(INPUT_DDOWN)) {
            if (sMenuSelectionTimer[1] == 0) {
                if (sMenuSelectionType[1] == 0) {
                    sMenuSelectionTimer[1] = 30;
                    sMenuSelectionType[1] = 1;
                } else {
                    sMenuSelectionTimer[1] = 10;
                }
                if (stickMag > 0 || get_input_held(INPUT_DUP)) {
                    if (*selectionY > minY) {
                        playSound = true;
                        *selectionY = *selectionY - 1;
                    } else {
                        if (flags & MENUSTICK_WRAPY) {
                            playSound = true;
                            *selectionY = maxY - 1;
                        }
                    }
                } else {
                    if (*selectionY < maxY - 1) {
                        playSound = true;
                        *selectionY = *selectionY + 1;
                    } else {
                        if (flags & MENUSTICK_WRAPY) {
                            playSound = true;
                            *selectionY = minY;
                        }
                    }
                }
            }
        } else {
            sMenuSelectionType[1] = 0;
            sMenuSelectionTimer[1] = 0;
        }
    }
    if (playSound) {
        play_sound_global(SOUND_SHELL1);
    }
}

char sMenuTextStack[40];

char *set_option_text(int optID) {
    if (sMenuOptions[optID].flags & OPTION_STRING) {
        int stringOffset = sMenuOptions[optID].string + (*sMenuOptions[optID].valuePtr - sMenuOptions[optID].minValue);
        if (gConfig.regionMode == PAL50 && sMenuOptions[optID].flags & OPTION_PAL_OFFSET) {
            stringOffset += (sMenuOptions[optID].maxValue - sMenuOptions[optID].minValue) + 1;
        }
        sprintf(sMenuTextStack, "%s: %s", sMenuOptions[optID].name, sMenuOptionStrings[stringOffset]);
    } else if (sMenuOptions[optID].flags & OPTION_BAR) {
        sprintf(sMenuTextStack, "%s: %d", sMenuOptions[optID].name, *sMenuOptions[optID].valuePtr);
    } else {
        sprintf(sMenuTextStack, "%s: %d", sMenuOptions[optID].name, *sMenuOptions[optID].valuePtr);
    }
    return sMenuTextStack;
}

void process_config_menu(int updateRate) {
    int xWrap = 0;
    MenuOption *m = &sMenuOptions[sMenuSelection[1]];
    if (m->flags & OPTION_WRAP) {
        xWrap = MENUSTICK_WRAPX;
    }

    if (sMenuDisplay == NULL) {
        sNumOptions = 0;
        init_menu_display(16, 22);
        for (int i = 0; i < sizeof(sMenuOptions) / sizeof(MenuOption); i++) {
            int colour;
            if (sMenuOptions[i].flags & OPTION_STUB || (sMenuOptions[i].flags & OPTION_PAL_ONLY && gIsPal == false)) {
                colour = 0x80808080;
            } else {
                colour = 0xFFFFFFFF;
            }
            
            add_menu_text(set_option_text(i), sNumOptions, colour, 0);
            sNumOptions++;
        }
    }

    short tempVar = *m->valuePtr;
    int prevMenuSelection = sMenuSelection[1];
    handle_menu_stick_input(updateRate, MENUSTICK_STICKX | MENUSTICK_STICKY | MENUSTICK_WRAPY | xWrap, &tempVar, &sMenuSelection[1], m->minValue, -1, m->maxValue, sizeof(sMenuOptions) / sizeof(MenuOption));
    if (tempVar != *m->valuePtr) {
        *m->valuePtr = tempVar;
        edit_menu_text(set_option_text(sMenuSelection[1]), sMenuSelection[1]);
        if (m->func) {
            (m->func)();
        }
    }
    if (prevMenuSelection != sMenuSelection[1]) {
        if (prevMenuSelection < sMenuSelection[1]) {
            while (sMenuSelection[1] <= (sizeof(sMenuOptions) / sizeof(MenuOption) -1) && (sMenuOptions[sMenuSelection[1]].flags & OPTION_STUB || (sMenuOptions[sMenuSelection[1]].flags & OPTION_PAL_ONLY && gIsPal == false))) {
                sMenuSelection[1]++;
            }
            if (sMenuSelection[1] >= sizeof(sMenuOptions) / sizeof(MenuOption)) {
                sMenuSelection[1] = 0;
            }
        } else {
            while (sMenuSelection[1] > -1 && (sMenuOptions[sMenuSelection[1]].flags & OPTION_STUB || (sMenuOptions[sMenuSelection[1]].flags & OPTION_PAL_ONLY && gIsPal == false))) {
                sMenuSelection[1]--;
            }
            if (sMenuSelection[1] < 0) {
                sMenuSelection[1] = (sizeof(sMenuOptions) / sizeof(MenuOption)) -1;
            }
        }
    }
}

void process_options_menu(int updateRate) {
    if (sMenuDisplay == NULL) {
        init_menu_display(32, display_get_height() - 80);
        add_menu_text("Continue", 0, 0xFFFFFFFF, 0);
        add_menu_text("Options", 1, 0xFFFFFFFF, 0);
        add_menu_text("Photo Mode", 2, 0xFFFFFFFF, 0);
        add_menu_text("Quit", 3, 0xFFFFFFFF, 0);
    }

    handle_menu_stick_input(updateRate, MENUSTICK_STICKY, NULL, &sMenuSelection[1], 0, 0, 0, 4);

    if (get_input_pressed(INPUT_A, 5)) {
        play_sound_global(SOUND_MENU_CLICK);
        clear_input(INPUT_A);
        switch (sMenuSelection[1]) {
        case 0:
            menu_set_backward(MENU_PREV);
            break;
        case 1:
            menu_set_forward(MENU_CONFIG);
            screenshot_clear();
            break;
        case 2:
            menu_set_backward(MENU_PREV);
            gCamera->mode = CAMERA_PHOTO;
            gCamera->pitch = -0xA00;
            gCamera->yaw -= 0x4000;
            screenshot_clear();
            break;
        case 3:
            menu_set_reset(MENU_TITLE);
            transition_into_scene(SCENE_INTRO, TRANSITION_FULLSCREEN_IN, 30, TRANSITION_FULLSCREEN_OUT);
            set_background_music(1, 30);
            break;
        }
    }

    if ((get_input_pressed(INPUT_START, 3) || get_input_pressed(INPUT_B, 3)) && sMenuSwapTimer == 0) {
        play_sound_global(SOUND_MENU_CLICK);
        clear_input(INPUT_START);
        clear_input(INPUT_B);
        menu_set_backward(MENU_PREV);
    }
}

void process_title_menu(int updateRate) {
    if (gCurrentController == -1) {
        return;
    }

    if (sMenuDisplay == NULL) {
        init_menu_display(32, display_get_height() - 80);
        add_menu_text("Play", 0, 0xFFFFFFFF, 0);
        add_menu_text("Options", 1, 0xFFFFFFFF, 0);
    }

    handle_menu_stick_input(updateRate, MENUSTICK_STICKY, NULL, &sMenuSelection[1], 0, 0, 0, 2);

    if (get_input_pressed(INPUT_A, 3) && sMenuSwapTimer == 0) {
        play_sound_global(SOUND_MENU_CLICK);
        clear_input(INPUT_A);
        switch (sMenuSelection[1]) {
        case 0:
            menu_set_reset(MENU_CLOSED);
            transition_into_scene(SCENE_TESTAREA, TRANSITION_FULLSCREEN_IN, 30, TRANSITION_FULLSCREEN_OUT);
            set_background_music(0, 30);
            break;
        case 1:
            menu_set_forward(MENU_CONFIG);
            sMenuSwapTimer = 30;
            break;
        }
    }
}

void process_menus(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    DECREASE_VAR(sMenuSwapTimer, updateRate, 0);
    switch (gMenuStatus) {
    case MENU_CLOSED:
        if (sMenuSwapTimer == 0 && gTalkControl == NULL && get_input_pressed(INPUT_START, 3)) {
            if (gCamera->mode == CAMERA_PHOTO) {
                gCamera->mode = CAMERA_TARGET;
                gCamera->fov = 50.0f;
                gCamera->pitch = 0x3400;
                gCamera->yaw = gCamera->yawTarget;
            } else {
                menu_set_forward(MENU_OPTIONS);
                screenshot_on(FMT_I8);
            }
            play_sound_global(SOUND_MENU_CLICK);
            clear_input(INPUT_START);
        }
        return;
    case MENU_TITLE:
        process_title_menu(updateRate);
        return;
    case MENU_OPTIONS:
        process_options_menu(updateRate);
        return;
    case MENU_CONFIG:
        process_config_menu(updateRate);
        if ((get_input_pressed(INPUT_START, 3) || get_input_pressed(INPUT_B, 3)) && sMenuSwapTimer == 0) {
            play_sound_global(SOUND_MENU_CLICK);
            clear_input(INPUT_START);
            clear_input(INPUT_B);
            menu_set_backward(MENU_PREV);
            if (gMenuStatus == MENU_OPTIONS) {
                screenshot_on(FMT_I8);
            }
            write_config();
        }
        return;
    }
    get_time_snapshot(PP_MENU, DEBUG_SNAPSHOT_1_END);
}

void render_menus(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    switch (gMenuStatus) {
    case MENU_CLOSED:
        return;
    case MENU_CONFIG:
        rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(255, 255, 255, 255),});
        rdpq_text_printf(NULL, FONT_MVBOLI, display_get_width() - 80, 16, "FPS: %2.2f", (double) display_get_fps());
        break;
    }
    render_menu_list();
    get_time_snapshot(PP_MENU, DEBUG_SNAPSHOT_1_END);
}