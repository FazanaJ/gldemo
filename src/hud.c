#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#include <malloc.h>

#include "hud.h"
#include "../include/global.h"

#include "object.h"
#include "main.h"
#include "input.h"
#include "math_util.h"
#include "debug.h"
#include "talk.h"
#include "scene.h"

static sprite_t *sHealthSprite;
static rspq_block_t *sHealthBlock;
static rspq_block_t *sHealthEmptyBlock;
static unsigned short sPrevHealth;
static unsigned short sPrevHealthMax;
static short sHealthPosX;
static short sHealthPosY;
char gScreenshotStatus;
char gTransitionTimer;
char gTransitionTarget;
char gTransitionType;
char gTransitionSceneOut;
int gTransitionScene;
surface_t gScreenshot;
sprite_t *gScreenshotSprite;

sprite_t *gPanelSprite[4];
rspq_block_t *gPanelBlock;

void generate_health_block(int hpMin, int hpMax, int hpBase) {
    int x = 0;
    int y = 0;
    int i;
    char c[3] = {215, 40, 57};
    int newColour = 0;
    
    if (sHealthSprite == NULL) {
        sHealthSprite = sprite_load(asset_dir("health.i8", DFS_SPRITE));
    }

    rspq_block_begin();
    glEnable(GL_MULTISAMPLE_ARB);
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
    glBegin(GL_QUADS);
    glColor3f(255, 255, 255);
    glVertex2i(12, 22 + offsetY);
    glVertex2i(14, 24 + offsetY);
    glVertex2i(21 + (maxLine * 12), 24 - (maxLine) + offsetY);
    glVertex2i(18 + (maxLine * 12), 22 - (maxLine) + offsetY);
    glEnd();
    rdpq_mode_push();
    rdpq_set_mode_standard();
    glDisable(GL_MULTISAMPLE_ARB);
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_set_prim_color(RGBA32(c[0], c[1], c[2], 192));
    rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);
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

