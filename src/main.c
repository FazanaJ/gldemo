#include <libdragon.h>
#include <malloc.h>

#include "main.h"
#include "../include/global.h"

#include "camera.h"
#include "render.h"
#include "textures.h"
#include "plane.h"
#include "dummy_low.h"
#include "input.h"
#include "math_util.h"
#include "debug.h"
#include "player.h"
#include "object.h"
#include "hud.h"
#include "audio.h"

surface_t gZBuffer;
surface_t *gFrameBuffers;
rdpq_font_t *gCurrentFont;
unsigned int gGlobalTimer = 0;
unsigned int gGameTimer = 0;
float gAspectRatio = 1.0f;
char gZTargetTimer = 0;

GLenum shade_model = GL_SMOOTH;


const char *texture_path[] = {
    "rom:/grass0.ci8.sprite",
    "rom:/skin0.ci8.sprite",
    "rom:/health.i8.sprite",
    "rom:/plant1.sprite",
};

const char *gFontAssetTable[] = {
    "rom:/arial.font64"
};

#define TEXTURE_NUMBER sizeof(texture_path) / sizeof(char*)
#define FONT_NUMBER sizeof(gFontAssetTable) / sizeof(char*)

GLuint textures[TEXTURE_NUMBER];
sprite_t *sprites[TEXTURE_NUMBER];

const resolution_t RESOLUTION_304x224 = {304, 224, false};
const resolution_t RESOLUTION_384x224 = {384, 224, false};
const resolution_t RESOLUTION_408x224 = {408, 224, false};
Config gConfig;

light_t light = {
    
    color: { 0.51f, 0.81f, 0.665f, 0.5f},
    diffuse: {1.0f, 1.0f, 1.0f, 1.0f},
    direction: {0.0f, -60.0f, 0.0f},
    position: {1.0f, 0.0f, 0.0f, 0.0f},
    radius: 10.0f,
};

void setup(void) {

    setup_textures(textures, sprites, texture_path, TEXTURE_NUMBER);

    setup_plane();
    make_plane_mesh();

    setup_light(light);
    setup_fog(light);

    gPlayer = spawn_object_pos(OBJ_PLAYER, 0.0f, 0.0f, 0.0f);
    spawn_object_pos(OBJ_BUSH, 4, 8, 0);
    spawn_object_pos(OBJ_BUSH, 2, 9, 0);
    spawn_object_pos(OBJ_BUSH, 7, -20, 0);
    spawn_object_pos(OBJ_BUSH, 15, -5, 0);
    spawn_object_pos(OBJ_BUSH, 12, 8, 0);
    spawn_object_pos(OBJ_BUSH, -5, 9, 0);
    spawn_object_pos(OBJ_BUSH, -19, 8, 0);
    spawn_object_pos(OBJ_BUSH, 9, 0, 0);
    spawn_object_pos(OBJ_BUSH, 15, -15, 0);

    camera_init();
    gAspectRatio = (float) display_get_width() / (float) display_get_height();
}

void apply_render_settings(void) {
    switch (gConfig.antiAliasing) {
    case AA_OFF:
        glDisable(GL_MULTISAMPLE_ARB);
        break;
    case AA_FAST:
        glEnable(GL_MULTISAMPLE_ARB);
        break;
    case AA_FANCY:
        glEnable(GL_MULTISAMPLE_ARB);
        break;
    }
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    if (gEnvironment->flags & ENV_FOG) {
        glEnable(GL_FOG);
    }
}

void render_sky(void) {
    Environment *e = gEnvironment;
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, display_get_width(), display_get_height(), 0.0f, -1.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glBegin(GL_QUADS);
    glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
    glVertex2i(0, 0);
    glColor3f(e->skyColourBottom[0], e->skyColourBottom[1], e->skyColourBottom[2]);
    glVertex2i(0, display_get_height());
    glVertex2i(display_get_width(), display_get_height());
    glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
    glVertex2i(display_get_width(), 0);
    glEnd();
}

void project_camera(void) {
    Camera *c = gCamera;
    float nearClip = 1.0f;
    float farClip = 100.0f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-nearClip * gAspectRatio, nearClip * gAspectRatio, -nearClip, nearClip, nearClip, farClip);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(c->pos[0], c->pos[1], c->pos[2], c->focus[0], c->focus[1], c->focus[2], 0.0f, 0.0f, 1.0f);
}

void render_bush(void) {
    Environment *e = gEnvironment;
    glPushMatrix();
    glBegin(GL_QUADS);
    glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
    glTexCoord2f(0, 0);
    glVertex3i(-1, 0, 2);
    glColor3f(e->skyColourBottom[0], e->skyColourBottom[1], e->skyColourBottom[2]);
    glTexCoord2f(0, 1.024f);
    glVertex3i(-1, 0, 0);
    glTexCoord2f(1.024f, 1.024f);
    glVertex3i(1, 0, 0);
    glColor3f(e->skyColourTop[0], e->skyColourTop[1], e->skyColourTop[2]);
    glTexCoord2f(1.024f, 0);
    glVertex3i(1, 0, 2);
    glEnd();
    glPopMatrix();
}

