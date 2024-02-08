#pragma once

#include "../include/global.h"
#include "rspq_profile.h"
#include "object.h"

#define TIME_ITERATIONS 60
#define TIME_AGGREGATE TIME_ITERATIONS
#define TIME_TOTAL TIME_ITERATIONS + 1

#define PROFILE_NAMES \
    "Player", \
    "BhvObjects", \
    "Render", \
    "Input", \
    "DMA", \
    "Audio", \
    "HUD", \
    "BhvClutter", \
    "Camera", \
    "Materials", \
    "Profiler", \
    "BhvParticles", \
    "Collision", \
    "Batch", \
    "RenObjects", \
    "RenClutter", \
    "RenParticles", \
    "RenLevel", \
    "Shadows", \
    "RenderList", \
    "Matrix", \
    "Menu", \
    "", \
    "Culling", \
    "Background", \
    "Hitboxes"


enum ProfileTimers {
    PP_PLAYER,
    PP_OBJECTS,
    PP_RENDER,
    PP_INPUT,
    PP_DMA,
    PP_AUDIO,
    PP_HUD,
    PP_CLUTTER,
    PP_CAMERA,
    PP_MATERIALS,
    PP_PROFILER,
    PP_PARTICLES,
    PP_COLLISION,
    PP_BATCH,
    PP_RENDEROBJECTS,
    PP_RENDERCLUTTER,
    PP_RENDERPARTICLES,
    PP_RENDERLEVEL,
    PP_SHADOWS,
    PP_RENDERLIST,
    PP_MATRIX,
    PP_MENU,
    PP_HALT,
    PP_CULLING,
    PP_BG,
    PP_HITBOXES,

    PP_TOTAL
};

enum PPPages {
    PAGE_MINIMAL,
    PAGE_OVERVIEW,
    PAGE_BREAKDOWN,
    PAGE_OBJECTS,
    PAGE_MEMORY,
    PAGE_AUDIO,

    PAGE_TOTAL
};

#define PP_PAGES \
    "Minimal", \
    "Overview", \
    "Breakdown", \
    "Objects", \
    "Memory", \
    "Audio"

typedef struct DebugData {
    rspq_profile_data_t rspData;
    unsigned int timer[PP_TOTAL][TIME_TOTAL + 1];
    unsigned short objTimer[OBJ_TOTAL][TIME_TOTAL + 1];
    unsigned char objCount[OBJ_TOTAL];
    ObjectEntry *objHeader[OBJ_TOTAL];
    unsigned int cpuTime[TIME_TOTAL + 1];
    unsigned int rspTime[TIME_TOTAL + 1];
    unsigned int rdpTime[TIME_TOTAL + 1];
    unsigned iteration : 8;
    unsigned enabled : 1;
    unsigned viewHitbox : 1;
    unsigned menuOpen : 1;
    signed char menuScroll;
    signed char menuPage;
    signed char menuOption;
    unsigned short matrixOps;
    model64_t *debugMeshes[2];
} DebugData;

extern DebugData *gDebugData;

#ifdef PUPPYPRINT_DEBUG
void reset_profiler_times(void);
void get_time_snapshot(int index, int diff);
void add_time_offset(int index, int diff);
int get_profiler_time(int index);
void get_cpu_time(int diff);
void get_rsp_time(int diff);
void get_rdp_time(int diff);
void init_debug(void);
void render_profiler(void);
void process_profiler(void);
void get_obj_snapshot(Object *obj, int diff, int count);
void profiler_wait(void);
#define DEBUG_SNAPSHOT_1() unsigned int first1 = timer_ticks()
#define DEBUG_SNAPSHOT_2() unsigned int first2 = timer_ticks()
#define DEBUG_SNAPSHOT_3() unsigned int first3 = timer_ticks()
#define DEBUG_SNAPSHOT_1_RESET() first1 = timer_ticks()
#define DEBUG_SNAPSHOT_2_RESET() first2 = timer_ticks()
#define DEBUG_SNAPSHOT_3_RESET() first3 = timer_ticks()
#define DEBUG_SNAPSHOT_1_END timer_ticks() - first1
#define DEBUG_SNAPSHOT_2_END timer_ticks() - first2
#define DEBUG_SNAPSHOT_3_END timer_ticks() - first3
#define DEBUG_GET_TIME_1(index) unsigned int compare1 = get_profiler_time(index)
#define DEBUG_GET_TIME_2(index) unsigned int compare2 = get_profiler_time(index)
#define DEBUG_GET_TIME_3(index) unsigned int compare3 = get_profiler_time(index)
#define DEBUG_GET_TIME_1_RESET(index) compare1 = get_profiler_time(index)
#define DEBUG_GET_TIME_2_RESET(index) compare2 = get_profiler_time(index)
#define DEBUG_GET_TIME_3_RESET(index) compare3 = get_profiler_time(index)
#define DEBUG_GET_TIME_1_END(index) get_profiler_time(index) - compare1
#define DEBUG_GET_TIME_2_END(index) get_profiler_time(index) - compare2
#define DEBUG_GET_TIME_3_END(index) get_profiler_time(index) - compare3
#define DEBUG_MATRIX_OP() gDebugData->matrixOps++
#else
#define reset_profiler_times()
#define get_time_snapshot(index, diff)
#define add_time_offset(index, diff)
#define get_obj_snapshot(obj, diff, count)
#define get_cpu_time(diff)
#define get_rsp_time(diff)
#define get_rdp_time(diff)
#define init_debug()
#define render_profiler()
#define process_profiler()
#define profiler_wait()
#define DEBUG_SNAPSHOT_1()
#define DEBUG_SNAPSHOT_2()
#define DEBUG_SNAPSHOT_3()
#define DEBUG_SNAPSHOT_1_RESET()
#define DEBUG_SNAPSHOT_2_RESET()
#define DEBUG_SNAPSHOT_3_RESET()
#define DEBUG_SNAPSHOT_1_END
#define DEBUG_SNAPSHOT_2_END
#define DEBUG_SNAPSHOT_3_END
#define DEBUG_GET_TIME_1(index)
#define DEBUG_GET_TIME_2(index)
#define DEBUG_GET_TIME_3(index)
#define DEBUG_GET_TIME_1_RESET(index)
#define DEBUG_GET_TIME_2_RESET(index)
#define DEBUG_GET_TIME_3_RESET(index)
#define DEBUG_GET_TIME_1_END(index)
#define DEBUG_GET_TIME_2_END(index)
#define DEBUG_GET_TIME_3_END(index)
#define DEBUG_MATRIX_OP()
#endif