void render_health_bg(int numHealth, int hpMax) {
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

void render_health(float updateRateF) {
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
}

void render_ztarget(void) {
    int targetPos;
    if (gConfig.regionMode == TV_PAL) {
        targetPos = gZTargetTimer * (1.5f * 1.2f);
    } else {
        targetPos = gZTargetTimer * 1.5f;
    }
    if (targetPos) {
        rdpq_set_mode_fill(RGBA32(0, 0, 0, 255));
        rdpq_mode_blender(0);
        rdpq_fill_rectangle(0, 0, display_get_width(), targetPos);
        rdpq_fill_rectangle(0, display_get_height() - targetPos, display_get_width(), display_get_height());
        rdpq_set_mode_standard();
    }
}

int get_text_height(char *text) {
    int textLen = strlen(text);
    int textHeight = 12;

    for (int i = 0; i < textLen; i++) {
        if (text[i] == '\n') {
            textHeight += 12;
        }
    }
    return textHeight;
}

typedef struct SubtitleData {
    char *text;
    short timer;
    unsigned char y;
    unsigned char opacity;
    short width;
    short height;
    struct SubtitleData *prev;
    struct SubtitleData *next;
} SubtitleData;

SubtitleData *sSubtitleHead;
SubtitleData *sSubtitleTail;
short sBoxWidth = 0;
short sBoxHeight = 0;

void add_subtitle(char *text, int timer) {
    int textLen = strlen(text);

    SubtitleData *s = malloc(sizeof(SubtitleData));
    s->text = malloc(textLen + 1);
    strcpy(s->text, text);

    if (sSubtitleHead == NULL) {
        sSubtitleHead = s;
        sSubtitleTail = s;
        s->y = 0;
        s->prev = NULL;
    } else {
        sSubtitleTail->next = s;
        s->prev = sSubtitleTail;
        s->y = sSubtitleTail->y;
        sSubtitleTail = s;
    }
    s->next = NULL;
    s->timer = timer;
    s->opacity = 0;
    rdpq_textparms_t parms = {
        .width = display_get_width(),
        .height = 0,
        .align = ALIGN_CENTER,
        .valign = VALIGN_BOTTOM,
        .line_spacing = -8,
    };
    rdpq_paragraph_t *layout = rdpq_paragraph_build(&parms, FONT_MVBOLI, s->text, &textLen);
    s->width = (((layout->bbox.x1 - layout->bbox.x0)) / 2) + 8;
    s->height = ((layout->bbox.y1 - layout->bbox.y0) / 2);
    rdpq_paragraph_free(layout);
}

void clear_subtitle(SubtitleData *subtitle) {
    if (subtitle == sSubtitleHead) {
        if (sSubtitleHead->next) {
            sSubtitleHead = sSubtitleHead->next;
            sSubtitleHead->prev = NULL;
        } else {
            sSubtitleHead = NULL;
            if (subtitle == sSubtitleTail) {
                sSubtitleTail = NULL;
            }
        }
    } else {
        if (subtitle == sSubtitleTail) {
            sSubtitleTail = sSubtitleTail->prev;
        }
        subtitle->prev->next = subtitle->next;
        if (subtitle->next) {
            subtitle->next->prev = subtitle->prev;
        }
    }
    if (subtitle->text) {
        free(subtitle->text);
    }
    free(subtitle);
}

void process_subtitle_timers(int updateRate, float updateRateF) {
    if (sSubtitleHead == NULL) {
        return;
    }
    SubtitleData *s = sSubtitleHead;

    int i = 0;
    sBoxWidth = 0;
    sBoxHeight = 0;
    while (s != NULL) {
        i += get_text_height(s->text);
        s->timer -= updateRate;
        s->y = approach(s->y, i, updateRate * 2);
        if (s->y + s->height > sBoxHeight) {
            sBoxHeight = s->y + s->height;
        }
        if (s->width > sBoxWidth) {
            sBoxWidth = s->width;
        }
        if (s->timer <= 0) {
            DECREASE_VAR(s->opacity, updateRate * 8, 0);
            if (s->opacity == 0) {
                SubtitleData *old = s;
                s = s->next;
                clear_subtitle(old);
            } else {
                s = s->next;
            }
        } else {
            INCREASE_VAR(s->opacity, updateRate * 8, 255);
            s = s->next;
        }
    }
}

void render_hud_subtitles(void) {
    if (sSubtitleHead == NULL) {
        return;
    }
    SubtitleData *s = sSubtitleHead;
    int screenMid = display_get_width() / 2;
    int screenBottom = display_get_height();
    int opacity = 0;
    rdpq_textparms_t parms = {
        .width = display_get_width(),
        .height = 0,
        .align = ALIGN_CENTER,
        .valign = VALIGN_BOTTOM,
        .line_spacing = -8,
    };

    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    while (s) {
        opacity = MAX(opacity, s->opacity);
        s = s->next;
    }
    s = sSubtitleHead;
    opacity *= 0.375f;
    rdpq_set_prim_color(RGBA32(0, 0, 0, opacity));

    rdpq_fill_rectangle(screenMid - sBoxWidth, screenBottom - 32 - sBoxHeight, screenMid + sBoxWidth, screenBottom - 32 - s->y + s->height);
    while (s) {        
        rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(255, 255, 255, s->opacity),});
        rdpq_text_printf(&parms, FONT_MVBOLI, 0, screenBottom - 32 - s->y, s->text);
        s = s->next;
    }
}

void init_hud(void) {
    gPanelSprite[0] = sprite_load(asset_dir("message1.rgba16", DFS_SPRITE));
    gPanelSprite[1] = sprite_load(asset_dir("message2.rgba16", DFS_SPRITE));
    gPanelSprite[2] = sprite_load(asset_dir("message3.rgba16", DFS_SPRITE));
    gPanelSprite[3] = sprite_load(asset_dir("message4.rgba16", DFS_SPRITE));
}

void render_panel(int x1, int y1, int x2, int y2, int style, unsigned int colour) {
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_set_prim_color(RGBA32((colour >> 24) & 0xFF, (colour >> 16) & 0xFF, (colour >> 8) & 0xFF, colour & 0xFF));
    rdpq_mode_combiner(RDPQ_COMBINER_TEX_FLAT);

    // Corners
    surface_t surf = sprite_get_pixels(gPanelSprite[0]);
    rdpq_tex_blit(&surf, x1, y1, NULL);
    rdpq_tex_blit(&surf, x2 - 8, y1, &(rdpq_blitparms_t){.flip_x = true});
    rdpq_tex_blit(&surf, x1, y2 - 8, &(rdpq_blitparms_t){.flip_y = true});
    rdpq_tex_blit(&surf, x2 - 8, y2 - 8, &(rdpq_blitparms_t){.flip_x = true, .flip_y = true});
    // Borders
    float xScale = (float) ((x2 - 8) - (x1 + 8)) / 8.0f;
    float yScale = (float) ((y2 - 8) - (y1 + 8)) / 8.0f;
    surf = sprite_get_pixels(gPanelSprite[1]);
    rdpq_tex_blit(&surf, x1 + 8, y1, &(rdpq_blitparms_t){.scale_x = xScale});
    rdpq_tex_blit(&surf, x1 + 8, y2 - 9, &(rdpq_blitparms_t){.scale_x = xScale, .flip_y = true});
    surf = sprite_get_pixels(gPanelSprite[2]);
    rdpq_tex_blit(&surf, x1, y1 + 8, &(rdpq_blitparms_t){.scale_y = yScale});
    rdpq_tex_blit(&surf, x2 - 9, y1 + 8, &(rdpq_blitparms_t){.scale_y = yScale, .flip_x = true});
    surf = sprite_get_pixels(gPanelSprite[3]);
    xScale = (float) ((x2 - 9) - (x1 + 9)) / 8.0f;
    yScale = (float) ((y2 - 9) - (y1 + 9)) / 8.0f;
    rdpq_tex_blit(&surf, x1 + 8, y1 + 8, &(rdpq_blitparms_t){.scale_x = xScale, .scale_y = yScale});
}

