#include <libdragon.h>
#include <malloc.h>

#include "../../include/global.h"

#include "../assets.h"
#include "../hud.h"
#include "../menu.h"
#include "../main.h"
#include "../input.h"
#include "../math_util.h"
#include "../audio.h"

float sKeyboardPos[2];
short sKeyboardKeyPos[2];
char sKeyboardEntry[32];
char sKeyboardCharLimit;
char sKeyboardCursorPos;
char sKeyboardUpperCase;
char sKeyboardShift;
char sSpacePrev;
char sKeyboardAnimID;
char sKeyboardAnimPos;
char sKeyboardAnimDown;
char sKeyboardAnimStage;
sprite_t *sKeyboardCapSprite;
sprite_t *sKeyboardSprite;

const char *sKeyboardString[] = {
    "1234567890\n"
    "qwertyuiop\n"
    "asdfghjkl.\n"
    "zxcvbnm ",

    "1234567890\n"
    "QWERTYUIOP\n"
    "ASDFGHJKL.\n"
    "ZXCVBNM "
};

char *sKeyboardNames[][2] = {
    {"Del", "Del2"},
    {"Caps", "Caps2"},
    {"Shift", "Shift2"},
    {"Done", "Done2"},
    {"Space", "Space2"}
};

void init(void) {
    sKeyboardEntry[0] = '\0';
    sKeyboardCharLimit = 12;
    sKeyboardCursorPos = 1;
    sKeyboardUpperCase = true;
    sKeyboardShift = true;
    sKeyboardPos[0] = (display_get_width() / 2) - (280 / 2);
    sKeyboardPos[1] = display_get_height() - 96;
    sKeyboardCapSprite = sprite_load(asset_dir("keycap.ia8", DFS_SPRITE));
    sKeyboardSprite = sprite_load(asset_dir("keyboard.ia8", DFS_SPRITE));
}

void loop(int updateRate, float updateRateF) {
    int prevPosX = sKeyboardKeyPos[0];
    int prevPosY = sKeyboardKeyPos[1];
    int keyPos = sKeyboardKeyPos[0] + (sKeyboardKeyPos[1] * 10) + sKeyboardKeyPos[1];
    handle_menu_stick_input(updateRate, MENUSTICK_STICKX | MENUSTICK_STICKY | MENUSTICK_WRAPX | MENUSTICK_WRAPY, &sKeyboardKeyPos[0], &sKeyboardKeyPos[1], 0, 0, 10, 4);


    // This crazy little contraption handles the spacebar logic.
    // This top one restores the previous X pos when leaving the space bar.
    if (prevPosY == 3 && sKeyboardKeyPos[1] != 3 && sKeyboardKeyPos[0] >= 7 && sKeyboardKeyPos[0] < 10) {
        sKeyboardKeyPos[0] = sSpacePrev;
    // This sets the cursor to the space bar since it takes 3 spaces.
    } else if (sKeyboardKeyPos[1] == 3 && sKeyboardKeyPos[0] >= 7 && sKeyboardKeyPos[0] < 10) {
        // Cursor moves right from space, go straight to end.
        if (sKeyboardKeyPos[0] > prevPosX && sKeyboardKeyPos[0] > 7) {
            sKeyboardKeyPos[0] = 10;
        } else {
            sKeyboardKeyPos[0] = 7;
        }
    } else {
        // And this sets the previous X pos to set back when leaving the space bar.
        sSpacePrev = prevPosX;
        sSpacePrev = CLAMP(sSpacePrev, 7, 9);
    }

    switch (sKeyboardAnimStage) {
    case 1: 
        sKeyboardAnimPos += updateRate * 2;
        if (sKeyboardAnimPos >= 4) {
            sKeyboardAnimPos = 4;
            sKeyboardAnimStage++;
        }
        break;
    case 2: 
        sKeyboardAnimPos -= updateRate * 3;
        if (sKeyboardAnimPos < 0) {
            sKeyboardAnimPos = 0;
            sKeyboardAnimStage = 0;
        }
        break;
    }

    if (input_pressed(INPUT_A, 3)) {
        int p = sKeyboardCursorPos - 1;
        input_clear(INPUT_A);
        if (sKeyboardKeyPos[0] < 10) {
            if (sKeyboardCursorPos <= sKeyboardCharLimit) {
                sKeyboardEntry[p] = sKeyboardString[(int) sKeyboardUpperCase][keyPos];
                sKeyboardEntry[p + 1] = '\0';
                sKeyboardCursorPos++;
                if (sKeyboardShift) {
                    sKeyboardShift = 0;
                    sKeyboardUpperCase ^= 1;
                }
            }
        } else {
            switch (sKeyboardKeyPos[1]) {
            case 0:
                if (sKeyboardCursorPos > 1) {
                    sKeyboardCursorPos--;
                    sKeyboardEntry[p - 1] = '\0';
                    if (sKeyboardCursorPos == 1) {
                        sKeyboardShift = true;
                        sKeyboardUpperCase = true;
                    }
                }
                break;
            case 2:
                sKeyboardShift ^= 1;
                // fallthrough
            case 1: 
                sKeyboardUpperCase ^= 1;
                break;
            case 3:
                menu_input_string(sKeyboardEntry);
                break;
            }
        }
        if (sKeyboardAnimStage == 0) {
            sKeyboardAnimStage = 1;
            sKeyboardAnimID = keyPos;
            if (sKeyboardKeyPos[0] == 10) {
                sKeyboardAnimDown = sKeyboardKeyPos[1];
            } else {
                if (sKeyboardKeyPos[1] == 3 && sKeyboardKeyPos[0] == 7) {
                    sKeyboardAnimDown = 4;
                } else {

                    sKeyboardAnimDown = -1;
                }
            }
        }
        play_sound_global(SOUND_KEYBOARD);
    } else if (input_pressed(INPUT_B, 3)) {
        play_sound_global(SOUND_KEYBOARD);
        if (sKeyboardAnimStage == 0) {
            sKeyboardAnimStage = 1;
            sKeyboardAnimID = -1;
            sKeyboardAnimDown = 0;
        }
        int p = sKeyboardCursorPos - 1;
        input_clear(INPUT_B);
        if (sKeyboardCursorPos > 1) {
            sKeyboardCursorPos--;
            sKeyboardEntry[p - 1] = '\0';
            if (sKeyboardCursorPos == 1) {
                sKeyboardShift = true;
                sKeyboardUpperCase = true;
            }
        }
    }
}

