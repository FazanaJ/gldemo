#include <libdragon.h>
#include <malloc.h>

#include "../../include/global.h"

#include "../assets.h"
#include "../hud.h"
#include "../menu.h"
#include "../main.h"
#include "../input.h"
#include "../math_util.h"

float sKeyboardPos[2];
short sKeyboardKeyPos[2];
char sKeyboardEntry[32];
char sKeyboardCharLimit;
char sKeyboardCursorPos;
char sKeyboardUpperCase;
char sKeyboardShift;

const char *sKeyboardString[] = {
    "1234567890\n"
    "qwertyuiop\n"
    "asdfghjkl!\n"
    "zxcvbnm .?\n",
    "1234567890\n"
    "QWERTYUIOP\n"
    "ASDFGHJKL!\n"
    "ZXCVBNM .?\n"
};

char *sKeyboardNames[][2] = {
    {"Del", "Del2"},
    {"Caps", "Caps2"},
    {"Shift", "Shift2"},
    {"Done", "Done"}
};

void init(void) {
    sKeyboardEntry[0] = '\0';
    sKeyboardCharLimit = 12;
    sKeyboardCursorPos = 1;
    sKeyboardUpperCase = false;
    sKeyboardShift = false;
    sKeyboardPos[0] = (display_get_width() / 2) - (240 / 2);
    sKeyboardPos[1] = display_get_height() - 96;
}

void loop(int updateRate, float updateRateF) {
    handle_menu_stick_input(updateRate, MENUSTICK_STICKX | MENUSTICK_STICKY | MENUSTICK_WRAPX | MENUSTICK_WRAPY, &sKeyboardKeyPos[0], &sKeyboardKeyPos[1], 0, 0, 10, 4);

    if (input_pressed(INPUT_A, 3)) {
        int p = sKeyboardCursorPos - 1;
        input_clear(INPUT_A);
        if (sKeyboardKeyPos[0] < 10) {
            int pos = sKeyboardKeyPos[0] + (sKeyboardKeyPos[1] * 10) + sKeyboardKeyPos[1];
            sKeyboardEntry[p] = sKeyboardString[(int) sKeyboardUpperCase][pos];
            sKeyboardEntry[p + 1] = '\0';
            sKeyboardCursorPos++;
        } else {
            switch (sKeyboardKeyPos[1]) {
            case 0:
                if (sKeyboardCursorPos > 0) {
                    sKeyboardEntry[p] = '\0';
                    sKeyboardCursorPos--;
                }
                break;
            case 2:
                sKeyboardShift ^= 1;
                // fallthrough
            case 1: 
                sKeyboardUpperCase ^= 1;
                break;
            }
        }
    } else if (input_pressed(INPUT_B, 3)) {
        int p = sKeyboardCursorPos - 1;
        input_clear(INPUT_B);
        if (sKeyboardCursorPos > 0) {
            sKeyboardEntry[p] = '\0';
            sKeyboardCursorPos--;
        }
    }
}

void render(int updateRate, float updateRateF) {
    int screenW = display_get_width();
    int screenH = display_get_height();
    int sineCol = 128 + (32 * sins(gGameTimer * 0x400));
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_set_prim_color(RGBA32(0, 0, 0, 192));
    rdpq_fill_rectangle(sKeyboardPos[0], sKeyboardPos[1], sKeyboardPos[0] + 248, sKeyboardPos[1] + 96);

    int x = sKeyboardPos[0] + 8;
    int y = sKeyboardPos[1] + 8;
    rdpq_set_prim_color(RGBA32(192, 192, 192, 144));
    for (int i = 0; sKeyboardString[(int) sKeyboardUpperCase][i] != '\0'; i++) {
        if (sKeyboardString[(int) sKeyboardUpperCase][i] != '\n') {
            rdpq_fill_rectangle(x, y, x + 16, y + 16);
            x += 20;
        } else {
            x = sKeyboardPos[0] + 8;
            y += 20;
        }
    }

    x = sKeyboardPos[0] + 208;
    y = sKeyboardPos[1] + 8;
    for (int i = 0; i < 4; i++) {
        rdpq_fill_rectangle(x, y, x + 32, y + 16);
        y += 20;
    }

    x = sKeyboardPos[0] + 9;
    y = sKeyboardPos[1] + 21;
    int prevColour = 0;
    char str[3] = "\0\0\0";
    rdpq_textparms_t parms = {
        .width = 16,
        .height = 0,
        .align = ALIGN_CENTER,
        .valign = VALIGN_CENTER,
    };
    rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(0, 0, 0, 255),});
    for (int i = 0; sKeyboardString[(int) sKeyboardUpperCase][i] != '\0'; i++) {
        if (sKeyboardString[(int) sKeyboardUpperCase][i] != '\n') {
            if (sKeyboardString[(int) sKeyboardUpperCase][i] == ' ') {
                str[0] = 'S';
                str[1] = 'P';
            } else {
                str[0] = sKeyboardString[(int) sKeyboardUpperCase][i];
                str[1] = '\0';
            }
            rdpq_text_print(&parms, FONT_MVBOLI, x, y, str);
            x += 20;
        } else {
            x = sKeyboardPos[0] + 9;
            y += 20;
        }
    }
    x = sKeyboardPos[0] + 8;
    y = sKeyboardPos[1] + 20;
    for (int i = 0; sKeyboardString[(int) sKeyboardUpperCase][i] != '\0'; i++) {
        int pos;
        if (sKeyboardString[(int) sKeyboardUpperCase][i] != '\n') {
            int colour;
            if (sKeyboardString[(int) sKeyboardUpperCase][i] == ' ') {
                str[0] = 'S';
                str[1] = 'P';
            } else {
                str[0] = sKeyboardString[(int) sKeyboardUpperCase][i];
                str[1] = '\0';
            }
            pos = sKeyboardKeyPos[0] + (sKeyboardKeyPos[1] * 10) + sKeyboardKeyPos[1];
            if (i == pos) {
                colour = sineCol;
            } else {
                colour = 255;
            }
            if (prevColour != colour) {
                prevColour = colour;
                rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(255, colour, colour, 255),});
            }
            rdpq_text_print(&parms, FONT_MVBOLI, x, y, str);
            x += 20;
        } else {
            x = sKeyboardPos[0] + 8;
            y += 20;
        }
    }

    parms.width = 32;
    x = sKeyboardPos[0] + 208;
    y = sKeyboardPos[1] + 20;
    for (int i = 0; i < 4; i++) {
        int colour;
        if (i == sKeyboardKeyPos[1] && sKeyboardKeyPos[0] == 10) {
            colour = sineCol;
        } else {
            colour = 255;
        }
        text_outline(&parms, x, y, sKeyboardNames[i][(int) gConfig.language], RGBA32(255, colour, colour, 255));
        y += 20;
    }

    char textBytes[sizeof(sKeyboardEntry) + 1];
    sprintf(textBytes, "%s|", sKeyboardEntry);
    text_outline(NULL, 64, 64, textBytes, RGBA32(255, 255, 255, 255));
}

void close(void) {

}