void render_game(void) {
    DEBUG_SNAPSHOT_1();
    rdpq_attach(gFrameBuffers, &gZBuffer);
    glShadeModel(shade_model);
    gl_context_begin();

    glClear(GL_DEPTH_BUFFER_BIT);
    render_sky();
    project_camera();
    apply_render_settings();
    glEnable(GL_TEXTURE_2D);
    set_light(light);

    glPushMatrix();
	glTranslatef(gPlayer->pos[0], gPlayer->pos[1], gPlayer->pos[2]);
    glRotatef(SHORT_TO_DEGREES(gPlayer->faceAngle[2]), 0, 0, 1);
	glScalef(1.f, 1.f, 1.f);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    render_dummy(); 
    glPopMatrix();

    ObjectList *list = gObjectListHead;
    Object *obj;

    glAlphaFunc(GL_GREATER, 0.5);
    glEnable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, textures[3]);
    while (list) {
        obj = list->obj;
        if (obj->objectID == OBJ_BUSH) {
            glPushMatrix();
            glTranslatef(obj->pos[0], obj->pos[1], obj->pos[2]);
            glRotatef(SHORT_TO_DEGREES(gCamera->yaw), 0, 0, 1);
            glScalef(obj->scale[0], obj->scale[1], obj->scale[2]);
            render_bush(); 
            glPopMatrix();
        }
        list = list->next;
    }
    glDisable(GL_ALPHA_TEST);

    glBindTexture(GL_TEXTURE_2D, textures[0]);
    render_plane();

    get_time_snapshot(PP_RENDER, DEBUG_SNAPSHOT_1_END);
    DEBUG_SNAPSHOT_1_RESET();
    render_end();
    
    gl_context_end();
    if (gZTargetTimer) {
        int outWard;
        if (gConfig.regionMode == TV_PAL) {
            outWard = gZTargetTimer * (1.5f * 1.2f);
        } else {
            outWard = gZTargetTimer * 1.5f;
        }
        rdpq_set_mode_fill(RGBA32(0, 0, 0, 255));
        rdpq_mode_blender(0);
        rdpq_fill_rectangle(0, 0, display_get_width(), outWard);
        rdpq_fill_rectangle(0, display_get_height() - outWard, display_get_width(), display_get_height());
        rdpq_set_mode_standard();
    }
    render_hud();
    get_time_snapshot(PP_HUD, DEBUG_SNAPSHOT_1_END);
}

void init_video(void) {
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    gZBuffer = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());
    rdpq_init();
    gl_init();
}

void init_memory(void) {
    dfs_init(DFS_DEFAULT_LOCATION);
    timer_init();
    gCurrentFont = rdpq_font_load(gFontAssetTable[0]);
}

void init_save(void) {
    gConfig.antiAliasing = AA_FAST;
    gConfig.dedither = false;
    gConfig.regionMode = get_tv_type();
    gConfig.screenMode = SCREEN_NORMAL;
}

void init_game(void) {
    init_memory();
    init_controller();
    init_video();
    init_audio();
    init_save();
    init_debug();
}

int update_game_time(void) {
    static unsigned int prevTime = 0;
    static unsigned int deltaTime = 0;
    static unsigned int curTime;
    int updateRate;
    
    curTime = timer_ticks();
    deltaTime += TIMER_MICROS(curTime - prevTime);
    prevTime = curTime;
    deltaTime -= 16666;
    updateRate = LOGIC_60FPS;
    while (deltaTime > 16666) {
        deltaTime -= 16666;
        updateRate++;
        if (updateRate == LOGIC_15FPS) {
            deltaTime = 0;
        }
    }
    return updateRate;
}

int main(void) {
    int updateRate;
    float updateRateF;

    init_game();    
    gCurrentFont = rdpq_font_load(gFontAssetTable[0]);
    setup();

    while (1) {
        gFrameBuffers = display_get();
        reset_profiler_times();
        DEBUG_SNAPSHOT_1();

        updateRate = update_game_time();
        // Convert it to float too, and make it 20% faster on PAL systems.
        updateRateF = (float) updateRate;
        if (gConfig.regionMode == TV_PAL) {
            updateRateF *= 1.2f;
        }

        update_inputs(updateRate);
        update_objects(updateRate, updateRateF);
        camera_loop(updateRate, updateRateF);
        audio_loop(updateRate, updateRateF);
        
        render_game();
        get_cpu_time(DEBUG_SNAPSHOT_1_END);
        
        process_profiler();
        if (gDebugData && gDebugData->enabled) {
            render_profiler();
        }
        rdpq_detach_wait();
        display_show(gFrameBuffers);
        
        gGlobalTimer++;
        gGameTimer += updateRate;
    }

}
