#include <libdragon.h>
#include <malloc.h>

#include "../../include/global.h"

#include "../assets.h"
#include "../main.h"
#include "../save.h"
#include "../menu.h"
#include "../input.h"
#include "../math_util.h"
#include "../hud.h"

#define TEST_MISSING_PAK 0

#define PAK_MODE_SELECT_SOURCE 0
#define PAK_MODE_OPTIONS 1
#define PAK_MODE_INFO 2
#define PAK_MODE_SELECT_DEST 3
#define PAK_MODE_CONFIRM 4
#define PAK_MODE_ERROR 5
#define PAK_MODE_FORMAT 6

static char sResetPaks;
static char sPakExists;
static char sResetTimer;
static char sPakMode;
static char sPakID;
static char sPrevPakID;
static char sPakModeOpt;
static char sPakError;
static char sPaksChanged[4];
static char sPakFull[4];
static char sPakFileIDs[16];
static char sPakPages[4];
static char sPakNotes[4];
static char sPakOptionCount[4];
static char sPrevPakSizes[4];
static short sPakConfirmOption;
static short sPakOption;
static short sPakErrorTime;
static float sPakScroll;
static float sPakScrollTarget;
static sprite_t *sControllerPakIcon;
static sprite_t *sRumblePakIcon;
static sprite_t *sTransferPakIcon;
static entry_structure_t *sPakSource;
static entry_structure_t *sPakFiles[16];

static void pak_reset_menu(void) {
    sPakMode = PAK_MODE_SELECT_SOURCE;
    sPakScroll = 0.0f;
    sPakScrollTarget = 0.0f;
    sPakOption = 0;
}

static void pakmenu_reset(void) {
    sPakExists = false;
    save_find_paks();
    if (gControllerPaks[(int) sPakID] != JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK) {
        sPakID--;
        if (sPakID == -1) {
            sPakID = 3;
        }
        while (gControllerPaks[(int) sPakID] != JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK) {
            sPakID--;
            if (sPakID == -1) {
                sPakID = 3;
            }
        }
    }
    for (int i = 0; i < 4; i++) {
        if (gControllerPaks[i] == JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK) {
            if (sPrevPakID != sPakID && sPakID == i) {
                if (sPakMode != PAK_MODE_SELECT_SOURCE && sPakMode != PAK_MODE_SELECT_DEST) {
                    sPakMode = PAK_MODE_SELECT_SOURCE;
                }
                sPakOption = 0;
                sPakScroll = 0.0f;
                sPakScrollTarget = 0.0f;
                sPakPages[i] = get_mempak_free_space(i);
                sPakNotes[i] = 0;
                bzero(&sPakFileIDs, sizeof(sPakFileIDs));
                sPakFull[i] = false;
                sPakOptionCount[i] = 0;
                if (validate_mempak(i) == 0) {
                    for (int j = 0; j < 16; j++) {
                        get_mempak_entry(i, j, sPakFiles[j]);
                        if (sPakFiles[j]->valid) {
                            sPakFileIDs[(int) sPakNotes[i]] = j;
                            sPakOptionCount[i]++;
                            sPakNotes[i]++;
                        }
                    }
                }
                if (sPakOptionCount[i] != 16 && sPakPages[i] != 0) {
                    sPakOptionCount[i]++;
                } else {
                    sPakFull[i] = true;
                }
                sPrevPakID = sPakID;
            }
            sPakExists = true;
        }
    }
    sResetPaks = false;
}

