#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>

#include "hud.h"
#include "../include/global.h"

#include "object.h"
#include "main.h"
#include "input.h"
#include "math_util.h"

typedef struct SubtitleData {
    char text[128];
    int timer;
    unsigned char colour[4];
} SubtitleData;

int rdpq_font_width(rdpq_font_t *fnt, const char *text, int nch);

static SubtitleData sSubtitleStruct[4];
char gNumSubtitles = 0;
short sSubtitlePrintY[sizeof(sSubtitleStruct) / sizeof(SubtitleData)];
short sSubtitlePrintYTarget[sizeof(sSubtitleStruct) / sizeof(SubtitleData)];

void render_hud_subtitles(void) {
    int boxCoords[4];
    int textWidth = 0;
    int textHeights[sizeof(sSubtitleStruct) / sizeof(SubtitleData)];
    int printY = 0;
    int topY = 0;
    int opacity = 0;
    int curTextWidth[4];

    for (int i = 0; i < sizeof(sSubtitleStruct) / sizeof(SubtitleData); i++)
    {
        if (sSubtitleStruct[i].colour[3] == 0) {
            continue;
        }
        curTextWidth[i] = rdpq_font_width(gCurrentFont, sSubtitleStruct[i].text, strlen(sSubtitleStruct[i].text));
        textWidth = MAX(textWidth, curTextWidth[i]);
        textHeights[i] = 12;//get_text_height(&gfx, sSubtitleStruct[i].text);
        sSubtitlePrintYTarget[i] = display_get_height()-36-printY-textHeights[i];
        topY = sSubtitlePrintY[i];
        opacity = MAX(sSubtitleStruct[i].colour[3] * 0.75f, opacity);
        printY += textHeights[i];
    }
    if (printY == 0)
        return;
    boxCoords[1] = topY - 4;
    boxCoords[0] = (display_get_width() / 2) - (textWidth/2) - 4;
    boxCoords[2] = (display_get_width() / 2) + (textWidth/2) + 4;
    boxCoords[3] = display_get_height() - 32;
    rdpq_set_prim_color(RGBA32(0, 0, 0, opacity));
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    glEnable(GL_SCISSOR_TEST);
    rdpq_fill_rectangle(boxCoords[0], boxCoords[1], boxCoords[2], boxCoords[3]);
    //glScissor(boxCoords[0], display_get_height() - boxCoords[1], boxCoords[0] - boxCoords[2], boxCoords[1] - boxCoords[3]);
    for (int i = 0; i < sizeof(sSubtitleStruct) / sizeof(SubtitleData); i++) {
        if (sSubtitleStruct[i].colour[3] == 0) {
            continue;
        }
        rdpq_font_begin(RGBA32(sSubtitleStruct[i].colour[0], sSubtitleStruct[i].colour[1], sSubtitleStruct[i].colour[2], sSubtitleStruct[i].colour[3]));
        rdpq_font_position((display_get_width() / 2) - (curTextWidth[i] / 2), sSubtitlePrintY[i] + 10);
        rdpq_font_print(gCurrentFont, sSubtitleStruct[i].text);
        rdpq_font_end();
    }
    glDisable(GL_SCISSOR_TEST);
}

void process_subtitle_timers(int updateRate, float updateRateF) {
    if (gNumSubtitles == 0) {
        return;
    }
    for (int i = 0; i < sizeof(sSubtitleStruct) / sizeof(SubtitleData); i++) {
        if (sSubtitleStruct[i].colour[3] == 0) {
            continue;
        }
        sSubtitlePrintY[i] = approach(sSubtitlePrintY[i], sSubtitlePrintYTarget[i], 8.0f * updateRateF);
        if (sSubtitleStruct[i].timer > 0) {
            sSubtitleStruct[i].colour[3] = approach(sSubtitleStruct[i].colour[3], 0xFF, 50 * updateRateF);
            sSubtitleStruct[i].timer -= updateRate;
        } else {
            if ((sSubtitleStruct[i].colour[3] = approach(sSubtitleStruct[i].colour[3], 0, 25 * updateRateF)) == 0) {
                sSubtitleStruct[i].colour[3] = 0;
                sSubtitlePrintY[i] = display_get_height() - 36;
                sSubtitlePrintYTarget[i] = display_get_height() - 36;
                gNumSubtitles--;
            }
        }
    }
}

void add_subtitle(char *text, int timer, int colour) {
    int i;
    if (gNumSubtitles == sizeof(sSubtitleStruct) / sizeof(SubtitleData)) {
        for (i = 0; i < sizeof(sSubtitleStruct) / sizeof(SubtitleData) - 1; i++) {
            memcpy(&sSubtitleStruct[i], &sSubtitleStruct[i+1], sizeof(SubtitleData));
        }
        bzero(&sSubtitleStruct[3], sizeof(SubtitleData));
        gNumSubtitles--;
    }
    int selected = gNumSubtitles;
    while (sSubtitleStruct[i].colour[3] != 0 && selected < 3) {
        selected++;
    }
    bzero(&sSubtitleStruct[selected], sizeof(SubtitleData));
    memcpy(&sSubtitleStruct[selected].text, text, strlen(text));
    sSubtitleStruct[selected].timer = timer_int(timer);
    sSubtitleStruct[selected].colour[0] = (colour >> 24) & 0xFF;
    sSubtitleStruct[selected].colour[1] = (colour >> 16) & 0xFF;
    sSubtitleStruct[selected].colour[2] = (colour >> 8) & 0xFF;
    sSubtitleStruct[selected].colour[3] = 1;
    sSubtitlePrintY[selected] = MAX(display_get_height() - 24, sSubtitlePrintY[0] - 24);
    sSubtitlePrintY[selected] = MAX(sSubtitlePrintY[0], sSubtitlePrintY[1] - 24);
    sSubtitlePrintY[selected] = MAX(sSubtitlePrintY[1] - 24, sSubtitlePrintY[2] - 24);
    gNumSubtitles++;
}

void clear_subtitles(void) {
    bzero(&sSubtitleStruct, sizeof(sSubtitleStruct));
    for (int i = 0; i < sizeof(sSubtitleStruct) / sizeof(SubtitleData); i++) {
        sSubtitlePrintY[i] = display_get_height() - 24;
        sSubtitlePrintYTarget[i] = display_get_height() - 24;
    }
}

void render_health(void) {
    if (gPlayer && gPlayer->data) {
        PlayerData *data = (PlayerData *) gPlayer->data;

        rdpq_font_begin(RGBA32(255, 0, 0, 255));
        rdpq_font_position(16, 16);
        rdpq_font_printf(gCurrentFont, "HP: %d/%d", data->health, data->healthMax);
        rdpq_font_end();
    }
}

void render_hud(int updateRate, float updateRateF) {
    //render_health();
    process_subtitle_timers(updateRate, updateRateF);
    render_hud_subtitles();

    if (gCurrentController == -1) {
        rdpq_font_begin(RGBA32(255, 0, 0, 255));
        rdpq_font_position((gFrameBuffers->width / 2) - 40, (gFrameBuffers->height / 2) - 40);
        rdpq_font_print(gCurrentFont, "Press Start");
        rdpq_font_end();
    }
}