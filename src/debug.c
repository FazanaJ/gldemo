#include <libdragon.h>
#include <malloc.h>

#include "debug.h"
#include "../include/global.h"

#include "main.h"
#include "object.h"
#include "input.h"
#include "assets.h"
#include "rspq_profile.h"

#ifdef PUPPYPRINT_DEBUG

float gFPS = 0.0f;
DebugData *gDebugData = NULL;
char *sDebugText[] = {
    PROFILE_NAMES
};

void reset_profiler_times(void) {
    for (int i = 0; i < PP_TOTAL; i++) {
        gDebugData->timer[i][TIME_AGGREGATE] -= gDebugData->timer[i][gDebugData->iteration];
        gDebugData->timer[i][gDebugData->iteration] = 0;
    }
    
    gDebugData->cpuTime[TIME_AGGREGATE] -= gDebugData->cpuTime[gDebugData->iteration];
    gDebugData->cpuTime[gDebugData->iteration] = 0;
    gDebugData->rspTime[TIME_AGGREGATE] -= gDebugData->rspTime[gDebugData->iteration];
    gDebugData->rspTime[gDebugData->iteration] = 0;
    gDebugData->rdpTime[TIME_AGGREGATE] -= gDebugData->rdpTime[gDebugData->iteration];
    gDebugData->rdpTime[gDebugData->iteration] = 0;
    
    gDebugData->matrixOps = 0;
}

void get_time_snapshot(int index, int diff) {
    int time = TIMER_MICROS(diff);
    if (time > 99999) {
        time = 99999;
    }
    gDebugData->timer[index][gDebugData->iteration] += time;
    gDebugData->timer[index][TIME_AGGREGATE] += time;
}

void add_time_offset(int index, int diff) {
    gDebugData->timer[index][gDebugData->iteration] -= diff;
    gDebugData->timer[index][TIME_AGGREGATE] -= diff;
}

int get_profiler_time(int index) {
    return gDebugData->timer[index][gDebugData->iteration];
}

void get_cpu_time(int diff) {
    int time = TIMER_MICROS(diff);
    if (time > 99999) {
        time = 99999;
    }
    gDebugData->cpuTime[gDebugData->iteration] += time;
    gDebugData->cpuTime[TIME_AGGREGATE] += time;
}

void get_rsp_time(int diff) {
    int time = (diff * 10) / 625;
    if (time > 99999) {
        time = 99999;
    }
    gDebugData->rspTime[gDebugData->iteration] += time;
    gDebugData->rspTime[TIME_AGGREGATE] += time;
}

void get_rdp_time(int diff) {
    int time = (diff * 10) / 625;
    if (time > 99999) {
        time = 99999;
    }
    gDebugData->rdpTime[gDebugData->iteration] += time;
    gDebugData->rdpTime[TIME_AGGREGATE] += time;
}

static void calculate_framerate(void) {
    static unsigned int curFrameTimeIndex = 0;
    static unsigned int frameTimes[30];
    unsigned int newTime = timer_ticks();
    unsigned int oldTime = frameTimes[curFrameTimeIndex];
    frameTimes[curFrameTimeIndex] = newTime;

    curFrameTimeIndex++;
    if (curFrameTimeIndex >= 30) {
        curFrameTimeIndex = 0;
    }
    gFPS = (30.0f * 1000000.0f) / TIMER_MICROS(newTime - oldTime);
}

void process_profiler(void) {
    calculate_framerate();

    if ((gGlobalTimer % 8) == 0) {
        rspq_profile_get_data(&gDebugData->rspData);
        rspq_profile_reset();
    }

    get_rsp_time(gDebugData->rspData.total_ticks / 8);
    rspq_profile_next_frame();

    for (int i = 0; i < PP_TOTAL; i++) {
        gDebugData->timer[i][TIME_TOTAL] = gDebugData->timer[i][TIME_AGGREGATE] / TIME_TOTAL;
    }

    gDebugData->cpuTime[TIME_TOTAL] = gDebugData->cpuTime[TIME_AGGREGATE] / TIME_TOTAL;
    gDebugData->rspTime[TIME_TOTAL] = gDebugData->rspTime[TIME_AGGREGATE] / TIME_TOTAL;
    gDebugData->rdpTime[TIME_TOTAL] = gDebugData->rdpTime[TIME_AGGREGATE] / TIME_TOTAL;

    gDebugData->iteration++;
    if (gDebugData->iteration >= TIME_ITERATIONS) {
        gDebugData->iteration = 0;
    }

    if (get_input_pressed(INPUT_L, 5) && get_input_held(INPUT_DUP)) {
        gDebugData->enabled ^= 1;
        clear_input(INPUT_L);
    }

    if ((gGlobalTimer % 120) == 0) {
        debugf("FPS: %2.2f | CPU: %dus (%d%%) | RSP: %dus (%d%%) | RDP: %dus (%d%%)\n", 
        gFPS, gDebugData->cpuTime[TIME_TOTAL], gDebugData->cpuTime[TIME_TOTAL] / 333,
        gDebugData->rspTime[TIME_TOTAL], gDebugData->rspTime[TIME_TOTAL] / 333, gDebugData->rdpTime[TIME_TOTAL], gDebugData->rdpTime[TIME_TOTAL] / 333);
    }
}