void init(void) {
    sResetPaks = true;
    sPakMode = PAK_MODE_SELECT_SOURCE;
    sPakID = 0;
    sPrevPakID = -1;
    sPakScroll = 0;
    sPakScrollTarget = 0;
    sPakOption = 0;
    sResetTimer = 0;
    sControllerPakIcon = sprite_load(asset_dir("cpak.ci8", DFS_SPRITE));
    sRumblePakIcon = sprite_load(asset_dir("rumblepak.ci8", DFS_SPRITE));
    sTransferPakIcon = sprite_load(asset_dir("transferpak.ci8", DFS_SPRITE));
    sPakFiles[0] = malloc(sizeof(entry_structure_t) * 16);
    sPakSource = malloc(sizeof(entry_structure_t));
    for (int i = 1; i < 16; i++) {
        sPakFiles[i] = (entry_structure_t *) (((unsigned int) sPakFiles[i - 1]) + sizeof(entry_structure_t));
    }
    for (int i = 0; i < 4; i++) {
        sPaksChanged[i] = false;
        sPrevPakSizes[i] = -1;
    }
}

void close(void) {
    sprite_free(sControllerPakIcon);
    sprite_free(sRumblePakIcon);
    sprite_free(sTransferPakIcon);
    free(sPakFiles[0]);
    free (sPakSource);
}

static char *sMissingText[] = {
    "No Controller Paks detected.",
    "No Controller Paks detected2.",
};

static char *sCopyText[] = {
    "Copy file where?",
    "Copy file where?2",
};

static char *sErrorText[][2] = {
    {"Not enough space to copy", "Not enough space to copy2"},
};

static char *sTextOptions[][2] = {
    {"Info", "Info2"},
    {"Copy", "Copy2"},
    {"Erase", "Erase2"},
    {"Back", "Back2"}
};

static char *sTextConfirm[][2] = {
    {"No", "No2"},
    {"Yes", "Yes2"},
};

static char *sTextConfirmNames[][2] = {
    {"Copy file?", "Copy file?2"},
    {"Erase file?", "Erase file?2"},
};

static char *sTextFormat[][2] = {
    {"Pak", "Pak"},
    {"must be formatted", "Must be formatted2"},
    {"Format now?", "Format now?2"},
};

void pakmenu_page_input(void) {
    int prevPak = sPakID;
    if (input_pressed(INPUT_L, 3) || input_pressed(INPUT_Z, 3)) {
        sPakID--;
        if (sPakID == -1) {
            sPakID = 3;
        }
        while (gControllerPaks[(int) sPakID] != JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK) {
            sPakID--;
            if (sPakID == -1) {
                sPakID = 3;
            }
        }
        sResetPaks = true;
    }
    if (input_pressed(INPUT_R, 3)) {
        sPakID++;
        if (sPakID == 4) {
            sPakID = 0;
        }
        while (gControllerPaks[(int) sPakID] != JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK) {
            sPakID++;
            if (sPakID == 4) {
                sPakID = 0;
            }
        }
        sResetPaks = true;
    }
    if (sPakID != prevPak) {
        pakmenu_reset();
    }
}

