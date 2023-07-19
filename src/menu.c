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

#define NUM_MENU_PREVS 4

char gMenuStatus = MENU_TITLE;
char gMenuPrev[NUM_MENU_PREVS];
char gIsPal = false;
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

void edit_menu_text(char *text, int index, unsigned int colour, int flagsOn, int flagsOff) {
    MenuListEntry *list = sMenuDisplay->list;
    while (index > 0 && list) {
        list = list->next;
        index--;
    }
    free(list->text);
    int textLen = strlen(text);
    list->text = malloc(textLen);
    strcpy(list->text, text);
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

    rdpq_font_begin(RGBA32(255, 255, 255, 255));
    int x = sMenuDisplay->x;
    int y = sMenuDisplay->y;
    while (list != NULL) {
        if (i == sMenuSelection[1]) {
            int sineCol = 128 + (32 * sins(gGameTimer * 0x400));
            rdpq_set_prim_color(RGBA32(255, sineCol, sineCol, 255));
        } else {
            rdpq_set_prim_color(RGBA32(list->colour[0], list->colour[1], list->colour[2], list->colour[3]));
        }
        rdpq_font_position(x, y);
        rdpq_font_print(gFonts[FONT_MVBOLI], list->text);
        y += 12;
        i++;
        list = list->next;
    }
    rdpq_font_end();
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

void render_menu_config(int updateRate, float updateRateF) {
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
        if (i == sMenuSelection[1]) {
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
            rdpq_font_printf(gFonts[FONT_MVBOLI], "%s: %s", m->name, sMenuOptionStrings[stringOffset]);
        } else if (m->flags & OPTION_BAR) {
            rdpq_font_printf(gFonts[FONT_MVBOLI], "%s: %d", m->name, *m->valuePtr);
        } else {
            rdpq_font_printf(gFonts[FONT_MVBOLI], "%s: %d", m->name, *m->valuePtr);
        }
        posY += 12;
    }
    rdpq_font_end();
}

void handle_menu_stick_input(int updateRate, int flags, short *selectionX, short *selectionY,  int minX, int minY, int maxX, int maxY) {
    int stickMag;

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
                        *selectionX = *selectionX - 1;
                    } else {
                        if (flags & MENUSTICK_WRAPX) {
                            *selectionX = maxX;
                        }
                    }
                } else {
                    if (*selectionX < maxX) {
                        *selectionX = *selectionX + 1;
                    } else {
                        if (flags & MENUSTICK_WRAPX) {
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
                        *selectionY = *selectionY - 1;
                    } else {
                        if (flags & MENUSTICK_WRAPY) {
                            *selectionY = maxY - 1;
                        }
                    }
                } else {
                    if (*selectionY < maxY - 1) {
                        *selectionY = *selectionY + 1;
                    } else {
                        if (flags & MENUSTICK_WRAPY) {
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
}

void process_config_menu(int updateRate) {
    int xWrap = 0;
    MenuOption *m = &sMenuOptions[sMenuSelection[1]];
    if (m->flags & OPTION_WRAP) {
        xWrap = MENUSTICK_WRAPX;
    }
    int prevMenuSelection = sMenuSelection[1];
    short tempVar = *m->valuePtr;
    handle_menu_stick_input(updateRate, MENUSTICK_STICKX | MENUSTICK_STICKY | MENUSTICK_WRAPY | xWrap, &tempVar, &sMenuSelection[1], m->minValue, 0, m->maxValue, sizeof(sMenuOptions) / sizeof(MenuOption));
    if (tempVar != *m->valuePtr) {
        *m->valuePtr = tempVar;
        if (m->func) {
            (m->func)();
        }
    }
    if (prevMenuSelection != sMenuSelection[1]) {
        if (prevMenuSelection < sMenuSelection[1]) {
            while (sMenuSelection[1] <= (sizeof(sMenuOptions) / sizeof(MenuOption) -1) && (sMenuOptions[sMenuSelection[1]].flags & OPTION_STUB || (sMenuOptions[sMenuSelection[1]].flags & OPTION_PAL_ONLY && gIsPal == false))) {
                sMenuSelection[1]++;
            }
            if (sMenuSelection[1] == -1) {
                sMenuSelection[1] = (sizeof(sMenuOptions) / sizeof(MenuOption)) -1;
            }
        } else {
            while (sMenuSelection[1] > 0 && (sMenuOptions[sMenuSelection[1]].flags & OPTION_STUB || (sMenuOptions[sMenuSelection[1]].flags & OPTION_PAL_ONLY && gIsPal == false))) {
                sMenuSelection[1]--;
            }
            if (sMenuSelection[1] >= sizeof(sMenuOptions) / sizeof(MenuOption)) {
                sMenuSelection[1] = 0;
            }
        }
    }
}

void process_options_menu(int updateRate) {

    if (sMenuDisplay == NULL) {
        init_menu_display(32, display_get_height() - 80);
        add_menu_text("Continue", 0, 0xFFFFFFFF, 0);
        add_menu_text("Options", 1, 0xFFFFFFFF, 0);
        add_menu_text("Quit", 2, 0xFFFFFFFF, 0);
    }

    handle_menu_stick_input(updateRate, MENUSTICK_STICKY, NULL, &sMenuSelection[1], 0, 0, 0, 3);

    if (get_input_pressed(INPUT_A, 5)) {
        clear_input(INPUT_A);
        switch (sMenuSelection[1]) {
        case 0:
            menu_set_backward(MENU_PREV);
            break;
        case 1:
            menu_set_forward(MENU_CONFIG);
            break;
        case 2:
            menu_set_reset(MENU_TITLE);
            load_scene(0);
            break;
        }
    }

    if ((get_input_pressed(INPUT_START, 3) || get_input_pressed(INPUT_B, 3)) && sMenuSwapTimer == 0) {
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
        clear_input(INPUT_A);
        switch (sMenuSelection[1]) {
        case 0:
            menu_set_reset(MENU_CLOSED);
            load_scene(1);
            break;
        case 1:
            menu_set_forward(MENU_CONFIG);
            sMenuSwapTimer = 30;
            break;
        }
    }
}

void process_menus(int updateRate, float updateRateF) {
    DECREASE_VAR(sMenuSwapTimer, updateRate, 0);
    switch (gMenuStatus) {
    case MENU_CLOSED:
        if (get_input_pressed(INPUT_START, 3) && sMenuSwapTimer == 0) {
            clear_input(INPUT_START);
            menu_set_forward(MENU_OPTIONS);
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
            clear_input(INPUT_START);
            clear_input(INPUT_B);
            menu_set_backward(MENU_PREV);
        }
        return;
    }
}

void render_menus(int updateRate, float updateRateF) {
    switch (gMenuStatus) {
    case MENU_CLOSED:
        return;
    case MENU_CONFIG:
        render_menu_config(updateRate, updateRateF);
        break;
    }
    render_menu_list();
}