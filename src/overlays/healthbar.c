#include <libdragon.h>
#include <malloc.h>

#include "../../include/global.h"

#include "../assets.h"
#include "../object.h"
#include "../main.h"
#include "../math_util.h"

static sprite_t *sHealthSprite;
static rspq_block_t *sHealthBlock;
static rspq_block_t *sHealthEmptyBlock;
static unsigned short sPrevHealth;
static unsigned short sPrevHealthMax;
static short sHealthPosX;
static short sHealthPosY;

static void generate_health_block(int hpMin, int hpMax, int hpBase) {
    int x = 0;
    int y = 0;
    int i;
    char c[3] = {215, 40, 57};
    int newColour = 0;

    int maxLine = MAX((hpMax + 3) / 4, (hpMin + 3) / 4);
    if (maxLine > 8) {
        maxLine = 8;
    }
    int offsetY = 0;
    int offsetCount = MAX(hpMax, hpMin);
    while (offsetCount > 32) {
        offsetCount -= 32;
        offsetY += 10;
        if (offsetY == 10) {
            break;
        }
    }
    rspq_block_begin();
    rdpq_mode_push();
    rdpq_sync_pipe();
    rdpq_mode_antialias(AA_STANDARD);
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_set_prim_color(RGBA32(255, 255, 255, 255));
    rdpq_mode_blender(0);
    /*rdpq_triangle(&TRIFMT_FILL,
        (float[]){12.0f, 22.0f + offsetY},
        (float[]){14.0f, 24.0f + offsetY},
        (float[]){21.0f + (maxLine * 12.0f), 24.0f - (maxLine) + offsetY}
    );
    rdpq_triangle(&TRIFMT_FILL,
        (float[]){21.0f + (maxLine * 12.0f), 24.0f - (maxLine) + offsetY},
        (float[]){18.0f + (maxLine * 12.0f), 22.0f - (maxLine) + offsetY},
        (float[]){12.0f, 22.0f + offsetY}
    );*/
    rdpq_mode_antialias(AA_NONE);
    rdpq_sync_pipe();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_set_prim_color(RGBA32(c[0], c[1], c[2], 192));
    rdpq_set_combiner_raw(RDPQ_COMBINER1((TEX0,PRIM,TEX0,PRIM), (TEX0,0,PRIM,0)));
    surface_t surf = sprite_get_pixels(sHealthSprite);
    if (hpMin > 0) {
        for (i = 0; i < hpMin; i += 4) {
            if (i > hpBase - 4) {
                //rdpq_set_prim_color(RGBA32(64, 215, 215, 192));
            }
            if (i < hpMin - 4) {
                rdpq_tex_blit(&surf, 16 + x, 12 + y, &(rdpq_blitparms_t) {.scale_x = 0.33f, .scale_y = 0.33f});
            } else {
                sHealthPosX = x;
                sHealthPosY = y;
            }
            x += 12;
            y -= 1;
            if ((i / 4) % 8 == 7) {
                y += 18;
                x = 0;
                if (y > 16) {
                    if (i < hpMin - 4) {
                        if (newColour == 0) {
                            c[0] = 255;
                            c[1] = 255;
                            c[2] = 0;
                        } else {
                            c[0] = 255;
                            c[1] = 128;
                            c[2] = 0;
                        }
                        rdpq_set_prim_color(RGBA32(c[0], c[1], c[2], 192));
                    }
                    x = 0;
                    y = 0;
                    newColour++;
                }
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
            y += 18;
            x = 0;
            if (y > 16) {
                x = 0;
                y = 0;
            }
        }
    }
    rdpq_mode_pop();
    sHealthEmptyBlock = rspq_block_end();
    sPrevHealth = hpMin;
    sPrevHealthMax = hpMax;
}

static void render_health_bg(int numHealth, int hpMax) {
    rdpq_debug_start();
    PlayerData *data = (PlayerData *) gPlayer->data;
    int heartSpeed = (gGameTimer * 0x200);
    if (data->health <= data->healthMax / 4) {
        heartSpeed *= 2.5f;
    } else if (data->health <= data->healthMax / 2) {
        heartSpeed *= 1.5f;
    }
    float addSize = sins(heartSpeed);
    float heartScale = 0.450f + (addSize / 20.0f);
    if (addSize < -0.9f) {
        addSize = -0.9f;
    }
    int x = sHealthPosX;
    int y = sHealthPosY;
    surface_t surf = sprite_get_pixels(sHealthSprite);
    rdpq_set_prim_color(RGBA32(0, 0, 0, 160));
    // If this isn't even the last heart then skip the funnies
    if ((data->healthMax / 4) < (data->health / 4)) {
        return;
    } else if ((data->healthMax / 4) != (data->health / 4)) {
        goto fullHealth;
    }
    switch (hpMax % 4) {
    case 0: // 4/4
        fullHealth:
        // Draw right half
        if (numHealth == 1) {
            rdpq_set_scissor(24 + x, 8 + y - addSize, 32 + x + addSize, 16 + y);
            rdpq_tex_blit(&surf, 16 + x - addSize, 8 + y - addSize, &(rdpq_blitparms_t){.scale_x = heartScale, .scale_y = heartScale});
        } else {
            rdpq_set_scissor(24 + x, 0, 24 + x + 8 + addSize, 100);
            rdpq_tex_blit(&surf, 16 + x - addSize, 8 + y - addSize, &(rdpq_blitparms_t){.scale_x = heartScale, .scale_y = heartScale});
        }
        firstQuart:
        if (numHealth == 3) {
            rdpq_set_scissor(16 + x - addSize, 16 + y, 24 + x, 24 + y + addSize);
            rdpq_tex_blit(&surf, 16 + x - addSize, 8 + y - addSize, &(rdpq_blitparms_t){.scale_x = heartScale, .scale_y = heartScale});
        }
        break;
    case 3: // 3/4
        if (numHealth != 1) {
            rdpq_set_scissor(24 + x, 16 + y, 32 + x + addSize, 24 + y + addSize);
            rdpq_tex_blit(&surf, 16 + x - addSize, 8 + y - addSize, &(rdpq_blitparms_t){.scale_x = heartScale, .scale_y = heartScale});
        }
        goto firstQuart;
        break;
    case 2: // 2/4
        goto firstQuart;
        break;
    }    
}

void loop(float updateRateF) {
    if (gPlayer && gPlayer->data) {
        PlayerData *data = (PlayerData *) gPlayer->data;

        if (sHealthBlock == NULL || sHealthEmptyBlock == NULL || sPrevHealth != data->health || sPrevHealthMax != data->healthMax) {
            if (sHealthBlock) {
                rspq_block_free(sHealthBlock);
            }
            if (sHealthEmptyBlock) {
                rspq_block_free(sHealthEmptyBlock);
            }
            generate_health_block(data->health, data->healthMax, data->healthBase);
        }
        rspq_block_run(sHealthBlock);
        if (data->health > 0) {
            int heartSpeed = (gGameTimer * 0x200);
            if (data->health <= data->healthMax / 4) {
                heartSpeed *= 2.5f;
            } else if (data->health <= data->healthMax / 2) {
                heartSpeed *= 1.5f;
            }
            float addSize = sins(heartSpeed);
            float heartScale = 0.450f + (addSize / 20.0f);
            if (addSize < -0.9f) {
                addSize = -0.9f;
            }
            int x = sHealthPosX;
            int y = sHealthPosY;
            surface_t surf = sprite_get_pixels(sHealthSprite);
            switch(data->health % 4) {
            case 1: // Quarter
                rdpq_set_scissor(16 + x - addSize, 8 + y - addSize, 16 + x + 8, 8 + y + 8);
                rdpq_tex_blit(&surf, 16 + x - addSize, 8 + y - addSize, &(rdpq_blitparms_t){.scale_x = heartScale, .scale_y = heartScale});
                render_health_bg(3, data->healthMax);
                break;
            case 2: // Half
                rdpq_set_scissor(16 + x - addSize, 0, 16 + x + 8, 100);
                rdpq_tex_blit(&surf, 16 + x - addSize, 8 + y - addSize, &(rdpq_blitparms_t){.scale_x = heartScale, .scale_y = heartScale});
                render_health_bg(2, data->healthMax);
                break;
            case 3: // Three Quarters
                rdpq_set_scissor(16 + x - addSize, 0, 16 + x + 8, 100);
                rdpq_tex_blit(&surf, 16 + x - addSize, 8 + y - addSize, &(rdpq_blitparms_t){.scale_x = heartScale, .scale_y = heartScale});
                rdpq_set_scissor(24 + x, 16 + y, 32 + x + addSize, 24 + y + addSize);
                rdpq_tex_blit(&surf, 16 + x - addSize, 8 + y - addSize, &(rdpq_blitparms_t){.scale_x = heartScale, .scale_y = heartScale});
                render_health_bg(1, data->healthMax);
                break;
            case 0: // Full
                rdpq_tex_blit(&surf, 16 + x - addSize, 8 + y - addSize, &(rdpq_blitparms_t){.scale_x = heartScale, .scale_y = heartScale});
                break;
            }
            rdpq_set_scissor(0, 0, display_get_width(), display_get_height()); 
        }
    }
    rspq_block_run(sHealthEmptyBlock);
    rdpq_debug_stop();
}

void init(void) {
    debugf("Loading overlay: [healthbar]\n");
    sHealthSprite = sprite_load(asset_dir(gTextureIDs[TEXTURE_HEALTH].file, DFS_SPRITE));
}

void destroy(void) {
    debugf("Closing overlay: [healthbar]\n");
    sprite_free(sHealthSprite);
}