void loop(int updateRate, float updateRateF) {
    if (sResetPaks) {
        pakmenu_reset();
    }

    if (gSavePaks[(int) sPakID] == -3 && sPakMode != PAK_MODE_FORMAT) {
        sPakPages[(int) sPakID] = 0;
        sPakNotes[(int) sPakID] = 0;
        sPakOptionCount[(int) sPakID] = 0;
        sPakMode = PAK_MODE_FORMAT;
        sPakConfirmOption = 0;
        sPakExists = true;
        sPakModeOpt = 2;
        sResetPaks = false;
        return;
    }

    switch (sPakMode) {
    case PAK_MODE_SELECT_SOURCE:
    case PAK_MODE_SELECT_DEST:
        sResetTimer += updateRate;
        if (sResetTimer >= 120) {
            sResetPaks = true;
            sResetTimer -= 120;
        }
        
        int prevOption = sPakOption;
        handle_menu_stick_input(updateRate, MENUSTICK_STICKY, NULL, &sPakOption, 0, 0, 0, sPakOptionCount[(int) sPakID]);
        int diff = sPakOption - (int) sPakScrollTarget;
        if (prevOption < sPakOption) {
            if (sPakOption != 15 && diff >= 8 && sPakOptionCount[(int) sPakID] - sPakOption > 1) {
                sPakScrollTarget += 1.0f;
            }
        } else {
            if (sPakOption != 0 && diff <= 0) {
                sPakScrollTarget -= 1.0f;
            }
        }

        sPakScroll = lerpf(sPakScroll, sPakScrollTarget * 18.0f, 0.1f * updateRateF);

        if (input_pressed(INPUT_A, 3)) {
            input_clear(INPUT_A);
            switch (sPakMode) {
            case PAK_MODE_SELECT_SOURCE:
                if (sPakOption != sPakOptionCount[(int) sPakID] - 1 || sPakNotes[(int) sPakID] == 16 || sPakPages[(int) sPakID] == 0) {
                    get_mempak_entry(sPakID, (int) sPakFileIDs[sPakOption], sPakSource);
                    sPakMode = PAK_MODE_OPTIONS;
                }
                break;
            case PAK_MODE_SELECT_DEST:
                if (sPakModeOpt == 0 || sPakOption != sPakOptionCount[(int) sPakID] - 1) {
                    int offset;
                    if (sPakOption != sPakOptionCount[(int) sPakID] - 1) {
                        offset = sPakFiles[(int) sPakFileIDs[(int) sPakOption]]->blocks;
                    } else {
                        offset = 0;
                    }
                    if (sPakPages[(int) sPakID] + offset < sPakSource->blocks) {
                        sPakMode = PAK_MODE_ERROR;
                        sPakError = 0;
                        sPakErrorTime = timer_int(200);
                    } else {
                        sPakMode = PAK_MODE_CONFIRM;
                    }
                }
                break;
            }
            sPakConfirmOption = 0;
        }
        pakmenu_page_input();
        break;
    case PAK_MODE_OPTIONS:
        handle_menu_stick_input(updateRate, MENUSTICK_STICKY | MENUSTICK_WRAPY, NULL, &sPakConfirmOption, 0, 0, 0, 4);
        if (input_pressed(INPUT_A, 3)) {
            input_clear(INPUT_A);
            switch (sPakConfirmOption) {
            case 0: // Info
                sPakConfirmOption = 0;
                sPakMode = PAK_MODE_INFO;
                break;
            case 1: // Copy
                sPakConfirmOption = 0;
                sPakModeOpt = 0;
                sPakMode = PAK_MODE_SELECT_DEST;
                break;
            case 2: // Erase
                sPakConfirmOption = 0;
                sPakModeOpt = 1;
                sPakMode = PAK_MODE_CONFIRM;
                break;
            case 3: // Back
                sPakMode = PAK_MODE_SELECT_SOURCE;
                break;
            }
        }
        break;
    case PAK_MODE_INFO:
        if (input_pressed(INPUT_A, 3)) {
            input_clear(INPUT_A);
            sPakMode = PAK_MODE_SELECT_SOURCE;
        }
        break;
    case PAK_MODE_CONFIRM:
    case PAK_MODE_FORMAT:
        handle_menu_stick_input(updateRate, MENUSTICK_STICKY | MENUSTICK_WRAPY, NULL, &sPakConfirmOption, 0, 0, 0, 2);
        if (sPakMode == PAK_MODE_FORMAT) {
            pakmenu_page_input();
        }
        if (input_pressed(INPUT_A, 3)) {
            input_clear(INPUT_A);
            switch (sPakConfirmOption) {
            case 0: // No
                if (sPakMode == PAK_MODE_FORMAT) {
                    sPakID++;
                    if (sPakID == 4) {
                        sPakID = 0;
                    }
                    while (gControllerPaks[(int) sPakID] != JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK) {
                        sPakID++;
                        if (sPakID == 4) {
                            sPakID = 0;
                        }
                    }
                    sResetPaks = true;
                }
                sPakMode = PAK_MODE_SELECT_SOURCE;
                break;
            case 1: // Yes
                if (sPakModeOpt == 0) {
                    void *data = malloc(sPakSource->blocks * 256);
                    read_mempak_entry_data(sPakID, sPakSource, data);
                    if (sPakOption != sPakOptionCount[(int) sPakID] - 1) {
                        entry_structure_t *file = sPakFiles[(int) sPakFileIDs[(int) sPakOption]];
                        delete_mempak_entry(sPakID, file);
                    }
                    write_mempak_entry_data(sPakID, sPakSource, data);
                    free(data);
                } else if (sPakModeOpt == 1) {
                    delete_mempak_entry(sPakID, sPakSource);
                } else if (sPakModeOpt == 2) {
                    format_mempak(sPakID);
                }
                sPakOptionCount[(int) sPakID] = 0;
                pak_reset_menu();
                sResetPaks = true;
                sPrevPakID = -1;
                sPakMode = PAK_MODE_SELECT_SOURCE;
                break;
            }
        }
        break;
    case PAK_MODE_ERROR:
        sPakErrorTime -= updateRate;
        if (sPakErrorTime <= 0) {
            sPakMode = PAK_MODE_SELECT_SOURCE;
            pak_reset_menu();
        }
        break;
    }
}

