#include <libdragon.h>

#include "main.h"
#include "../include/global.h"

#include "camera.h"
#include "render.h"
#include "assets.h"
#include "input.h"
#include "math_util.h"
#include "debug.h"
#include "object.h"
#include "hud.h"
#include "audio.h"
#include "menu.h"
#include "scene.h"
#include "save.h"
#include "talk.h"
#include "screenshot.h"

surface_t gZBuffer;
surface_t *gFrameBuffers;
unsigned int gGlobalTimer;
unsigned int gGameTimer;
char gResetDisplay = false;

static const resolution_t RESOLUTION_304x224 = {SCREEN_WIDTH, SCREEN_HEIGHT, false};
static const resolution_t RESOLUTION_384x224 = {SCREEN_WIDTH_16_10, SCREEN_HEIGHT, false};
static const resolution_t RESOLUTION_408x224 = {SCREEN_WIDTH_16_9, SCREEN_HEIGHT, false};

static const resolution_t sVideoModes[] = {
    RESOLUTION_304x224,
    RESOLUTION_384x224,
    RESOLUTION_408x224
};

Config gConfig;

void reset_display(void) {
    gl_close();
    rdpq_close();
    display_close();
    display_init(sVideoModes[(int) gConfig.screenMode], DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    //gZBuffer = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());
    gZBuffer.flags = FMT_RGBA16 | SURFACE_FLAGS_OWNEDBUFFER;
    gZBuffer.width = display_get_width();
    gZBuffer.height = display_get_height();
    gZBuffer.stride = TEX_FORMAT_PIX2BYTES(FMT_RGBA16, display_get_width());
    gZBuffer.buffer = (void *) ((0x80800000 - 0x10000) - ((display_get_width() * display_get_height()) * 2));
    gl_init();
    init_renderer();
}

/**
 * Generate the delta time values.
 * Use a while loop with a frame debt system to generate accurate integer delta, with just a regular comparison for floats.
 * Multiply the float value by 1.2 for PAL users. For integer, use timer_int(int timer) to set a region corrected timer instead.
*/
static inline void update_game_time(int *updateRate, float *updateRateF) {
    static unsigned int prevTime = 0;
    static unsigned int deltaTime = 0;
    static unsigned int curTime;
    
    curTime = timer_ticks();
    // Convert it to float too, and make it 20% faster on PAL systems.
    *updateRateF = ((float) TIMER_MICROS(curTime - prevTime) / 16666.666f);
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

void boot_game(void) {
    dfs_init(DFS_DEFAULT_LOCATION);
    void *bootOvl = dlopen(asset_dir("boot", DFS_OVERLAY), RTLD_LOCAL);
    void (*func)() = dlsym(bootOvl, "init_game");
    (*func)();
    dlclose(bootOvl);
    debugf("Game booted in %2.3fs.\n", ((float) TIMER_MICROS(timer_ticks())) / 1000000.0f);
}

int main(void) {
    int updateRate;
    float updateRateF;

    boot_game();
    load_scene(SCENE_INTRO);

    while (1) {
        reset_profiler_times();
        DEBUG_SNAPSHOT_1();

        update_game_time(&updateRate, &updateRateF);
        asset_cycle(updateRate);
        update_inputs(updateRate);
        if (gTalkControl == NULL) {
            update_game_entities(updateRate, updateRateF);
        }
        process_hud(updateRate, updateRateF);
        process_menus(updateRate, updateRateF);
#ifdef PUPPYPRINT_DEBUG
        first1 = DEBUG_SNAPSHOT_1_END;
#endif
        if (gScreenshotStatus <= SCREENSHOT_NONE) {
            gFrameBuffers = display_get();
        }
        DEBUG_SNAPSHOT_2();
        audio_loop(updateRate, updateRateF);
        render_game(updateRate, updateRateF);
        get_cpu_time((DEBUG_SNAPSHOT_2_END) + first1);
        DEBUG_SNAPSHOT_1_RESET();
        process_profiler();
        if (gDebugData && gDebugData->enabled && gScreenshotStatus != SCREENSHOT_GENERATE) {
            render_profiler();
        }
        get_time_snapshot(PP_PROFILER, DEBUG_SNAPSHOT_1_END);

        if (gScreenshotStatus > SCREENSHOT_NONE) {
            rdpq_detach_wait();
            gScreenshotStatus = SCREENSHOT_SHOW;
        } else {
            rdpq_detach_show();
        }

        if (gResetDisplay) {
            reset_display();
            gResetDisplay = false;
        }

        gGlobalTimer++;
        gGameTimer += updateRate;
    }
}
