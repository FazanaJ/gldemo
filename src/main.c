#include <libdragon.h>
#include <malloc.h>

#include "main.h"
#include "../include/global.h"

#include "camera.h"
#include "render.h"
#include "assets.h"
#include "input.h"
#include "math_util.h"
#include "debug.h"
#include "player.h"
#include "object.h"
#include "hud.h"
#include "audio.h"
#include "menu.h"

surface_t gZBuffer;
surface_t *gFrameBuffers;
rdpq_font_t *gCurrentFont;
unsigned int gGlobalTimer;
unsigned int gGameTimer;
char gResetDisplay = false;

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

Config gConfig;

void setup(void) {

    init_renderer();

    gPlayer = spawn_object_pos(OBJ_PLAYER, 0.0f, 0.0f, 0.0f);
    spawn_clutter(CLUTTER_BUSH, 20, 40, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, 10, 45, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, 35, -100, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, 75, -25, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, 60, 40, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, -25, 45, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, -95, 40, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, 45, 0, 0, 0, 0, 0);
    spawn_clutter(CLUTTER_BUSH, 75, -75, 0, 0, 0, 0);
    spawn_object_pos(OBJ_NPC, 50.0f, 0.0f, 0.0f);

    camera_init();
}

char sFirstBoot = 0;

void reset_display(void) {
    if (sFirstBoot) {
        gl_close();
        rdpq_close();
    }
    display_close();
    display_init(sVideoModes[(unsigned) gConfig.screenMode], DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
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
    gConfig.antiAliasing = AA_FAST;
    gConfig.dedither = false;
    gConfig.regionMode = get_tv_type();
    if (gConfig.regionMode == PAL50) {
        gIsPal = true;
    }
    //gConfig.regionMode = PAL60;
    gConfig.screenMode = SCREEN_4_3;
    gConfig.frameCap = 0;
    gConfig.soundMode = SOUND_STEREO;
    gConfig.musicVolume = 9;
    gConfig.soundVolume = 9;
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
        *updateRate = *updateRate + 1;
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
        cycle_textures(updateRate);
        update_inputs(updateRate);
        update_game_entities(updateRate, updateRateF);
        audio_loop(updateRate, updateRateF);
        process_menus(updateRate, updateRateF);
        
        render_game(updateRate, updateRateF);
        get_cpu_time(DEBUG_SNAPSHOT_1_END);
        
        DEBUG_SNAPSHOT_1_RESET();
        process_profiler();
        if (gDebugData && gDebugData->enabled) {
            render_profiler();
        }
        get_time_snapshot(PP_PROFILER, DEBUG_SNAPSHOT_1_END);

        rdpq_detach_wait();
        display_show(gFrameBuffers);

        if (gResetDisplay) {
            reset_display();
            gResetDisplay = false;
        }

        gGlobalTimer++;
        gGameTimer += updateRate;
    }

}
