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

static sprite_t *sHealthSprite;
static rspq_block_t *sHealthBlock;
static rspq_block_t *sHealthEmptyBlock;
static unsigned short sPrevHealth;
static unsigned short sPrevHealthMax;
static short sHealthPosX;
static short sHealthPosY;

typedef struct SubtitleData {
    char text[128];
    int timer;
    unsigned char colour[4];
} SubtitleData;

int rdpq_font_width(rdpq_font_t *fnt, const char *text, int nch);

static SubtitleData sSubtitleStruct[4];
char gNumSubtitles = 0;
short sSubtitlePrintY[sizeof(sSubtitleStruct) / sizeof(SubtitleData)] = {
    194, 188, 176, 164
};
short sSubtitlePrintYTarget[sizeof(sSubtitleStruct) / sizeof(SubtitleData)];

// I know this works (mostly) It's stubbed so that it doesn't error for others.
void render_hud_subtitles(void) {
    /*int boxCoords[4];
    int textWidth = 0;
    int textHeights[sizeof(sSubtitleStruct) / sizeof(SubtitleData)];
    int printY = 0;
    int topY = 0;
    int opacity = 0;
    int curTextWidth[4];
    int screenWidth = display_get_width();
    int screenHeight = display_get_height();

    for (int i = 0; i < sizeof(sSubtitleStruct) / sizeof(SubtitleData); i++) {
        if (sSubtitleStruct[i].colour[3] == 0) {
            continue;
        }
        curTextWidth[i] = rdpq_font_width(gCurrentFont, sSubtitleStruct[i].text, strlen(sSubtitleStruct[i].text));
        textWidth = MAX(textWidth, curTextWidth[i]);
        textHeights[i] = 12;//get_text_height(&gfx, sSubtitleStruct[i].text);
        sSubtitlePrintYTarget[i] = screenHeight-36-printY-textHeights[i];
        topY = sSubtitlePrintY[i];
        opacity = MAX(sSubtitleStruct[i].colour[3] * 0.75f, opacity);
        printY += textHeights[i];
    }
    if (printY == 0) {
        return;
    }
    boxCoords[1] = topY - 4;
    boxCoords[0] = (screenWidth / 2) - (textWidth/2) - 4;
    boxCoords[2] = (screenWidth / 2) + (textWidth/2) + 4;
    boxCoords[3] = screenHeight - 32;
    rdpq_set_prim_color(RGBA32(0, 0, 0, opacity));
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    glEnable(GL_SCISSOR_TEST);
    rdpq_fill_rectangle(boxCoords[0], boxCoords[1], boxCoords[2], boxCoords[3]);
    //glScissor(boxCoords[0], display_get_height() - boxCoords[1], boxCoords[0] - boxCoords[2], boxCoords[1] - boxCoords[3]);
    rdpq_font_begin(RGBA32(0, 0, 0, 0));
    for (int i = 0; i < sizeof(sSubtitleStruct) / sizeof(SubtitleData); i++) {
        if (sSubtitleStruct[i].colour[3] == 0) {
            continue;
        }
        rdpq_set_prim_color(RGBA32(sSubtitleStruct[i].colour[0], sSubtitleStruct[i].colour[1], sSubtitleStruct[i].colour[2], sSubtitleStruct[i].colour[3]));
        rdpq_font_position((screenWidth / 2) - (curTextWidth[i] / 2), sSubtitlePrintY[i] + 10);
        rdpq_font_print(gCurrentFont, sSubtitleStruct[i].text);
    }
    rdpq_font_end();
    glDisable(GL_SCISSOR_TEST);*/
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

void generate_health_block(int hpMin, int hpMax) {
        int x = 0;
        int y = 0;
        int i;

        rspq_block_begin();
        rdpq_mode_push();
        rdpq_set_mode_standard();
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
        rdpq_set_prim_color(RGBA32(215, 40, 57, 192));
        rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);
        if (hpMin > 0) {
            for (i = 0; i < hpMin; i += 4) {
                if (i < hpMin - 4) {
                    rdpq_sprite_blit(sHealthSprite, 16 + x, 12 + y, &(rdpq_blitparms_t) {.scale_x = 0.33f, .scale_y = 0.33f});
                } else {
                    sHealthPosX = x;
                    sHealthPosY = y;
                }
                x += 12;
                y -= 1;
                if ((i / 4) % 8 == 7) {
                    y += 20;
                    x = 0;
                }
            }
        } else {
            i = 0;
        }
        sHealthBlock = rspq_block_end();
        rspq_block_begin();
        rdpq_mode_blender(0);
        rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
        rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
        for (; i < hpMax; i += 4) {
            rdpq_fill_rectangle(21 + x, 16 + y, 21 + x + 2, 16 + y + 2);
            x += 12;
            y -= 1;
            if ((i / 4) % 8 == 7) {
                y += 20;
                x = 0;
            }
        }
    rdpq_mode_pop();
    sHealthEmptyBlock = rspq_block_end();
    sPrevHealth = hpMin;
    sPrevHealthMax = hpMax;
}