void transition_into_scene(int sceneID, int transitionType, int timer, int transitionOut) {
    transition_set(transitionType, timer);
    gTransitionScene = sceneID;
    gTransitionSceneOut = transitionOut;
}

void transition_timer(int updateRate) {
    if (gTransitionType != TRANSITION_NONE) {
        gTransitionTimer += updateRate;
        if (gTransitionTimer > gTransitionTarget) {
            gTransitionTimer = gTransitionTarget;
            if (gTransitionScene != -1) {
                load_scene(gTransitionScene);
                gTransitionScene = -1;
                if (gTransitionSceneOut) {
                    transition_set(gTransitionSceneOut, gTransitionTarget);
                }
                return;
            }
            if ((gTransitionType % 2) == 0) {
                transition_clear();
            }
        }
    }
}

void transition_set(int type, int timer) {
    gTransitionType = type;
    gTransitionTarget = timer;
    gTransitionTimer = 0;
}

inline void transition_clear(void) {
    gTransitionType = TRANSITION_NONE;
    gTransitionScene = -1;
}

void process_hud(int updateRate, float updateRateF) {
    transition_timer(updateRate);
    process_subtitle_timers(updateRate, updateRateF);
    talk_update(updateRate);
}

void screenshot_generate(void) {
    if (gScreenshot.buffer) {
        surface_free(&gScreenshot);
    }
    gScreenshot = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());
    rdpq_attach_clear(&gScreenshot, &gZBuffer);
    
}

void screenshot_clear(void) {
    surface_free(&gScreenshot);
    gScreenshotStatus = SCREENSHOT_NONE;
}

void transition_render_fullscreen(void) {
    int offset;
    if (gTransitionType == TRANSITION_FULLSCREEN_OUT) {
        offset = gTransitionTarget - gTransitionTimer;
    } else {
        offset = gTransitionTimer;
    }
    int alpha = ((float) offset / (float) gTransitionTarget) * 255.0f;
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_set_prim_color(RGBA32(0, 0, 0, alpha));
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_fill_rectangle(0, 0, display_get_width(), display_get_height());
}

void transition_render(void) {
    switch (gTransitionType) {
    case TRANSITION_NONE:
        return;
    case TRANSITION_FULLSCREEN_IN:
    case TRANSITION_FULLSCREEN_OUT:
        transition_render_fullscreen();
        return;
    }
}

void render_hud(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, display_get_width(), display_get_height(), 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (gPlayer) {
        render_ztarget();
        render_health(updateRateF);
    }
    render_hud_subtitles();

    if (get_input_pressed(INPUT_CDOWN, 0)) {
        add_subtitle("You have pressed C down!", 120);
    }
    if (get_input_pressed(INPUT_CLEFT, 0)) {
        add_subtitle("You have pressed C left!\nThat's quite an acomplishment right there.", 200);
    }
    if (get_input_pressed(INPUT_CUP, 0)) {
        add_subtitle("You have pressed C left!\nThat's quite an acomplishment right there.\nOh god, we have a THIRD line now?", 200);
    }

    transition_render();

    talk_render();

    if (gCurrentController == -1) {
        render_panel((gFrameBuffers->width / 2) - 64, (gFrameBuffers->height / 2) - 64, (gFrameBuffers->width / 2) + 64, (gFrameBuffers->height / 2) - 32, 0, 0xFFFFFFFF);
        rdpq_text_printf(NULL, FONT_MVBOLI, (gFrameBuffers->width / 2) - 40, (gFrameBuffers->height / 2) - 40, "Press Blart");
    }
    get_time_snapshot(PP_HUD, DEBUG_SNAPSHOT_1_END);
}