void render(int updateRate, float updateRateF) {
    int sineCol = 128 + (32 * sins(gGameTimer * 0x400));
    int keyPos = sKeyboardKeyPos[0] + (sKeyboardKeyPos[1] * 10) + sKeyboardKeyPos[1];
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_alphacompare(64);
    rdpq_mode_blender(0);

    rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
    rdpq_fill_rectangle(sKeyboardPos[0] + (140) - 64, sKeyboardPos[1] - 40, sKeyboardPos[0] + (140) + 64, sKeyboardPos[1] + 4);

    rdpq_set_prim_color(RGBA32(214 * 0.5f, 209 * 0.5f, 185 * 0.5f, 255));
    rdpq_texparms_t texParams = {
        .s.repeats = 32,
        .s.mirror = MIRROR_REPEAT,
    };
    rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);
    rdpq_sprite_upload(0, sKeyboardSprite, &texParams);
    rdpq_texture_rectangle_raw(0, sKeyboardPos[0], sKeyboardPos[1], sKeyboardPos[0] + 280, sKeyboardPos[1] + 96, 0, 0, 0.45714f, 0.66f);

    rdpq_set_prim_color(RGBA32(214, 209, 185, 255));
    rdpq_sprite_upload(0, sKeyboardCapSprite, &texParams);
    int x = sKeyboardPos[0] + 24;
    int y = sKeyboardPos[1] + 8;
    for (int i = 0; sKeyboardString[(int) sKeyboardUpperCase][i] != ' '; i++) {
        if (sKeyboardString[(int) sKeyboardUpperCase][i] != '\n') {
            int newY;
            if (i == sKeyboardAnimID) {
                newY = y + sKeyboardAnimPos;
            } else {
                newY = y;
            }
            rdpq_texture_rectangle_raw(0, x, newY, x + 16, newY + 16, 0, 0, 4, 4);
            x += 20;
        } else {
            x = sKeyboardPos[0] + 24;
            y += 20;
        }
    }

    x = sKeyboardPos[0] + 224;
    y = sKeyboardPos[1] + 8;
    texParams.s.repeats = 0;
    rdpq_sprite_upload(0, sKeyboardCapSprite, &texParams);
    for (int i = 0; i < 4; i++) {
        int newY;
        if (i == sKeyboardAnimDown) {
            newY = y + sKeyboardAnimPos;
        } else {
            newY = y;
        }
        rdpq_texture_rectangle_raw(0, x, newY, x + 16, newY + 16, 0, 0, 4, 4);
        rdpq_texture_rectangle_raw(0, x + 16, newY, x + 32, newY + 16, 32 << 1, 0, -4, 4);
        y += 20;
    }

    y -= 20;
    x = sKeyboardPos[0] + 164;
    int newY;
    if (sKeyboardAnimDown == 4) {
        newY = y + sKeyboardAnimPos;
    } else {
        newY = y;
    }
    rdpq_texture_rectangle_raw(0, x, newY, x + 28, newY + 16, 0, 0, 4, 4);
    rdpq_texture_rectangle_raw(0, x + 28, newY, x + 56, newY + 16, 56 << 1, 0, -4, 4);

    int prevColour = 0;
    char str[2] = "\0\0";
    rdpq_textparms_t parms = {
        .width = 16,
        .height = 0,
        .align = ALIGN_CENTER,
        .valign = VALIGN_CENTER,
    };
    rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(0, 0, 0, 255),});
    x = sKeyboardPos[0] + 24;
    y = sKeyboardPos[1] + 19;
    for (int i = 0; sKeyboardString[(int) sKeyboardUpperCase][i] != ' '; i++) {
        if (sKeyboardString[(int) sKeyboardUpperCase][i] != '\n') {
            int colour;
            int colour2;
            str[0] = sKeyboardString[(int) sKeyboardUpperCase][i];
            if (i == keyPos) {
                colour = sineCol;
                colour2 = 255;
            } else {
                colour = 0;
                colour2 = 0;
            }
            if (prevColour != colour) {
                prevColour = colour;
                rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(colour2, colour, colour, 255),});
            }
            int newY;
            if (i == sKeyboardAnimID) {
                newY = y + sKeyboardAnimPos;
            } else {
                newY = y;
            }
            rdpq_text_print(&parms, FONT_MVBOLI, x, newY, str);
            x += 20;
        } else {
            x = sKeyboardPos[0] + 24;
            y += 20;
        }
    }

    parms.width = 32;
    x = sKeyboardPos[0] + 224;
    y = sKeyboardPos[1] + 19;
    for (int i = 0; i < 4; i++) {
        int colour;
        int colour2;
        if (i == sKeyboardKeyPos[1] && sKeyboardKeyPos[0] == 10) {
            colour = sineCol;
            colour2 = 255;
        } else if ((i == 1 && !sKeyboardShift && sKeyboardUpperCase) || (i == 2 && sKeyboardShift)) {
            colour = 0;
            colour2 = 255;
        } else {
            colour = 0;
            colour2 = 0;
        }
        int newY;
        if (i == sKeyboardAnimDown) {
            newY = y + sKeyboardAnimPos;
        } else {
            newY = y;
        }
        rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(colour2, colour, colour, 255),});
        rdpq_text_print(&parms, FONT_MVBOLI, x, newY, sKeyboardNames[i][(int) gConfig.language]);
        y += 20;
    }
    
    int colour;
    int colour2;
    parms.width = 56;
    if (sKeyboardKeyPos[0] == 7 && sKeyboardKeyPos[1] == 3) {
        colour = sineCol;
        colour2 = 255;
    } else {
        colour = 0;
        colour2 = 0;
    }
    if (sKeyboardAnimDown == 4) {
        newY = sKeyboardAnimPos;
    } else {
        newY = 0;
    }
    rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(colour2, colour, colour, 255),});
    rdpq_text_print(&parms, FONT_MVBOLI, sKeyboardPos[0] + 164, newY + sKeyboardPos[1] + 79, sKeyboardNames[4][(int) gConfig.language]);

    char textBytes[sizeof(sKeyboardEntry) + 1];
    sprintf(textBytes, "%s|", sKeyboardEntry);
    rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(32, 32, 32, 255),});
    rdpq_text_print(NULL, FONT_MVBOLI, sKeyboardPos[0] + (140) - 56, sKeyboardPos[1] - 24, "Name:");
    rdpq_text_print(NULL, FONT_MVBOLI, sKeyboardPos[0] + (140) - 56, sKeyboardPos[1] - 10, textBytes);
}

void close(void) {
    sprite_free(sKeyboardCapSprite);
    sprite_free(sKeyboardSprite);
}