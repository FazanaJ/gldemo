#include <libdragon.h>
#include <malloc.h>

#include "../../include/global.h"

#include "../assets.h"
#include "../audio.h"
#include "../menu.h"
#include "../main.h"
#include "../input.h"
#include "../math_util.h"

static char sMenuTextStack[64];
static char sNumOptions = 0;

static char *sOptionTextGraphics[LANG_TOTAL] = {
    "Graphics",
    "Graphics2"
};

static char *sOptionTextLanguage[LANG_TOTAL] = {
    "Language",
    "Language2"
};

static char *sOptionTextScreenMode[LANG_TOTAL] = {
    "Screen Mode",
    "Screen Mode2"
};

static char *sOptionTextPal[LANG_TOTAL] = {
    "PAL",
    "PAL2"
};

static char *sOptionTextSoundMode[LANG_TOTAL] = {
    "Sound Mode",
    "Sound Mode2"
};

static char *sOptionTextScreenX[LANG_TOTAL] = {
    "Screen Pos X",
    "Screen Pos X2"
};

static char *sOptionTextScreenY[LANG_TOTAL] = {
    "Screen Pos Y",
    "Screen Pos Y2"
};

static char *sOptionTextSound[LANG_TOTAL] = {
    "Sound Volume",
    "Sound Volume2"
};

static char *sOptionTextMusic[LANG_TOTAL] = {
    "Music Volume",
    "Music Volume2"
};

static char *sOptionTextVsync[LANG_TOTAL] = {
    "Vsync",
    "Vsync2"
};

extern int __boot_tvtype;

static void menu_change_pal60(void) {
    if (gConfig.regionMode == PAL60) {
        gConfig.regionMode = NTSC60;
    }
    __boot_tvtype = gConfig.regionMode; // Writes to osTvType
    menu_reset_display();
}

static void menu_set_sound(void) {
    gMusicVolume = (float) gConfig.musicVolume / (float) 10.0f;
    gSoundVolume = (float) gConfig.soundVolume / (float) 10.0f;
    set_music_volume(gMusicVolume);
}

static MenuOption sMenuOptions[] = {
    {sOptionTextGraphics, &gConfig.graphics, -1, 1, OPTION_WRAP | OPTION_STRING, NULL, 0},
    {sOptionTextLanguage, &gConfig.language, 0, 1, OPTION_WRAP | OPTION_STRING, free_menu_display, 15},
    {sOptionTextScreenMode, &gConfig.screenMode, 0, 2, OPTION_WRAP | OPTION_STRING | OPTION_STUB, menu_reset_display, 5},
    {sOptionTextPal, &gConfig.regionMode, 0, 1, OPTION_WRAP | OPTION_PAL_ONLY | OPTION_STRING, menu_change_pal60, 8},
    {sOptionTextSoundMode, &gConfig.soundMode, 0, 2, OPTION_WRAP | OPTION_STRING, NULL, 10},
    {sOptionTextScreenX, &gConfig.screenPosX, -8, 8, OPTION_STUB | OPTION_BAR, NULL, 0},
    {sOptionTextScreenY, &gConfig.screenPosY, -8, 8, OPTION_STUB | OPTION_BAR, NULL, 0},
    {sOptionTextSound, &gConfig.soundVolume, 0, 10, OPTION_BAR, menu_set_sound, 0},
    {sOptionTextMusic, &gConfig.musicVolume, 0, 10, OPTION_BAR, menu_set_sound, 0},
    {sOptionTextVsync, &gConfig.vsync, 0, 1, OPTION_STRING | OPTION_STUB, NULL, 13},
};

static const char *sMenuOptionStrings[][LANG_TOTAL] = {
    {"Performance", "Performance2"},
    {"Default", "Default2"},
    {"Beautiful", "Beautiful2"},
    {"Off", "Off"},
    {"On", "On"},
    {"4:3", "4:3"},
    {"16:10", "16:10"},
    {"16:9", "16:9"},
    {"50Hz", "50Hz"},
    {"60Hz", "60Hz"},
    {"Mono", "Mono2"},
    {"Stereo", "Stereo2"},
    {"Surround", "Surround2"},
    {"Triple Buffered", "Triple Buffered2"},
    {"Double Buffered", "Double Buffered2"},
    {"English", NULL},
    {NULL, "Placeholder"}
};

