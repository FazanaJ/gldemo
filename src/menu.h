#pragma once

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

enum MenuStatus {
    MENU_CLOSED,
    MENU_OPTIONS,

    MENU_TOTAL
};

extern char gMenuStatus;
extern char gIsPal;

void render_menus(int updateRate, float updateRateF);
void process_menus(int updateRate, float updateRateF);
void menu_set_sound(void);