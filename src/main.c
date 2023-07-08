#include <libdragon.h>
#include <malloc.h>

#include "main.h"
#include "../include/global.h"

#include "camera.h"
#include "render.h"
#include "assets.h"
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
unsigned int gGlobalTimer;
unsigned int gGameTimer;
char gZTargetTimer = 0;
char gZTargetOut = 0;

GLenum shade_model = GL_SMOOTH;

const char *gFontAssetTable[] = {
    "rom:/arial.font64"
};

static const resolution_t RESOLUTION_304x224 = {320, 240, false};
static const resolution_t RESOLUTION_384x224 = {384, 240, false};
static const resolution_t RESOLUTION_408x224 = {424, 240, false};

static const resolution_t sVideoModes[] = {
    RESOLUTION_304x224,
    RESOLUTION_384x224,
    RESOLUTION_408x224
};

Material gTempMaterials[] = {
    {NULL, 0, MATERIAL_DEPTH_WRITE | MATERIAL_FOG | MATERIAL_LIGHTING},
    {NULL, -1, MATERIAL_DEPTH_WRITE | MATERIAL_FOG | MATERIAL_LIGHTING | MATERIAL_VTXCOL},
    {NULL, 1, MATERIAL_DEPTH_WRITE | MATERIAL_FOG | MATERIAL_XLU | MATERIAL_LIGHTING},
    {NULL, 2, MATERIAL_DEPTH_WRITE | MATERIAL_FOG | MATERIAL_CUTOUT | MATERIAL_LIGHTING | MATERIAL_VTXCOL},
};

Config gConfig;


static model64_t *gPlayerModel;

light_t light = {
    
    color: { 0.51f, 0.81f, 0.665f, 0.5f},
    diffuse: {1.0f, 1.0f, 1.0f, 1.0f},
    direction: {0.0f, -60.0f, 0.0f},
    position: {1.0f, 0.0f, 0.0f, 0.0f},
    radius: 10.0f,
};

void init_renderer(void) {
    setup_light(light);
    setup_fog(light);
    init_materials();
    gPlayerModel = model64_load("rom:/humanoid.model64");
}

void setup(void) {

    init_renderer();
    setup_plane();
    make_plane_mesh();

    gPlayer = spawn_object_pos(OBJ_PLAYER, 0.0f, 0.0f, 0.0f);
    spawn_clutter(CLUTTER_BUSH, 4, 8, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, 2, 9, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, 7, -20, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, 15, -5, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, 12, 8, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, -5, 9, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, -19, 8, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, 9, 0, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, 15, -15, 0, 0, 0, 0);

    camera_init();
}

void apply_anti_aliasing(int mode) {
    switch (gConfig.antiAliasing) {
    case AA_OFF:
        glDisable(GL_MULTISAMPLE_ARB);
        rdpq_mode_antialias(AA_NONE);
        break;
    case AA_FAST:
        if (mode == AA_ACTOR) {
            goto mrFancyPants;
        }
        glEnable(GL_MULTISAMPLE_ARB);
        glHint(GL_MULTISAMPLE_HINT_N64, GL_FASTEST);
        rdpq_mode_antialias(AA_REDUCED);
        break;
    case AA_FANCY:
        mrFancyPants:
        glEnable(GL_MULTISAMPLE_ARB);
        glHint(GL_MULTISAMPLE_HINT_N64, GL_NICEST);
        rdpq_mode_antialias(AA_STANDARD);
        break;
    }
}

void apply_render_settings(void) {
    glAlphaFunc(GL_GREATER, 0.5f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


rspq_block_t *sPlayerBlock;
rspq_block_t *sBushBlock;
rspq_block_t *sPlaneBlock;

void render_game(void) {
    DEBUG_SNAPSHOT_1();
    rdpq_attach(gFrameBuffers, &gZBuffer);
    glShadeModel(shade_model);
    gl_context_begin();
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, gZTargetOut, display_get_width(), display_get_height() - (gZTargetOut * 2));
    glClear(GL_DEPTH_BUFFER_BIT);
    render_sky();
    apply_anti_aliasing(AA_ACTOR);
    project_camera();
    apply_render_settings();
    set_light(light);
    
    if (gConfig.regionMode == TV_PAL) {
        gZTargetOut = gZTargetTimer * (1.5f * 1.2f);
    } else {
        gZTargetOut = gZTargetTimer * 1.5f;
    }

    glPushMatrix();
	glTranslatef(gPlayer->pos[0], gPlayer->pos[1], gPlayer->pos[2]);
    glRotatef(SHORT_TO_DEGREES(gPlayer->faceAngle[2]), 0, 0, 1);
	glScalef(0.18f, 0.25f, 0.25f);
    set_material(&gTempMaterials[1]);
    rdpq_set_mode_standard();
    rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,SHADE),(0,0,0,1)));
    rdpq_mode_blender(RDPQ_BLENDER2((FOG_RGB, SHADE_ALPHA, IN_RGB, INV_MUX_ALPHA), (CYCLE1_RGB, IN_ALPHA, MEMORY_RGB, MEMORY_CVG)));
    if (sPlayerBlock == NULL) {
        rspq_block_begin();
        model64_draw(gPlayerModel);
        sPlayerBlock = rspq_block_end();
    }
    rspq_block_run(sPlayerBlock);
    glPopMatrix();


    ClutterList *list = gClutterListHead;
    Clutter *obj;
    
    ObjectList *list2 = gObjectListHead;
    Object *obj2;
    
    apply_anti_aliasing(AA_GEO);
    while (list) {
        obj = list->clutter;
        if (obj->objectID == CLUTTER_BUSH/* && !(obj->flags & OBJ_FLAG_INVISIBLE)*/) {
            glPushMatrix();
            
            set_material(&gTempMaterials[3]);
            glTranslatef(obj->pos[0], obj->pos[1], obj->pos[2]);
            glRotatef(SHORT_TO_DEGREES(gCamera->yaw), 0, 0, 1);
            //glScalef(obj->scale[0], obj->scale[1], obj->scale[2]);
            
            if (sBushBlock == NULL) {
                rspq_block_begin();
                render_bush(); 
                sBushBlock = rspq_block_end();
            }
            rspq_block_run(sBushBlock);
            glPopMatrix();
        }
        list = list->next;
    }

    apply_anti_aliasing(AA_ACTOR);
    set_material(&gTempMaterials[0]);
    if (sPlaneBlock == NULL) {
        rspq_block_begin();
        render_plane();
        sPlaneBlock = rspq_block_end();
    }
    rspq_block_run(sPlaneBlock);
    
    while (list2) {
        obj2 = list2->obj;
        if (obj2->objectID == OBJ_PROJECTILE) {
            glPushMatrix();
            set_material(&gTempMaterials[2]);
            glTranslatef(obj2->pos[0], obj2->pos[1], obj2->pos[2]);
            glRotatef(SHORT_TO_DEGREES(gCamera->yaw), 0, 0, 1);
            //glScalef(obj2->scale[0], obj2->scale[1], obj2->scale[2]);
            render_bush(); 
            glPopMatrix();
        }
        list2 = list2->next;
    }

    apply_anti_aliasing(AA_GEO);
    get_time_snapshot(PP_RENDER, DEBUG_SNAPSHOT_1_END);
    DEBUG_SNAPSHOT_1_RESET();
    
    render_end();
    gl_context_end();
    if (gZTargetTimer) {
        rdpq_set_mode_fill(RGBA32(0, 0, 0, 255));
        rdpq_mode_blender(0);
        rdpq_fill_rectangle(0, 0, display_get_width(), gZTargetOut);
        rdpq_fill_rectangle(0, display_get_height() - gZTargetOut, display_get_width(), display_get_height());
        rdpq_set_mode_standard();
    }
    render_hud();
    get_time_snapshot(PP_HUD, DEBUG_SNAPSHOT_1_END);
}