void render_health(float updateRateF) {
    if (gPlayer && gPlayer->data) {
        PlayerData *data = (PlayerData *) gPlayer->data;

        if (sHealthSprite == NULL) {
            sHealthSprite = sprite_load("rom:/health.i8.sprite");
        }

        if (sHealthBlock == NULL || sHealthEmptyBlock == NULL || sPrevHealth != data->health || sPrevHealthMax != data->healthMax) {
            if (sHealthBlock) {
                rspq_block_free(sHealthBlock);
            }
            if (sHealthEmptyBlock) {
                rspq_block_free(sHealthEmptyBlock);
            }
            generate_health_block(data->health, data->healthMax);
        }
        if (data->health > 0) {
            rspq_block_run(sHealthBlock);
            int heartSpeed = (gGameTimer * 0x200);
            if (data->health < data->healthMax / 4) {
                heartSpeed *= 2.5f;
            } else if (data->health < data->healthMax / 2) {
                heartSpeed *= 1.5f;
            }
            float addSize = sins(heartSpeed);
            float heartScale = 0.450f + (addSize / 20.0f);
            if (addSize < -0.9f) {
                addSize = -0.9f;
            }
            int x = sHealthPosX;
            int y = sHealthPosY;
            switch(data->health % 4) {
            case 1: // Quarter
                rdpq_set_scissor(16 + x - addSize, 8 + y - addSize, 16 + x + 8, 8 + y + 8);
                rdpq_sprite_blit(sHealthSprite, 16 + x - addSize, 8 + y - addSize, &(rdpq_blitparms_t){.scale_x = heartScale, .scale_y = heartScale});
                break;
            case 2: // Half
                rdpq_set_scissor(16 + x - addSize, 0, 16 + x + 8, 100);
                rdpq_sprite_blit(sHealthSprite, 16 + x - addSize, 8 + y - addSize, &(rdpq_blitparms_t){.scale_x = heartScale, .scale_y = heartScale});
                break;
            case 3: // Three Quarters
                rdpq_set_scissor(16 + x - addSize, 0, 16 + x + 8, 100);
                rdpq_sprite_blit(sHealthSprite, 16 + x - addSize, 8 + y - addSize, &(rdpq_blitparms_t){.scale_x = heartScale, .scale_y = heartScale});
                rdpq_set_scissor(24 + x, 16 + y, 32 + x + addSize, 24 + y + addSize);
                rdpq_sprite_blit(sHealthSprite, 16 + x - addSize, 8 + y - addSize, &(rdpq_blitparms_t){.scale_x = heartScale, .scale_y = heartScale});
                break;
            case 0: // Full
                rdpq_sprite_blit(sHealthSprite, 16 + x - addSize, 8 + y - addSize, &(rdpq_blitparms_t){.scale_x = heartScale, .scale_y = heartScale});
                break;
            }
            rdpq_set_scissor(0, 0, display_get_width(), display_get_height()); 
        }
    }
    rspq_block_run(sHealthEmptyBlock);
}

void render_hud(int updateRate, float updateRateF) {
    render_health(updateRateF);
    process_subtitle_timers(updateRate, updateRateF);
    render_hud_subtitles();

    if (gCurrentController == -1) {
        rdpq_font_begin(RGBA32(255, 0, 0, 255));
        rdpq_font_position((gFrameBuffers->width / 2) - 40, (gFrameBuffers->height / 2) - 40);
        rdpq_font_print(gCurrentFont, "Press Start");
        rdpq_font_end();
    }

    
    /*static sprite_t *background;

    if (!background) {
        background = sprite_load("rom:/wheat.rgba32.sprite");
    }
    
    rdpq_mode_push();
    //rdpq_mode_tlut(TLUT_RGBA16);
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    //rdpq_tex_upload_tlut(sprite_get_palette(background), 0, 16);
    rdpq_sprite_blit(background, 120, 120, NULL);
    rdpq_mode_pop();*/
}