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
static char sSaveMenu = false;
static char sKeyboard = false;
short gMenuSelection[2];
static short sMenuSelectionPrev[NUM_MENU_PREVS][2];
static char sMenuSelectionTimer[2] = {0, 0};
static char sMenuSelectionType[2] = {0, 0};
static unsigned char sMenuStackPos = 0;
MenuListRoot *gMenuDisplay = NULL;
char gMenuInputString[32];

void menu_input_string(char *string) {
    bzero(&gMenuInputString, sizeof(gMenuInputString));
    memcpy(&gMenuInputString, string, strlen(string));
    sKeyboard = false;
}

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

void menutext_bar(MenuListEntry *m, int size) {
    if (size < 0) {
        size = 0;
    }
    if (size > 100) {
        size = 100;
    }
    m->var1 = size;
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
    newList->var1 = 0;
    newList->var2 = 0;
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
        color_t colour;
        if (i == gMenuSelection[1]) {
            int sineCol = 128 + (32 * sins(gGameTimer * 0x400));
            colour =RGBA32(255, sineCol, sineCol, 255);
        } else {
            colour = RGBA32(list->colour[0], list->colour[1], list->colour[2], list->colour[3]);
        }
        text_outline(NULL, x, y, list->text, colour);
        if (list->flags & MENUTEXT_BAR) {
            rdpq_set_mode_fill(RGBA32(0, 0, 0, 255));
            rdpq_fill_rectangle(x + 139, y - 9, x + 241, y + 1);
            if (list->var1 != 0) {
                rdpq_set_mode_fill(RGBA32(255, 255, 255, 255));
                rdpq_fill_rectangle(x + 140, y - 8, x + 140 + list->var1, y);
            }
            rdpq_set_mode_standard();
        }
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


void (*sSavesRender)(int, float);

void menu_saves(int updateRate, float updateRateF) {
    static void *ovl = NULL;
    static void (*func)(int, float);
    //overlay_run(0, updateRateF, "healthbar", sRenderHealth, &func, &ovl);

    if (sSaveMenu) {
        if (ovl == NULL) {
            ovl = dlopen(asset_dir("pakmenu", DFS_OVERLAY), RTLD_LOCAL);
            void (*init)() = dlsym(ovl, "init");
            sSavesRender = dlsym(ovl, "render");
            (*init)();
            func = dlsym(ovl, "loop");
        }
        (*func)(updateRate, updateRateF);
        sSaveMenu = false;
    } else {
        if (ovl != NULL) {
            void (*close)() = dlsym(ovl, "close");
            (*close)();
            dlclose(ovl);
            ovl = NULL;
            sSavesRender = NULL;
        }
    }
}

void (*sKeyboardRender)(int, float);

void menu_keyboard(int updateRate, float updateRateF) {
    static void *ovl = NULL;
    static void (*func)(int, float);
    //overlay_run(0, updateRateF, "healthbar", sRenderHealth, &func, &ovl);

    if (sKeyboard) {
        if (ovl == NULL) {
            ovl = dlopen(asset_dir("keyboard", DFS_OVERLAY), RTLD_LOCAL);
            void (*init)() = dlsym(ovl, "init");
            sKeyboardRender = dlsym(ovl, "render");
            (*init)();
            func = dlsym(ovl, "loop");
        }
        (*func)(updateRate, updateRateF);
        //sKeyboard = false;
    } else {
        if (ovl != NULL) {
            void (*close)() = dlsym(ovl, "close");
            (*close)();
            dlclose(ovl);
            ovl = NULL;
            sKeyboardRender = NULL;
        }
    }
}

void menu_config(int updateRate, float updateRateF) {
    static void *ovl = NULL;
    static void (*func)(int, float);
    //overlay_run(0, updateRateF, "healthbar", sRenderHealth, &func, &ovl);

    if (sConfigMenu) {
        if (ovl == NULL) {
            ovl = dlopen(asset_dir("options", DFS_OVERLAY), RTLD_LOCAL);
            void (*init)() = dlsym(ovl, "init");
            (*init)();
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
    {"Saves", "Saves2"},
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
        case 2:
            menu_set_forward(MENU_SAVES);
            sMenuSwapTimer = 30;
            break;
#ifdef PUPPYPRINT_DEBUG
        case 3:
            menu_set_forward(MENU_SCENESELECT);
            sMenuSwapTimer = 30;
            break;
#endif
        case 4:
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
    menu_keyboard(updateRate, updateRateF);
    menu_saves(updateRate, updateRateF);
    if (sKeyboard) {
        get_time_snapshot(PP_MENU, DEBUG_SNAPSHOT_1_END);
        return;
    }
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
        break;
    case MENU_TITLE:
        process_title_menu(updateRate);
        break;
    case MENU_OPTIONS:
        process_options_menu(updateRate);
        break;
    case MENU_CONTROLS:
        if (input_pressed(INPUT_B, 3)) {
            input_clear(INPUT_B);
            menu_set_backward(MENU_PREV);
        }
        break;
    case MENU_SAVES:
        if (input_pressed(INPUT_B, 3) && sMenuSwapTimer == 0) {
            sSaveMenu = false;
            play_sound_global(SOUND_MENU_CLICK);
            input_clear(INPUT_B);
            menu_set_backward(MENU_PREV);
        } else {
            sSaveMenu = true;
        }
        break;
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
        break;
#ifdef PUPPYPRINT_DEBUG
    case MENU_SCENESELECT:
        process_sceneselect_menu(updateRate);
        break;
#endif
    }
    get_time_snapshot(PP_MENU, DEBUG_SNAPSHOT_1_END);
}

void menu_overlay_render(int updateRate, float updateRateF) {
    if (sSaveMenu && sSavesRender) {
        (*sSavesRender)(updateRate, updateRateF);
    } else if (sKeyboard && sKeyboardRender) {
        (*sKeyboardRender)(updateRate, updateRateF);
    }
}

void render_menus(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    switch (gMenuStatus) {
    case MENU_CLOSED:
        return;
    case MENU_CONTROLS:
        text_outline(NULL, 32, 32, "Controls:\nA: Interact\nZ: Target\nL: Move Camera\n\n\nB: Back", RGBA32(255, 255, 255, 255));
        break;
    case MENU_CONFIG:
        char textBytes[12];
        sprintf(textBytes, "FPS: %2.2f", (double) display_get_fps());
        text_outline(NULL, display_get_width() - 80, 16, textBytes, RGBA32(255, 255, 255, 255));
        break;
    case MENU_TITLE:
        break;
    }
    render_menu_list();
    menu_overlay_render(updateRate, updateRateF);
    get_time_snapshot(PP_MENU, DEBUG_SNAPSHOT_1_END);
}