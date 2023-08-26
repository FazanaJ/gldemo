#include <libdragon.h>
#include <malloc.h>

#include "../../include/global.h"

#include "../main.h"
#include "../assets.h"
#include "../render.h"
#include "../input.h"
#include "../debug.h"
#include "../hud.h"
#include "../save.h"
#include "../audio.h"

/*
static const resolution_t RESOLUTION_304x224 = {320, 240, false};
static const resolution_t RESOLUTION_384x224 = {384, 240, false};
static const resolution_t RESOLUTION_408x224 = {424, 240, false};

static const resolution_t sVideoModes[] = {
    RESOLUTION_304x224,
    RESOLUTION_384x224,
    RESOLUTION_408x224
};

void init_debug(void) {
    debug_init_isviewer();
    debug_init_usblog();
    rspq_profile_start();
    //console_init();
    //console_set_render_mode(RENDER_MANUAL);
    //rdpq_debug_start();
    //rdpq_debug_log(true);
    gDebugData = malloc(sizeof(DebugData));
    bzero(gDebugData, sizeof(DebugData));
    gDebugData->enabled = false;
}

void set_region_type(int region) {
    if (region == PAL60) {
        region = NTSC60;
    }
    *(uint32_t*) 0x80000300 = region; // Writes to osTvType
    display_init(sVideoModes[(unsigned) gConfig.screenMode], DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    //gZBuffer = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());
    gZBuffer.flags = FMT_RGBA16 | SURFACE_FLAGS_OWNEDBUFFER;
    gZBuffer.width = display_get_width();
    gZBuffer.height = display_get_height();
    gZBuffer.stride = TEX_FORMAT_PIX2BYTES(FMT_RGBA16, display_get_width());
    gZBuffer.buffer = (void *) ((0x80800000 - 0x10000) - ((display_get_width() * display_get_height()) * 2));
    gl_init();
    init_renderer();
}

void memory_error_screen(void) {
    if (get_memory_size() == 0x400000) {
        display_init(RESOLUTION_320x240, DEPTH_16_BPP, 1, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
        sprite_t *background = sprite_load(asset_dir("memory_error.rgba16", DFS_SPRITE));
        graphics_draw_sprite(display_get(), 0, 0, background);
        while (1);
    }
}

void init_memory(void) {
    timer_init();
    memory_error_screen();
}

void init_controller(void) {
    controller_init();
    bzero(&gInputData, sizeof(Input));
    gInputData.pak = 0;
}
*/

void init_game(void) {
    /*init_memory();
    init_controller();
    init_hud();
    init_audio();
    init_debug();
    init_save_data();
    set_region_type(gConfig.regionMode);
    load_font(FONT_ARIAL);
    load_font(FONT_MVBOLI);
    gGlobalTimer = 0;
    gGameTimer = 0;*/
    //debugf("init_game has run :)\n");
}