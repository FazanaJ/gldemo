#pragma once

#include "../include/global.h"

enum OptionFlags {
    OPTION_NULL,
    OPTION_WRAP = (1 << 0),
    OPTION_STUB = (1 << 1),
    OPTION_BAR = (1 << 2),
    OPTION_STRING = (1 << 3),
    OPTION_PAL_OFFSET = (1 << 4),
    OPTION_PAL_ONLY = (1 << 5),
};

typedef struct MenuOption {
    char *name;
    char *valuePtr;
    char minValue;
    char maxValue;
    char flags;
    void (*func)();
    char string;
} MenuOption;

typedef struct MenuListEntry {
    char *text;
    int colour[4];
    int flags;
    struct MenuListEntry *prev;
    struct MenuListEntry *next;
} MenuListEntry;

typedef struct MenuListRoot {
    short x;
    short y;
    char listCount;
    char b;
    MenuListEntry *list;
    MenuListEntry *tail;
} MenuListRoot;

enum MenuStatus {
    MENU_PREV = -1,
    MENU_CLOSED,
    MENU_TITLE,
    MENU_OPTIONS,
    MENU_CONFIG,
#ifdef PUPPYPRINT_DEBUG
    MENU_SCENESELECT,
#endif

    MENU_TOTAL
};

enum MenuStickFlags {
    MENUSTICK_NULL,
    MENUSTICK_STICKX = (1 << 0),
    MENUSTICK_STICKY = (1 << 1),
    MENUSTICK_WRAPX = (1 << 2),
    MENUSTICK_WRAPY = (1 << 3),

    MENUSTICK_TOTAL
};

extern char gMenuStatus;
extern char gIsPal;

void render_menus(int updateRate, float updateRateF);
void process_menus(int updateRate, float updateRateF);
void menu_set_sound(void);
void handle_menu_stick_input(int updateRate, int flags, short *selectionX, short *selectionY,  int minX, int minY, int maxX, int maxY);