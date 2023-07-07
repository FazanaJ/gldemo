#include <libdragon.h>
#include <malloc.h>

#include "debug.h"
#include "../include/global.h"

#include "main.h"
#include "object.h"
#include "input.h"

#ifdef PUPPYPRINT_DEBUG

float gFPS = 0.0f;
DebugData *gDebugData = NULL;

void init_debug(void) {
    debug_init_isviewer();
    debug_init_usblog();
    //console_init();
    //console_set_render_mode(RENDER_MANUAL);
    //rdpq_debug_start();
    //rdpq_debug_log(true);
    gDebugData = malloc(sizeof(DebugData));
    bzero(gDebugData, sizeof(DebugData));
    gDebugData->enabled = true;
}

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
}

void get_time_snapshot(int index, int diff) {
    int time = TIMER_MICROS(diff);
    if (time > 99999) {
        time = 99999;
    }
    gDebugData->timer[index][gDebugData->iteration] += time;
    gDebugData->timer[index][TIME_AGGREGATE] += time;
}

void get_cpu_time(int diff) {
    int time = TIMER_MICROS(diff);
    if (time > 99999) {
        time = 99999;
    }
    gDebugData->cpuTime[gDebugData->iteration] += time;
    gDebugData->cpuTime[TIME_AGGREGATE] += time;
}
extern Input sInputData;;
void get_rsp_time(int diff) {
    int time = TIMER_MICROS(diff);
    if (time > 99999) {
        time = 99999;
    }
    gDebugData->rspTime[gDebugData->iteration] += time;
    gDebugData->rspTime[TIME_AGGREGATE] += time;
}

void get_rdp_time(int diff) {
    int time = TIMER_MICROS(diff);
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
    unsigned int  oldTime = frameTimes[curFrameTimeIndex];
    frameTimes[curFrameTimeIndex] = newTime;

    curFrameTimeIndex++;
    if (curFrameTimeIndex >= 30) {
        curFrameTimeIndex = 0;
    }
    gFPS = (30.0f * 1000000.0f) / TIMER_MICROS(newTime - oldTime);
}

void process_profiler(void) {
    calculate_framerate();

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

    if (get_input_pressed(INPUT_L, 0) && get_input_held(INPUT_DUP)) {
        gDebugData->enabled ^= 1;
    }
}

void render_profiler(void) {
    int boxHeight = 0;
    int divisor = (get_tv_type() == TV_PAL) ? 400 : 333;
    struct mallinfo mem_info = mallinfo();
    rdpq_set_prim_color(RGBA32(0, 0, 0, 96));
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    if (gDebugData->rspTime[TIME_TOTAL] > 10) {
        boxHeight += 10;
    }
    if (gDebugData->rdpTime[TIME_TOTAL] > 10) {
        boxHeight += 10;
    }
    if (gDebugData->timer[PP_INPUT][TIME_TOTAL] > 10) {
        boxHeight -= 10;
    }
    for (int i = 0; i < PP_TOTAL; i++) {
        if (gDebugData->timer[i][TIME_TOTAL] > 10) {
            boxHeight += 10;
        }
    }
    rdpq_fill_rectangle(8, 6, 118, 28 + boxHeight);
    rdpq_fill_rectangle(8, gFrameBuffers->height - 20, 132, gFrameBuffers->height - 6);
    rdpq_mode_blender(0);
    rdpq_font_begin(RGBA32(255, 255, 255, 255));
    rdpq_font_position(8, 16);
    rdpq_font_printf(gCurrentFont, "FPS: %2.2f", gFPS);
    rdpq_font_position(8, 26);
    rdpq_font_printf(gCurrentFont, "CPU: %dus (%d%%)", gDebugData->cpuTime[TIME_TOTAL], gDebugData->cpuTime[TIME_TOTAL] / divisor);
    boxHeight = 0;
    if (gDebugData->rspTime[TIME_TOTAL] > 10) {
        boxHeight += 10;
        rdpq_font_position(8, 26 + boxHeight);
        rdpq_font_printf(gCurrentFont, "RSP: %dus (%d%%)", gDebugData->rspTime[TIME_TOTAL], gDebugData->rspTime[TIME_TOTAL] / divisor);
    }
    if (gDebugData->rdpTime[TIME_TOTAL] > 10) {
        boxHeight += 10;
        rdpq_font_position(8, 26 + boxHeight);
        rdpq_font_printf(gCurrentFont, "RDP: %dus (%d%%)", gDebugData->rdpTime[TIME_TOTAL], gDebugData->rdpTime[TIME_TOTAL] / divisor);
    }
    if (gDebugData->timer[PP_OBJECTS][TIME_TOTAL] > 10) {
        boxHeight += 10;
        rdpq_font_position(8, 26 + boxHeight);
        rdpq_font_printf(gCurrentFont, "OBJ: %dus (%d%%): %d", gDebugData->timer[PP_OBJECTS][TIME_TOTAL], gDebugData->timer[PP_OBJECTS][TIME_TOTAL] / divisor, gNumObjects);
    }
    if (gDebugData->timer[PP_RENDER][TIME_TOTAL] > 10) {
        boxHeight += 10;
        rdpq_font_position(8, 26 + boxHeight);
        rdpq_font_printf(gCurrentFont, "REN: %dus (%d%%)", gDebugData->timer[PP_RENDER][TIME_TOTAL], gDebugData->timer[PP_RENDER][TIME_TOTAL] / divisor);
    }
    if (gDebugData->timer[PP_AUDIO][TIME_TOTAL] > 10) {
        boxHeight += 10;
        rdpq_font_position(8, 26 + boxHeight);
        rdpq_font_printf(gCurrentFont, "AUD: %dus (%d%%)", gDebugData->timer[PP_AUDIO][TIME_TOTAL], gDebugData->timer[PP_AUDIO][TIME_TOTAL] / divisor);
    }
    if (gDebugData->timer[PP_HUD][TIME_TOTAL] > 10) {
        boxHeight += 10;
        rdpq_font_position(8, 26 + boxHeight);
        rdpq_font_printf(gCurrentFont, "HUD: %dus (%d%%)", gDebugData->timer[PP_HUD][TIME_TOTAL], gDebugData->timer[PP_HUD][TIME_TOTAL] / divisor);
    }
    rdpq_font_position(8, gFrameBuffers->height - 8);
    rdpq_font_printf(gCurrentFont, "RAM: %dKB/%dKB", (mem_info.uordblks / 1024), get_memory_size() / 1024);
    rdpq_font_end();
}

#endif