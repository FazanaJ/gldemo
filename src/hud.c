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
#include "camera.h"
#include "render.h"
#include "menu.h"

char gTransitionTimer;
char gTransitionTarget;
char gTransitionType;
char gTransitionSceneOut;
char gCameraHudToggle;
int gTransitionScene;

sprite_t *gPanelSprite[4];
rspq_block_t *gPanelBlock;

void text_outline(rdpq_textparms_t *parms, int x, int y, char *text, color_t colour) {
    rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(0, 0, 0, 255),});
    rdpq_text_print(parms, FONT_MVBOLI, x + 1, y + 1, text);
    rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = colour,});
    rdpq_text_print(parms, FONT_MVBOLI, x, y, text);
}

static void render_ztarget(void) {
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

static void process_subtitle_timers(int updateRate, float updateRateF) {
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

static void render_hud_subtitles(void) {
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
        .valign = VALIGN_TOP,
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
        rdpq_text_print(&parms, FONT_MVBOLI, 0, screenBottom - 32 - s->y, s->text);
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
    if ((colour & 0xFF) != 255) {
        rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    } else {
        rdpq_mode_blender(0);
    }
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

static void transition_timer(int updateRate) {
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

static void transition_render_fullscreen(void) {
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

static void transition_render(void) {
    switch (gTransitionType) {
    case TRANSITION_NONE:
        return;
    case TRANSITION_FULLSCREEN_IN:
    case TRANSITION_FULLSCREEN_OUT:
        transition_render_fullscreen();
        return;
    }
}

static void render_camera_hud(void) {
    if (gCameraHudToggle == 0) {
        return;
    }
    char *text = "A: Toggle text\nL/Z: Down\nR: Up\nC: Look\nStick: Move\nDpad Up/Down: FoV";
    text_outline(NULL, 32, 32, text, RGBA32(255, 255, 255, 255));
}

static char sRenderHealth = false;
static char sRenderMinimap = false;

/*void overlay_run(int updateRate, int updateRateF, char *name, int var, void (**func)(float, int), void **ovl) {
    if (var) {
        if (*ovl == NULL) {
            //debugf("Loading overlay: [%s]\n", name);
            *ovl = dlopen(asset_dir(name, DFS_OVERLAY), RTLD_LOCAL);
            void (*init)() = dlsym(*ovl, "init");
            (*init)();
            *func = dlsym(*ovl, "loop");
        }
        (**func)(updateRate, updateRateF);
    } else {
        if (*ovl != NULL) {
            //debugf("Closing overlay: [%s]\n", name);
            void (*destroy)() = dlsym(*ovl, "destroy");
            (*destroy)();
            dlclose(*ovl);
            *ovl = NULL;
        }
    }
}*/

void hud_healthbar(float updateRateF) {
    static void *ovl = NULL;
    static void (*func)(int, float);
    //overlay_run(0, updateRateF, "healthbar", sRenderHealth, &func, &ovl);


    if (sRenderHealth) {
        if (ovl == NULL) {
            ovl = dlopen(asset_dir("healthbar", DFS_OVERLAY), RTLD_LOCAL);
            void (*init)() = dlsym(ovl, "init");
            (*init)();
            func = dlsym(ovl, "loop");
        }
        (*func)(0, updateRateF);
    } else {
        if (ovl != NULL) {
            void (*destroy)() = dlsym(ovl, "destroy");
            (*destroy)();
            dlclose(ovl);
            ovl = NULL;
        }
    }
}

void hud_minimap(void) {
    static void *ovl = NULL;
    static void (*func)(int, float);
    //overlay_run(0, updateRateF, "healthbar", sRenderHealth, &func, &ovl);


    if (sRenderMinimap) {
        if (ovl == NULL) {
            ovl = dlopen(asset_dir("minimap", DFS_OVERLAY), RTLD_LOCAL);
            void (*init)() = dlsym(ovl, "init");
            (*init)();
            func = dlsym(ovl, "loop");
        }
        (*func)(0, 0);
    } else {
        if (ovl != NULL) {
            void (*destroy)() = dlsym(ovl, "destroy");
            (*destroy)();
            dlclose(ovl);
            ovl = NULL;
        }
    }
}

void render_hud(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    if (gCamera->mode == CAMERA_PHOTO) {
        render_camera_hud();
        return;
    }
    sRenderHealth = false;
    sRenderMinimap = false;
    matrix_ortho();
    if (gPlayer && gMenuStatus == MENU_CLOSED) {
        render_ztarget();
        //render_health(updateRateF);
        sRenderHealth = true;
        sRenderMinimap = true;
    }
    hud_healthbar(updateRateF);
#if OPENGL
    //hud_minimap();
#endif
    render_hud_subtitles();

    if (input_pressed(INPUT_CDOWN, 0)) {
        add_subtitle("You have pressed C down!", 120);
    }
    if (input_pressed(INPUT_CLEFT, 0)) {
        add_subtitle("You have pressed C left!\nThat's quite an acomplishment right there.", 200);
    }
    if (input_pressed(INPUT_CUP, 0)) {
        add_subtitle("You have pressed C left!\nThat's quite an acomplishment right there.\nOh god, we have a THIRD line now?", 200);
    }

    transition_render();
    talk_render();

    if (gCurrentController == -1) {
        render_panel((gFrameBuffers->width / 2) - 64, (gFrameBuffers->height / 2) - 64, (gFrameBuffers->width / 2) + 64, (gFrameBuffers->height / 2) - 32, 0, 0xFFFFFFFF);
        text_outline(NULL, (gFrameBuffers->width / 2) - 40, (gFrameBuffers->height / 2) - 40, "Press Blart", RGBA32(0, 0, 0, 255));
    }

    /*char textBytes[12];
    static int balls = 0;

    if (input_pressed(INPUT_CRIGHT, 3)) {
        input_clear(INPUT_CRIGHT);
        balls++;

        switch (balls) {
            case 0:
            display_set_fps_limit(60.0f);
            break;
            case 1:
            display_set_fps_limit(40.0f);
            break;
            case 2:
            display_set_fps_limit(33.3333f);
            break;
            case 3:
            display_set_fps_limit(30.0f);
            break;
            case 4:
            display_set_fps_limit(20.0f);
            break;
            case 5:
            display_set_fps_limit(15.0f);
            break;
            case 6:
            display_set_fps_limit(60.0f);
            balls = 0;
            break;
        }
    }

    sprintf(textBytes, "FPS: %2.2f", (double) display_get_fps());
    text_outline(NULL, 32, 80, textBytes, RGBA32(255, 255, 255, 255));*/

    get_time_snapshot(PP_HUD, DEBUG_SNAPSHOT_1_END);
}