void render(int updateRate, float updateRateF) {
    char textBytes[64];
    int sineCol = 128 + (32 * sins(gGameTimer * 0x400));
    int screenWidth = display_get_width();
    int screenHeight = display_get_height();
    int screenHalfW = screenWidth / 2;
    int screenHalfH = screenHeight / 2;

    if (sPakExists == false || TEST_MISSING_PAK) {
        text_outline(NULL, 33, 65, sMissingText[(int) gConfig.language], RGBA32(255, 255, 255, 255));
        return;
    }

    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(RGBA32(0, 0, 0, 192));
    rdpq_fill_rectangle(screenHalfW - 120, 16, screenHalfW + 120, 32); // Top

    rdpq_textparms_t parms = {
        .width = 240,
        .height = 0,
        .align = ALIGN_CENTER,
        .valign = VALIGN_CENTER,
    };
    
    sprintf(textBytes, "Pak %d - Notes: %d/16 - Pages: %d/123", sPakID + 1, sPakNotes[(int) sPakID], 123 - sPakPages[(int) sPakID]);
    text_outline(&parms, screenHalfW - 120, 28, textBytes, RGBA32(255, 255, 255, 255));

    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    if (sPakMode != PAK_MODE_INFO) {
        rdpq_set_scissor(0, 39, screenWidth, screenHeight - 39);

        rdpq_set_prim_color(RGBA32(0, 0, 0, 192));
        int y = 40 - sPakScroll;
        for (int i = 0; i < sPakOptionCount[(int) sPakID]; i++) {
            rdpq_fill_rectangle(screenHalfW - 120, y, screenHalfW + 80, y + 16); // Middle
            if (y + 16 > 40 && y < screenHeight - 24 && (i + 1 != sPakOptionCount[(int) sPakID] || sPakNotes[(int) sPakID] == 16 || sPakPages[(int) sPakID] == 0)) {
                rdpq_fill_rectangle(screenHalfW + 84, y, screenHalfW + 120, y + 16); // Middle Right
            }
            y += 18;
        }
        
        rdpq_mode_blender(0);
        y = 40 - sPakScroll;
        for (int i = 0; i < sPakOptionCount[(int) sPakID]; i++) {
            if (y + 16 > 40 && y < screenHeight - 24) {
                char *text;
                int isLast;
                int size;
                int fileID = sPakFileIDs[i];
                unsigned int colour;
                if (sPakOption == i) {
                    colour = sineCol;
                } else {
                    colour = 255;
                }
                if (i + 1 == sPakOptionCount[(int) sPakID] && sPakNotes[(int) sPakID] != 16 && sPakPages[(int) sPakID] != 0) {
                    isLast = true;
                } else {
                    isLast = false;
                }

                if (isLast) {
                    text = "------";
                } else {
                    text = sPakFiles[fileID]->name;
                }
                parms.width = 200;
                text_outline(&parms, screenHalfW - 120, y + 12, text, RGBA32(255, colour, colour, 255));

                if (isLast == false) {
                    parms.width = 36;
                    size = sPakFiles[fileID]->blocks;
                    sprintf(textBytes, "%d", size);
                    text_outline(&parms, screenHalfW + 84, y + 12, textBytes, RGBA32(255, colour, colour, 255));
                }
            }
            y += 18;
        }
        rdpq_set_scissor(0, 0, screenWidth, screenHeight);
    } else {
        rdpq_set_prim_color(RGBA32(0, 0, 0, 192));
        rdpq_fill_rectangle(screenHalfW - 120, screenHalfH - 80, screenHalfW + 120, screenHalfH + 80);
        parms.width = 240;
        text_outline(&parms, screenHalfW - 120 + 1, screenHalfH - 64, sPakSource->name, RGBA32(255, 255, 255, 255));
        sprintf(textBytes, "Size: %d pages", sPakSource->blocks);
        text_outline(&parms, screenHalfW - 120 + 1, screenHalfH - 48, textBytes, RGBA32(255, 255, 255, 255));
        sprintf(textBytes, "Vendor: 0x%X", (unsigned int) sPakSource->vendor);
        text_outline(&parms, screenHalfW - 120 + 1, screenHalfH - 36, textBytes, RGBA32(255, 255, 255, 255));
        sprintf(textBytes, "Company ID: 0x%X", sPakSource->game_id);
        text_outline(&parms, screenHalfW - 120 + 1, screenHalfH - 24, textBytes, RGBA32(255, 255, 255, 255));
        sprintf(textBytes, "Region: 0x%X", sPakSource->region);
        text_outline(&parms, screenHalfW - 120 + 1, screenHalfH - 12, textBytes, RGBA32(255, 255, 255, 255));
        sprintf(textBytes, "Note: %X", sPakSource->entry_id + 1);
        text_outline(&parms, screenHalfW - 120 + 1, screenHalfH - 0, textBytes, RGBA32(255, 255, 255, 255));
    }

    int x = screenHalfW - (32 * 2);
    int x2 = screenHalfW + (32 * 2) - 8;
    int y = screenHeight - 32;

    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(RGBA32(0, 0, 0, 255));
    // Left arrow
    rdpq_triangle(&TRIFMT_FILL,
        (float[]){x - 8, y},
        (float[]){x - 32, y + 12},
        (float[]){x - 8, y + 24}
    );
    // Right arrow
    rdpq_triangle(&TRIFMT_FILL,
        (float[]){x2 + 8, y},
        (float[]){x2 + 32, y + 12},
        (float[]){x2 + 8, y + 24}
    );
    text_outline(NULL, x - 22, y + 16, "Z", RGBA32(255, 255, 255, 255));
    text_outline(NULL, x2 + 10, y + 16, "R", RGBA32(255, 255, 255, 255));
    for (int i = 0; i < 4; i++) {
        sprite_t *spr = NULL;

        if (gControllerPaks[i] == JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK) {
            spr = sControllerPakIcon;
        } else if (gControllerPaks[i] == JOYPAD_ACCESSORY_TYPE_TRANSFER_PAK) {
            spr = sTransferPakIcon;
        } else if (gControllerPaks[i] == JOYPAD_ACCESSORY_TYPE_RUMBLE_PAK) {
            spr = sRumblePakIcon;
        }

        unsigned int colour;
        int colour2;
        if (sPakID == i && gControllerPaks[(int) sPakID] == JOYPAD_ACCESSORY_TYPE_CONTROLLER_PAK) {
            colour = sineCol;
            colour2 = 255;
        } else {
            colour = 0;
            colour2 = 0;
        }
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        rdpq_set_prim_color(RGBA32(colour2, colour, colour, 255));
        rdpq_fill_rectangle(x, screenHeight - 32, x + 24, screenHeight - 8);
        if (spr == NULL) {
            rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
            rdpq_fill_rectangle(x + 1, screenHeight - 32 + 1, x + 23, screenHeight - 9);
        } else {
            rdpq_mode_combiner(RDPQ_COMBINER_TEX);
            rdpq_sprite_blit(spr, x + 1, screenHeight - 31, NULL);
        }

        x += 32;
    }

    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    if (sPakMode == PAK_MODE_OPTIONS) {
        rdpq_set_prim_color(RGBA32(0, 0, 0, 224));
        rdpq_fill_rectangle(screenHalfW - 40, screenHalfH - 32, screenHalfW + 40, screenHalfH + 24);
        parms.width = 80;
        for (int i = 0; i < 4; i++) {
            int colour;
            if (sPakConfirmOption == i) {
                colour = sineCol;
            } else {
                colour = 255;
            }
            text_outline(&parms, screenHalfW - 40, (screenHalfH - 18) + (i * 12), sTextOptions[i][(int) gConfig.language], RGBA32(255, colour, colour, 255));
        }
    } else if (sPakMode == PAK_MODE_SELECT_DEST) {
    rdpq_set_prim_color(RGBA32(0, 0, 0, 240));
    rdpq_fill_rectangle(screenHalfW - 80, 16, screenHalfW + 80, 32);
    parms.width = 160;
    text_outline(&parms, screenHalfW - 80, 28, sCopyText[(int) gConfig.language], RGBA32(255, sineCol, sineCol, 255));
    } else if (sPakMode == PAK_MODE_CONFIRM) {
        rdpq_set_prim_color(RGBA32(0, 0, 0, 224));
        rdpq_fill_rectangle(screenHalfW - 80, screenHalfH - 32, screenHalfW + 80, screenHalfH + 24);
        parms.width = 160;
        text_outline(&parms, screenHalfW - 80, (screenHalfH - 18), sTextConfirmNames[(int)sPakModeOpt][(int) gConfig.language], RGBA32(255, 255, 255, 255));
        for (int i = 0; i < 2; i++) {
            int colour;
            if (sPakConfirmOption == i) {
                colour = sineCol;
            } else {
                colour = 255;
            }
            text_outline(&parms, screenHalfW - 80, (screenHalfH) + (i * 12), sTextConfirm[i][(int) gConfig.language], RGBA32(255, colour, colour, 255));
        }
    } else if (sPakMode == PAK_MODE_ERROR) {
        rdpq_set_prim_color(RGBA32(0, 0, 0, 224));
        rdpq_fill_rectangle(screenHalfW - 80, screenHalfH, screenHalfW + 80, screenHalfH + 32);
        parms.width = 160;
        text_outline(&parms, screenHalfW - 80, (screenHalfH + 16), sErrorText[(int)sPakError][(int) gConfig.language], RGBA32(255, 255, 255, 255));
    } else if (sPakMode == PAK_MODE_FORMAT) {
        rdpq_set_prim_color(RGBA32(0, 0, 0, 224));
        rdpq_fill_rectangle(screenHalfW - 80, screenHalfH - 32, screenHalfW + 80, screenHalfH + 40);
        parms.width = 160;
        parms.line_spacing = -8;
        char textBytes[64];
        sprintf(textBytes, "%s %d %s\n%s", sTextFormat[0][(int) gConfig.language], sPakID + 1, sTextFormat[1][(int) gConfig.language], sTextFormat[2][(int) gConfig.language]);
        text_outline(&parms, screenHalfW - 80, (screenHalfH - 18), textBytes, RGBA32(255, 255, 255, 255));
        for (int i = 0; i < 2; i++) {
            int colour;
            if (sPakConfirmOption == i) {
                colour = sineCol;
            } else {
                colour = 255;
            }
            text_outline(&parms, screenHalfW - 80, (screenHalfH + 12) + (i * 12), sTextConfirm[i][(int) gConfig.language], RGBA32(255, colour, colour, 255));
        }
    }
}