void render_profiler(void) {
    int boxHeight = 0;
    int width = display_get_width();
    int height = display_get_height();
    struct mallinfo mem_info = mallinfo();
    rdpq_set_prim_color(RGBA32(0, 0, 0, 127));
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    if (gDebugData->rspTime[TIME_TOTAL] > 10) {
        boxHeight += 8;
    }
    if (gDebugData->rdpTime[TIME_TOTAL] > 10) {
        boxHeight += 8;
    }
    rdpq_fill_rectangle(8, 6, 96, 28 + boxHeight);
    rdpq_fill_rectangle(8, gFrameBuffers->height - 42, 120, gFrameBuffers->height - 6);
    boxHeight = 10;
    for (int i = 0; i < PP_TOTAL; i++) {
        if (gDebugData->timer[i][TIME_TOTAL] > 1) {
            boxHeight += 8;
        }
    }
    rdpq_fill_rectangle(width - 104, 0, width, boxHeight);
    rdpq_mode_blender(0);
    rdpq_text_printf(NULL, FONT_ARIAL, 8, 16, "FPS: %2.2f", gFPS);
    rdpq_text_printf(NULL, FONT_ARIAL, 8, 26, "CPU: %dus (%d%%)", gDebugData->cpuTime[TIME_TOTAL], gDebugData->cpuTime[TIME_TOTAL] / 333);
    boxHeight = 0;
    if (gDebugData->rspTime[TIME_TOTAL] > 10) {
        boxHeight += 8;
        rdpq_text_printf(NULL, FONT_ARIAL, 8, 26 + boxHeight, "RSP: %dus (%d%%)", gDebugData->rspTime[TIME_TOTAL], gDebugData->rspTime[TIME_TOTAL] / 333);
    }
    if (gDebugData->rdpTime[TIME_TOTAL] > 10) {
        boxHeight += 8;
        rdpq_text_printf(NULL, FONT_ARIAL, 8, 26 + boxHeight, "RDP: %dus (%d%%)", gDebugData->rdpTime[TIME_TOTAL], gDebugData->rdpTime[TIME_TOTAL] / 333);
    }
    boxHeight = 0;
    for (int i = 0; i < PP_TOTAL; i++) {
        if (gDebugData->timer[i][TIME_TOTAL] > 1) {
            boxHeight += 8;
            rdpq_text_printf(NULL, FONT_ARIAL, width - 100, 8 + boxHeight, "%s:", sDebugText[i]);
            rdpq_text_printf(NULL, FONT_ARIAL, width - 38, 8 + boxHeight, "%dus", gDebugData->timer[i][TIME_TOTAL]);
        }
    }
    int ramUsed = mem_info.uordblks - (size_t) (((width * height) * 2) - ((unsigned int) HEAP_START_ADDR - 0x80000000) - 0x10000);
    rdpq_text_printf(NULL, FONT_ARIAL, 8, gFrameBuffers->height - 8, "RAM: %dKB/%dKB", (ramUsed / 1024), get_memory_size() / 1024);
    rdpq_text_printf(NULL, FONT_ARIAL, 8, gFrameBuffers->height - 16, "Tex: %d | Loads: %d", gNumTextures, gNumTextureLoads);
    rdpq_text_printf(NULL, FONT_ARIAL, 8, gFrameBuffers->height - 24, "Obj: %d | Clu: %d | Par: %d", gNumObjects, gNumClutter, gNumParticles);
    rdpq_text_printf(NULL, FONT_ARIAL, 8, gFrameBuffers->height - 32, "Mtx: %d | Mdl: %d | Ovl: %d", gDebugData->matrixOps, gNumModels, gNumOverlays);
}

#endif