char sFirstBoot = 0;

void reset_display(void) {
    if (sFirstBoot) {
        gl_close();
        rdpq_close();
    }
    display_close();
    display_init(sVideoModes[gConfig.screenMode], DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    //gZBuffer = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());
    gZBuffer.flags = FMT_RGBA16 | SURFACE_FLAGS_OWNEDBUFFER;
    gZBuffer.width = display_get_width();
    gZBuffer.height = display_get_height();
    gZBuffer.stride = TEX_FORMAT_PIX2BYTES(FMT_RGBA16, display_get_width());
    gZBuffer.buffer = (void *) ((0x80800000 - 0x10000) - ((display_get_width() * display_get_height()) * 2));
    sFirstBoot = true;
    gl_init();
    init_renderer();
}

void set_region_type(int region) {
    if (region == PAL60) {
        region = NTSC60;
    }
    *(uint32_t*) 0x80000300 = region; // Writes to osTvType
    reset_display();
}

void init_video(void) {
    set_region_type(gConfig.regionMode);
}

void init_memory(void) {
    dfs_init(DFS_DEFAULT_LOCATION);
    timer_init();
    gCurrentFont = rdpq_font_load(gFontAssetTable[0]);
}

void init_save(void) {
    gConfig.antiAliasing = AA_OFF;
    gConfig.dedither = false;
    gConfig.regionMode = get_tv_type();
    //gConfig.regionMode = PAL60;
    gConfig.screenMode = SCREEN_4_3;
    gConfig.soundMode = SOUND_STEREO;
}

void init_game(void) {
    init_memory();
    init_controller();
    init_save();
    init_video();
    init_audio();
    init_debug();
    gGlobalTimer = 0;
    gGameTimer = 0;
}

void update_game_time(int *updateRate, float *updateRateF) {
    static unsigned int prevTime = 0;
    static unsigned int deltaTime = 0;
    static unsigned int curTime;
    
    curTime = timer_ticks();
    // Convert it to float too, and make it 20% faster on PAL systems.
    *updateRateF = ((float) (curTime - prevTime) / 1000000.0f);
    if (gConfig.regionMode == PAL50) {
        *updateRateF *= 1.2f;
    }
    if (*updateRateF <= 0.0001f) {
        *updateRateF = 0.0001f;
    }
    deltaTime += TIMER_MICROS(curTime - prevTime);
    prevTime = curTime;
    deltaTime -= 16666;
    *updateRate = LOGIC_60FPS;
    while (deltaTime > 16666) {
        deltaTime -= 16666;
        updateRate++;
        if (*updateRate == LOGIC_15FPS) {
            deltaTime = 0;
        }
    }
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

        update_game_time(&updateRate, &updateRateF);
        update_inputs(updateRate);
        update_game_entities(updateRate, updateRateF);
        camera_loop(updateRate, updateRateF);
        audio_loop(updateRate, updateRateF);
        
        render_game();
        get_cpu_time(DEBUG_SNAPSHOT_1_END);
        
        process_profiler();
        if (gDebugData && gDebugData->enabled) {
            render_profiler();
        }

        if (get_input_pressed(INPUT_R, 0)) {
            gConfig.antiAliasing++;
            if (gConfig.antiAliasing == 2) {
                gConfig.antiAliasing = -1;
            }
        }

        cycle_textures(updateRate);
        rdpq_detach_wait();
        display_show(gFrameBuffers);

        gGlobalTimer++;
        gGameTimer += updateRate;
    }

}
