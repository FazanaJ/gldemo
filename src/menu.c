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
#include "screenshot.h"

#define NUM_MENU_PREVS 4

char gMenuStatus = MENU_TITLE;
char gMenuPrev[NUM_MENU_PREVS];
char gIsPal = false;
static char sMenuSwapTimer = 0;
static char sConfigMenu = false;
short gMenuSelection[2];
static short sMenuSelectionPrev[NUM_MENU_PREVS][2];
static char sMenuSelectionTimer[2] = {0, 0};
static char sMenuSelectionType[2] = {0, 0};
static unsigned char sMenuStackPos = 0;
MenuListRoot *gMenuDisplay = NULL;

void free_menu_display(void) {
    if (gMenuDisplay) {
        if (gMenuDisplay->list) {
            MenuListEntry *curList = gMenuDisplay->tail;
            MenuListEntry *prev = gMenuDisplay->tail;
            while (prev) {
                prev = curList->prev;
                free(curList->text);
                free(curList);
                curList = prev;
            }
        }
        free(gMenuDisplay);
        gMenuDisplay = NULL;
    }

    if (gMenuStatus == MENU_CLOSED) {
        if (gScreenshotStatus == SCREENSHOT_SHOW) {
            screenshot_clear();
        }
    }
}

void init_menu_display(int x, int y) {
    if (gMenuDisplay == NULL) {
        gMenuDisplay = malloc(sizeof(MenuListRoot));
    } else {
        assertf(gMenuDisplay == NULL, "gMenuDisplay already exists.");
    }

    gMenuDisplay->x = x;
    gMenuDisplay->y = y;
    gMenuDisplay->tail = NULL;
    gMenuDisplay->list = NULL;
    gMenuDisplay->listCount = 0;
}

void add_menu_text(char *text, int index, unsigned int colour, int flags) {
    MenuListEntry *newList;
    newList = malloc(sizeof(MenuListEntry));
    if (gMenuDisplay->list != NULL) {
        MenuListEntry *list = gMenuDisplay->list;
        while (index > 0 && list) {
            list = list->next;
            index--;
        }
        if (list == NULL) {
            list = newList;
            gMenuDisplay->tail->next = newList;
            newList->prev = gMenuDisplay->tail;
            gMenuDisplay->tail = newList;
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
            if (gMenuDisplay->tail == list) {
                gMenuDisplay->tail = newList;
            }
        }
    } else {
        gMenuDisplay->list = newList;
        gMenuDisplay->tail = newList;
        newList->next = NULL;
        newList->prev = NULL;
    }
    int textLen = strlen(text);
    newList->text = malloc(textLen + 1);
    sprintf(newList->text, "%s", text);
    newList->colour[0] = (colour >> 24) & 0xFF;
    newList->colour[1] = (colour >> 16) & 0xFF;
    newList->colour[2] = (colour >> 8) & 0xFF;
    newList->colour[3] = colour & 0xFF;
    newList->flags = flags;
    gMenuDisplay->listCount++;
}

void edit_menu_text(char *text, int index) {
    MenuListEntry *list = gMenuDisplay->list;
    while (index > 0 && list) {
        list = list->next;
        index--;
    }
    free(list->text);
    int textLen = strlen(text);
    list->text = malloc(textLen);
    strcpy(list->text, text);
}

