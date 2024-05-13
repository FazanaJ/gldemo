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

enum MenuFlags {
    MENUTEXT_NONE,
    MENUTEXT_BAR = (1 << 0),
};

typedef struct MenuOption {
    char **name;
    char *valuePtr;
    char minValue;
    char maxValue;
    char flags;
    void (*func)();
    char string;
} MenuOption;

typedef struct MenuListEntry {
    char *text;
    char colour[4];
    int flags;
    short var1;
    short var2;
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
    MENU_CONTROLS,
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
extern MenuListRoot *gMenuDisplay;
extern short gMenuSelection[2];

void add_menu_text(char *text, int index, unsigned int colour, int flags);
void edit_menu_text(char *text, int index);
void init_menu_display(int x, int y);
void render_menus(int updateRate, float updateRateF);
void process_menus(int updateRate, float updateRateF);
void handle_menu_stick_input(int updateRate, int flags, short *selectionX, short *selectionY,  int minX, int minY, int maxX, int maxY);
void free_menu_display(void);
void menu_reset_display(void);
void menutext_bar(MenuListEntry *m, int size);