static char *set_option_text(int optID) {
    if (sMenuOptions[optID].flags & OPTION_STRING) {
        int stringOffset = sMenuOptions[optID].string + (*sMenuOptions[optID].valuePtr - sMenuOptions[optID].minValue);
        if (gIsPal && sMenuOptions[optID].flags & OPTION_PAL_OFFSET) {
            stringOffset += (sMenuOptions[optID].maxValue - sMenuOptions[optID].minValue) + 1;
        }
        sprintf(sMenuTextStack, "%s: %s", sMenuOptions[optID].name[(int) gConfig.language], sMenuOptionStrings[stringOffset][(int) gConfig.language]);
    } else if (sMenuOptions[optID].flags & OPTION_BAR) {
        sprintf(sMenuTextStack, "%s:", sMenuOptions[optID].name[(int) gConfig.language]);
    } else {
        sprintf(sMenuTextStack, "%s: %d", sMenuOptions[optID].name[(int) gConfig.language], *sMenuOptions[optID].valuePtr);
    }
    return sMenuTextStack;
}

void init(void) {
    if (gMenuDisplay == NULL) {
        sNumOptions = 0;
        init_menu_display(16, 22);
        for (int i = 0; i < sizeof(sMenuOptions) / sizeof(MenuOption); i++) {
            int colour;
            int hasBar;
            if (sMenuOptions[i].flags & OPTION_STUB || (sMenuOptions[i].flags & OPTION_PAL_ONLY && gIsPal == false)) {
                colour = 0x80808080;
            } else {
                colour = 0xFFFFFFFF;
            }
            if (sMenuOptions[i].flags & OPTION_BAR) {
                hasBar = true;
            } else {
                hasBar = false;
            }
            add_menu_text(set_option_text(i), sNumOptions, colour, hasBar);
            sNumOptions++;
        }
    }
}

void loop(int updateRate) {
    int xWrap = 0;
    MenuOption *m = &sMenuOptions[gMenuSelection[1]];
    if (m->flags & OPTION_WRAP) {
        xWrap = MENUSTICK_WRAPX;
    }

    if (input_held(INPUT_L)) {
        return;
    }

    short tempVar = *m->valuePtr;
    int prevMenuSelection = gMenuSelection[1];
    handle_menu_stick_input(updateRate, MENUSTICK_STICKX | MENUSTICK_STICKY | MENUSTICK_WRAPY | xWrap, &tempVar, &gMenuSelection[1], m->minValue, -1, m->maxValue, (sizeof(sMenuOptions) / sizeof(MenuOption)) + 1);
    if (tempVar != *m->valuePtr) {
        *m->valuePtr = tempVar;
        edit_menu_text(set_option_text(gMenuSelection[1]), gMenuSelection[1]);
        if (m->func) {
            (m->func)();
        }
        init();
    }
    if (gMenuSelection[1] < 0) {
        gMenuSelection[1] = (sizeof(sMenuOptions) / sizeof(MenuOption)) -1;
        prevMenuSelection = gMenuSelection[1] + 1;
    }
    if (gMenuSelection[1] >= sizeof(sMenuOptions) / sizeof(MenuOption)) {
        gMenuSelection[1] = 0;
        prevMenuSelection = gMenuSelection[1] - 1;
    }
    if (prevMenuSelection != gMenuSelection[1]) {
        if (prevMenuSelection < gMenuSelection[1]) {
            while (sMenuOptions[gMenuSelection[1]].flags & OPTION_STUB || (sMenuOptions[gMenuSelection[1]].flags & OPTION_PAL_ONLY && gIsPal == false)) {
                gMenuSelection[1]++;
                if (gMenuSelection[1] >= sizeof(sMenuOptions) / sizeof(MenuOption)) {
                    gMenuSelection[1] = 0;
                }
                if (gMenuSelection[1] < 0) {
                    gMenuSelection[1] = (sizeof(sMenuOptions) / sizeof(MenuOption)) -1;
                }
            }
        } else {
            while (sMenuOptions[gMenuSelection[1]].flags & OPTION_STUB || (sMenuOptions[gMenuSelection[1]].flags & OPTION_PAL_ONLY && gIsPal == false)) {
                gMenuSelection[1]--;
                if (gMenuSelection[1] < 0) {
                    gMenuSelection[1] = (sizeof(sMenuOptions) / sizeof(MenuOption)) -1;
                }
                if (gMenuSelection[1] >= sizeof(sMenuOptions) / sizeof(MenuOption)) {
                    gMenuSelection[1] = 0;
                }
            }
        }
    }
    MenuListEntry *entry = gMenuDisplay->list;
    for (int i = 0; i < gMenuDisplay->listCount; i++) {
        if ((sMenuOptions[i].flags & OPTION_BAR) == false) {
            entry = entry->next;
            continue;
        }
        int entryOffset;
        entryOffset = MIN(0, sMenuOptions[i].minValue);
        menutext_bar(entry, (int)(((float) (*sMenuOptions[i].valuePtr - entryOffset) / (float) (sMenuOptions[i].maxValue - entryOffset)) * 100.0f));
        entry = entry->next;
    } 
}