static void edit_menu_style(int index, unsigned int colour, int flagsOn, int flagsOff) {
    MenuListEntry *list = gMenuDisplay->list;
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

static void render_menu_list(void) {
    if (gMenuDisplay == NULL) {
        return;
    }
    int i = 0;
    MenuListEntry *list = gMenuDisplay->list;

    int x = gMenuDisplay->x;
    int y = gMenuDisplay->y;
    while (list != NULL) {
        rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(0, 0, 0, 255),});
        rdpq_text_printf(NULL, FONT_MVBOLI, x + 1, y + 1, list->text);
        if (i == gMenuSelection[1]) {
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

static void menu_set_forward(int menuID) {
    sMenuSelectionPrev[sMenuStackPos][0] = gMenuSelection[0];
    sMenuSelectionPrev[sMenuStackPos][1] = gMenuSelection[1];
    gMenuSelection[0] = 0;
    gMenuSelection[1] = 0;
    sMenuSelectionTimer[0] = 0;
    sMenuSelectionTimer[1] = 0;
    sMenuSelectionType[0] = 0;
    sMenuSelectionType[1] = 0;
    gMenuPrev[sMenuStackPos] = gMenuStatus;
    sMenuSwapTimer = 30;
    gMenuStatus = menuID;
    sMenuStackPos++;
    assertf(sMenuStackPos < NUM_MENU_PREVS, "sMenuStackPos exceeded limit.\n Limit: %d", NUM_MENU_PREVS);
    free_menu_display();
}

static void menu_set_reset(int menuID) {
    gMenuSelection[0] = 0;
    gMenuSelection[1] = 0;
    sMenuSelectionTimer[0] = 0;
    sMenuSelectionTimer[1] = 0;
    sMenuSelectionType[0] = 0;
    sMenuSelectionType[1] = 0;
    sMenuSwapTimer = 30;
    gMenuStatus = menuID;
    sMenuStackPos = 0;
    free_menu_display();
}

static void menu_set_backward(int menuID) {
    sMenuStackPos--;
    gMenuSelection[0] = sMenuSelectionPrev[sMenuStackPos][0];
    gMenuSelection[1] = sMenuSelectionPrev[sMenuStackPos][1];
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

void handle_menu_stick_input(int updateRate, int flags, short *selectionX, short *selectionY,  int minX, int minY, int maxX, int maxY) {
    int stickMag;
    int playSound = false;

    if (flags & MENUSTICK_STICKX) {
        stickMag = input_stick_x(STICK_LEFT);
        DECREASE_VAR(sMenuSelectionTimer[0], updateRate, 0);
        if (fabs(stickMag) > 25 || input_held(INPUT_DLEFT) || input_held(INPUT_DRIGHT)) {
            if (sMenuSelectionTimer[0] == 0) {
                if (sMenuSelectionType[0] == 0) {
                    sMenuSelectionTimer[0] = 30;
                    sMenuSelectionType[0] = 1;
                } else {
                    sMenuSelectionTimer[0] = 10;
                }
                if (stickMag < 0 || input_held(INPUT_DLEFT)) {
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
        stickMag = input_stick_y(STICK_LEFT);
        DECREASE_VAR(sMenuSelectionTimer[1], updateRate, 0);
        if (fabs(stickMag) > 25 || input_held(INPUT_DUP) || input_held(INPUT_DDOWN)) {
            if (sMenuSelectionTimer[1] == 0) {
                if (sMenuSelectionType[1] == 0) {
                    sMenuSelectionTimer[1] = 30;
                    sMenuSelectionType[1] = 1;
                } else {
                    sMenuSelectionTimer[1] = 10;
                }
                if (stickMag > 0 || input_held(INPUT_DUP)) {
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

char *sPauseMenuText[][LANG_TOTAL] = {
    {"Continue", "Continue2"},
    {"Options", "Options2"},
    {"Photo Mode", "Photo Mode2"},
    {"Quit", "Quit2"},
};

void menu_config(int updateRate, float updateRateF) {
    static void *ovl = NULL;
    static void (*func)(int, float);
    //overlay_run(0, updateRateF, "healthbar", sRenderHealth, &func, &ovl);


    if (sConfigMenu) {
        if (ovl == NULL) {
            ovl = dlopen(asset_dir("options", DFS_OVERLAY), RTLD_LOCAL);
            func = dlsym(ovl, "loop");
        }
        (*func)(updateRate, updateRateF);
        sConfigMenu = false;
    } else {
        if (ovl != NULL) {
            dlclose(ovl);
            ovl = NULL;
        }
    }
}

static void process_options_menu(int updateRate) {
    if (gMenuDisplay == NULL) {
        init_menu_display(32, display_get_height() - 80);
        for (int i = 0; i < sizeof(sPauseMenuText) / (sizeof(char *) * LANG_TOTAL); i++) {
            add_menu_text(sPauseMenuText[i][(int) gConfig.language], i, 0xFFFFFFFF, 0);
        }
    }

    handle_menu_stick_input(updateRate, MENUSTICK_STICKY, NULL, &gMenuSelection[1], 0, 0, 0, 4);

    if (input_pressed(INPUT_A, 5)) {
        play_sound_global(SOUND_MENU_CLICK);
        input_clear(INPUT_A);
        switch (gMenuSelection[1]) {
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
            gCamera->yaw = atan2s(gCamera->pos[0] - gCamera->focus[0], gCamera->pos[2] - gCamera->focus[2]) - 0x4000;
            gCamera->yawTarget = gCamera->yaw;
            gCamera->pitch = 0;
            gCameraHudToggle = true;
            screenshot_clear();
            break;
        case 3:
            menu_set_reset(MENU_TITLE);
            transition_into_scene(SCENE_INTRO, TRANSITION_FULLSCREEN_IN, 30, TRANSITION_FULLSCREEN_OUT);
            set_background_music(1, 30);
            break;
        }
    }

    if ((input_pressed(INPUT_START, 3) || input_pressed(INPUT_B, 3)) && sMenuSwapTimer == 0) {
        play_sound_global(SOUND_MENU_CLICK);
        input_clear(INPUT_START);
        input_clear(INPUT_B);
        menu_set_backward(MENU_PREV);
    }
}

#ifdef PUPPYPRINT_DEBUG
static void process_sceneselect_menu(int updateRate) {
    if (gMenuDisplay == NULL) {
        init_menu_display(32, 32);
        for (int i = 0; i < sizeof(sSceneTable) / sizeof(char *); i++) {
            add_menu_text(sSceneTable[i], i, 0xFFFFFFFF, 0);
        }
        add_menu_text("Back", gMenuDisplay->listCount, 0xFFFFFFFF, 0);
    }

    handle_menu_stick_input(updateRate, MENUSTICK_STICKY, NULL, &gMenuSelection[1], 0, 0, 0, gMenuDisplay->listCount);

    if (input_pressed(INPUT_A, 3) && sMenuSwapTimer == 0) {
        if (gMenuSelection[1] == gMenuDisplay->listCount - 1) {
            goto goback;
        }
        play_sound_global(SOUND_MENU_CLICK);
        input_clear(INPUT_A);
        transition_into_scene(gMenuSelection[1], TRANSITION_FULLSCREEN_IN, 30, TRANSITION_FULLSCREEN_OUT);
        menu_set_reset(MENU_CLOSED);
        set_background_music(0, 30);
    } else if (input_pressed(INPUT_B, 3) && sMenuSwapTimer == 0) {
        goback:
        play_sound_global(SOUND_MENU_CLICK);
        input_clear(INPUT_B);
        menu_set_backward(MENU_PREV);
    }
}
#endif

char *sTitleMenuText[][LANG_TOTAL] = {
    {"Play", "Play2"},
    {"Options", "Options2"},
#ifdef PUPPYPRINT_DEBUG
    {"Scene Select", "Scene Select2"},
#endif
    {"Controls", "Controls2"},
};

static void process_title_menu(int updateRate) {
    if (gCurrentController == -1) {
        return;
    }

    if (gMenuDisplay == NULL) {
        init_menu_display(32, display_get_height() - 80);
        for (int i = 0; i < sizeof(sTitleMenuText) / (sizeof(char *) * LANG_TOTAL); i++) {
            add_menu_text(sTitleMenuText[i][(int) gConfig.language], gMenuDisplay->listCount, 0xFFFFFFFF, 0);
        }
    }

    handle_menu_stick_input(updateRate, MENUSTICK_STICKY, NULL, &gMenuSelection[1], 0, 0, 0, gMenuDisplay->listCount);

    if (input_pressed(INPUT_A, 3) && sMenuSwapTimer == 0) {
        play_sound_global(SOUND_MENU_CLICK);
        input_clear(INPUT_A);
        switch (gMenuSelection[1]) {
        case 0:
            menu_set_reset(MENU_CLOSED);
            transition_into_scene(SCENE_TESTAREA, TRANSITION_FULLSCREEN_IN, 30, TRANSITION_FULLSCREEN_OUT);
            set_background_music(0, 30);
            break;
        case 1:
            menu_set_forward(MENU_CONFIG);
            sMenuSwapTimer = 30;
            break;
#ifdef PUPPYPRINT_DEBUG
        case 2:
            menu_set_forward(MENU_SCENESELECT);
            sMenuSwapTimer = 30;
            break;
#endif
        case 3:
            menu_set_forward(MENU_CONTROLS);
            sMenuSwapTimer = 30;
            break;
        }
    }
}

void process_menus(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    DECREASE_VAR(sMenuSwapTimer, updateRate, 0);
    menu_config(updateRate, updateRateF);
    switch (gMenuStatus) {
    case MENU_CLOSED:
        if (sMenuSwapTimer == 0 && gTalkControl == NULL && input_pressed(INPUT_START, 3)) {
            if (gCamera->mode == CAMERA_PHOTO) {
                camera_reset();
            } else {
                menu_set_forward(MENU_OPTIONS);
                screenshot_on(FMT_I8);
            }
            play_sound_global(SOUND_MENU_CLICK);
            input_clear(INPUT_START);
        }
        return;
    case MENU_TITLE:
        process_title_menu(updateRate);
        return;
    case MENU_OPTIONS:
        process_options_menu(updateRate);
        return;
    case MENU_CONTROLS:
        if (input_pressed(INPUT_B, 3)) {
            input_clear(INPUT_B);
            menu_set_backward(MENU_PREV);
        }
        return;
    case MENU_CONFIG:
        if ((input_pressed(INPUT_START, 3) || input_pressed(INPUT_B, 3)) && sMenuSwapTimer == 0) {
            sConfigMenu = false;
            play_sound_global(SOUND_MENU_CLICK);
            input_clear(INPUT_START);
            input_clear(INPUT_B);
            menu_set_backward(MENU_PREV);
            if (gMenuStatus == MENU_OPTIONS) {
                screenshot_on(FMT_I8);
            }
            save_config_write();
        } else {
            sConfigMenu = true;
        }
        return;
#ifdef PUPPYPRINT_DEBUG
    case MENU_SCENESELECT:
        process_sceneselect_menu(updateRate);
        return;
#endif
    }
    get_time_snapshot(PP_MENU, DEBUG_SNAPSHOT_1_END);
}

void render_menus(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    switch (gMenuStatus) {
    case MENU_CLOSED:
        return;
    case MENU_CONTROLS:
        rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(0, 0, 0, 255),});
        rdpq_text_print(NULL, FONT_MVBOLI, 33, 33, "Controls:\nA: Interact\nZ: Target\nL: Move Camera\n\n\nB: Back");
        rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(255, 255, 255, 255),});
        rdpq_text_print(NULL, FONT_MVBOLI, 32, 32, "Controls:\nA: Interact\nZ: Target\nL: Move Camera\n\n\nB: Back");
        break;
    case MENU_CONFIG:
        rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(0, 0, 0, 255),});
        rdpq_text_printf(NULL, FONT_MVBOLI, display_get_width() - 79, 17, "FPS: %2.2f", (double) display_get_fps());
        rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(255, 255, 255, 255),});
        rdpq_text_printf(NULL, FONT_MVBOLI, display_get_width() - 80, 16, "FPS: %2.2f", (double) display_get_fps());
        break;
    }
    render_menu_list();
    get_time_snapshot(PP_MENU, DEBUG_SNAPSHOT_1_END);
}