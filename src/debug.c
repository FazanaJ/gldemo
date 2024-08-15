#include <libdragon.h>
#include <malloc.h>

#include "debug.h"
#include "../include/global.h"

#include "main.h"
#include "object.h"
#include "input.h"
#include "assets.h"
#include "rspq_profile.h"
#include "math_util.h"

#ifdef PUPPYPRINT_DEBUG

DebugData *gDebugData = NULL;
char *sPuppyPrintPageStrings[] = { PP_PAGES };
char *sDebugText[] = {
    PROFILE_NAMES
};

void reset_profiler_times(void) {
    for (int i = 0; i < PP_TOTAL; i++) {
        gDebugData->timer[i][TIME_AGGREGATE] -= gDebugData->timer[i][gDebugData->iteration];
        gDebugData->timer[i][gDebugData->iteration] = 0;
    }
    for (int i = 0; i < OBJ_TOTAL; i++) {
        gDebugData->objTimer[i][TIME_AGGREGATE] -= gDebugData->objTimer[i][gDebugData->iteration];
        gDebugData->objTimer[i][gDebugData->iteration] = 0;
        gDebugData->objCount[i] = 0;
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

void get_obj_snapshot(Object *obj, int diff, int count) {
    int time = TIMER_MICROS(diff);
    if (time > 99999) {
        time = 99999;
    }
    gDebugData->objTimer[obj->objectID][gDebugData->iteration] += time;
    gDebugData->objTimer[obj->objectID][TIME_AGGREGATE] += time;
    gDebugData->objCount[obj->objectID] += count;
    gDebugData->objHeader[obj->objectID] = obj->header;
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


#define RCP_TICKS_TO_USECS(ticks) (((ticks) * 1000000ULL) / RCP_FREQUENCY)

static bool debug_profile_is_idle(uint32_t index) {
  return index == RSPQ_PROFILE_CSLOT_WAIT_CPU || index == RSPQ_PROFILE_CSLOT_WAIT_RDP
    || index == RSPQ_PROFILE_CSLOT_WAIT_RDP_SYNCFULL || index == RSPQ_PROFILE_CSLOT_WAIT_RDP_SYNCFULL_MULTI;
}

typedef struct {
  uint32_t calls;
  int32_t timeUs;
  uint32_t index;
  color_t color;
  bool isIdle;
  const char *name;
} ProfileSlot;

void get_rsp_time(int diff) {
    const uint32_t SLOT_COUNT = RSPQ_PROFILE_SLOT_COUNT + 1;
    uint64_t totalTicks = 0;
    uint32_t timeTotalBusy = 0;
    uint32_t timeTotalWait = 0;
    ProfileSlot slots[SLOT_COUNT];
    for(size_t i = 0; i < RSPQ_PROFILE_SLOT_COUNT; i++)
    {
      ProfileSlot *slot = &slots[i];
      slot->index = i;
      slot->isIdle = debug_profile_is_idle(i);
      slot->name = gDebugData->rspData.slots[i].name;

      if(slot->name == NULL)continue;

      totalTicks += gDebugData->rspData.slots[i].total_ticks;

      slot->calls = (uint32_t)(gDebugData->rspData.slots[i].sample_count / gDebugData->rspData.frame_count);
      slot->timeUs = RCP_TICKS_TO_USECS(gDebugData->rspData.slots[i].total_ticks / gDebugData->rspData.frame_count);

      if(slot->isIdle) {
        timeTotalWait += slot->timeUs;
      } else {
        timeTotalBusy += slot->timeUs;
      }
    }

    // Calc. global times
    uint64_t dispatchTicks = gDebugData->rspData.total_ticks > totalTicks ? gDebugData->rspData.total_ticks - totalTicks : 0;
    uint64_t dispatchTime = RCP_TICKS_TO_USECS(dispatchTicks / gDebugData->rspData.frame_count);
    uint64_t rdpTimeBusy = RCP_TICKS_TO_USECS(gDebugData->rspData.rdp_busy_ticks / gDebugData->rspData.frame_count);

    // Add dispatch time into the slots
    slots[SLOT_COUNT - 1] = (ProfileSlot){
      .index = SLOT_COUNT - 1,
      .name = "Cmd process",
      .calls = 0,
      .timeUs = dispatchTime
    };
    timeTotalBusy += dispatchTime;

    gDebugData->rdpTime[gDebugData->iteration] += rdpTimeBusy;
    gDebugData->rdpTime[TIME_AGGREGATE] += rdpTimeBusy;
    gDebugData->rspTime[gDebugData->iteration] += timeTotalBusy;
    gDebugData->rspTime[TIME_AGGREGATE] += timeTotalBusy;
}

void profiler_wait(void) {
    if (gDebugData && gDebugData->enabled) {
        DEBUG_SNAPSHOT_3();
        rspq_wait();
        get_time_snapshot(PP_HALT, DEBUG_SNAPSHOT_3_END);
    }
}

extern unsigned char sResetTime;

void process_profiler(void) {

    if ((gGlobalTimer % 8) == 0 && sResetTime == false) {
        //rspq_wait();
        rspq_profile_get_data(&gDebugData->rspData);
        rspq_profile_reset();
    }
    
    if (gDebugData->rspData.frame_count != 0) {
        get_rsp_time(0);
    }
    rspq_profile_next_frame();

    for (int i = 0; i < PP_TOTAL; i++) {
        gDebugData->timer[i][TIME_TOTAL] = gDebugData->timer[i][TIME_AGGREGATE] / TIME_TOTAL;
    }

    for (int i = 0; i < OBJ_TOTAL; i++) {
        gDebugData->objTimer[i][TIME_TOTAL] = gDebugData->objTimer[i][TIME_AGGREGATE] / TIME_TOTAL;
    }

    gDebugData->cpuTime[TIME_TOTAL] = (gDebugData->cpuTime[TIME_AGGREGATE] / TIME_TOTAL) - gDebugData->timer[PP_HALT][TIME_TOTAL];
    gDebugData->rspTime[TIME_TOTAL] = gDebugData->rspTime[TIME_AGGREGATE] / TIME_TOTAL;
    gDebugData->rdpTime[TIME_TOTAL] = gDebugData->rdpTime[TIME_AGGREGATE] / TIME_TOTAL;

    gDebugData->iteration++;
    if (gDebugData->iteration >= TIME_ITERATIONS) {
        gDebugData->iteration = 0;
    }

    if (input_pressed(INPUT_L, 5) && input_held(INPUT_DUP)) {
        gDebugData->enabled ^= 1;
        input_clear(INPUT_L);
        input_clear(INPUT_DUP);
    } else if (gDebugData->enabled) {
        if (input_pressed(INPUT_L, 5)) {
            gDebugData->menuOpen ^= 1;
            if (gDebugData->menuOpen == false) {
                gDebugData->menuPage = gDebugData->menuOption;
            }
            input_clear(INPUT_L);
        }
        if (input_pressed(INPUT_DUP, 5)) {
            input_clear(INPUT_DUP);
            gDebugData->menuOption--;
            if (gDebugData->menuOption <= -1) {
                gDebugData->menuOption = PAGE_TOTAL - 1;
                gDebugData->menuScroll = PAGE_TOTAL - 5;
            }
        } else if (input_pressed(INPUT_DDOWN, 5)) {
            input_clear(INPUT_DDOWN);
            gDebugData->menuOption++;
            if (gDebugData->menuOption >= PAGE_TOTAL) {
                gDebugData->menuOption = 0;
                gDebugData->menuScroll = 0;
            }
        }
        if (gDebugData->menuScroll + 4 < gDebugData->menuOption) {
            gDebugData->menuScroll++;
        } else if (gDebugData->menuScroll > gDebugData->menuOption) {
            gDebugData->menuScroll--;
        }
    }

    if ((gGlobalTimer % 120) == 0) {
        debugf("FPS: %2.2f | CPU: %dus (%d%%) | RSP: %dus (%d%%) | RDP: %dus (%d%%)\n", 
        (double ) display_get_fps(), gDebugData->cpuTime[TIME_TOTAL], gDebugData->cpuTime[TIME_TOTAL] / 333,
        gDebugData->rspTime[TIME_TOTAL], gDebugData->rspTime[TIME_TOTAL] / 333, gDebugData->rdpTime[TIME_TOTAL], gDebugData->rdpTime[TIME_TOTAL] / 333);
    }
}

void profiler_draw_minimal(void) {
    int boxHeight = 0;
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
    rdpq_mode_blender(0);
    rdpq_text_printf(NULL, FONT_ARIAL, 8, 16, "FPS %2.2f", (double ) display_get_fps());
    rdpq_text_printf(NULL, FONT_ARIAL, 8, 26, "CPU %dus (%d%%)", gDebugData->cpuTime[TIME_TOTAL], gDebugData->cpuTime[TIME_TOTAL] / 333);
    boxHeight = 0;
    if (gDebugData->rspTime[TIME_TOTAL] > 10) {
        boxHeight += 8;
        rdpq_text_printf(NULL, FONT_ARIAL, 8, 26 + boxHeight, "RSP %dus (%d%%)", gDebugData->rspTime[TIME_TOTAL], gDebugData->rspTime[TIME_TOTAL] / 333);
    }
    if (gDebugData->rdpTime[TIME_TOTAL] > 10) {
        boxHeight += 8;
        rdpq_text_printf(NULL, FONT_ARIAL, 8, 26 + boxHeight, "RDP %dus (%d%%)", gDebugData->rdpTime[TIME_TOTAL], gDebugData->rdpTime[TIME_TOTAL] / 333);
    }
}

void profiler_draw_overview(void) {
    profiler_draw_minimal();
    struct mallinfo mem_info = mallinfo();
    int width = display_get_width();
    int height = display_get_height();

    rdpq_set_prim_color(RGBA32(0, 0, 0, 127));
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_fill_rectangle(8, height - 58, 120, height - 6);

    int ramUsed = mem_info.uordblks - (size_t) (((width * height) * 2) - ((unsigned int) HEAP_START_ADDR - 0x80000000) - 0x10000);
    rdpq_text_printf(NULL, FONT_ARIAL, 8, height - 8, "RAM %dKB/%dKB", (ramUsed / 1024), get_memory_size() / 1024);
    rdpq_text_printf(NULL, FONT_ARIAL, 8, height - 16, "Tex %d | Loads %d", gNumMaterials, gNumTextureLoads);
    rdpq_text_printf(NULL, FONT_ARIAL, 8, height - 24, "Obj %d | Clu %d | Par %d", gNumObjects, gNumClutter, gNumParticles);
    rdpq_text_printf(NULL, FONT_ARIAL, 8, height - 32, "Mtx %d | Mdl %d | Ovl %d", gDebugData->matrixOps, gNumModels, gNumOverlays);
    rdpq_text_printf(NULL, FONT_ARIAL, 8, height - 40, "OPA 0x%X", gSortRecord[DRAW_OPA]);
    rdpq_text_printf(NULL, FONT_ARIAL, 8, height - 48, "XLU 0x%X", gSortRecord[DRAW_XLU]);

    if (gPlayer) {
        rdpq_text_printf(NULL, FONT_ARIAL, 8, 128 + 0, "X: %2.2f", (double) gPlayer->pos[0]);
        rdpq_text_printf(NULL, FONT_ARIAL, 8, 128 + 8, "Y: %2.2f", (double) gPlayer->pos[1]);
        rdpq_text_printf(NULL, FONT_ARIAL, 8, 128 + 16, "Z: %2.2f", (double) gPlayer->pos[2]);
    }
}

void profiler_draw_breakdown(void) {
    profiler_draw_minimal();
    int boxHeight = 0;
    int width = display_get_width();


    rdpq_set_prim_color(RGBA32(0, 0, 0, 127));
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    boxHeight = 10;
    for (int i = 0; i < PP_TOTAL; i++) {
        if (i != PP_HALT && gDebugData->timer[i][TIME_TOTAL] > 1) {
            boxHeight += 8;
        }
    }
    rdpq_fill_rectangle(width - 104, 0, width, boxHeight);
    rdpq_mode_blender(0);
    boxHeight = 0;
    for (int i = 0; i < PP_TOTAL; i++) {
        if (i != PP_HALT && gDebugData->timer[i][TIME_TOTAL] > 1) {
            boxHeight += 8;
            rdpq_text_printf(NULL, FONT_ARIAL, width - 100, 8 + boxHeight, "%s", sDebugText[i]);
            rdpq_text_printf(NULL, FONT_ARIAL, width - 38, 8 + boxHeight, "%d", gDebugData->timer[i][TIME_TOTAL]);
        }
    }
}

void profiler_draw_objects(void) {
    profiler_draw_minimal();
    int boxHeight = 0;
    int width = display_get_width();

    rdpq_set_prim_color(RGBA32(0, 0, 0, 127));
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    boxHeight = 10;
    for (int i = 0; i < OBJ_TOTAL; i++) {
        if (i != PP_HALT && gDebugData->objTimer[i][TIME_TOTAL] > 1) {
            boxHeight += 8;
        }
    }
    rdpq_fill_rectangle(width - 104, 0, width, boxHeight);
    rdpq_mode_blender(0);
    boxHeight = 0;
    for (int i = 0; i < OBJ_TOTAL; i++) {
        if (i != PP_HALT && gDebugData->objTimer[i][TIME_TOTAL] > 1 && gDebugData->objHeader[i] != NULL) {
            boxHeight += 8;
            rdpq_text_printf(NULL, FONT_ARIAL, width - 100, 8 + boxHeight, "%s (%d):", gDebugData->objHeader[i]->name, (int) gDebugData->objCount[i]);
            rdpq_text_printf(NULL, FONT_ARIAL, width - 38, 8 + boxHeight, "%dus", gDebugData->objTimer[i][TIME_TOTAL]);
        }
    }
}

void profiler_draw_menu(void) {
    int y;
    
    rdpq_set_prim_color(RGBA32(0, 0, 0, 127));
    rdpq_set_mode_standard();
    rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
    rdpq_mode_combiner(RDPQ_COMBINER_FLAT);
    rdpq_fill_rectangle(10 - 2, 56, 112, 108 + 2);
    rdpq_mode_blender(0);

    y = -gDebugData->menuScroll * 10;
    for (int i = 0; i < PAGE_TOTAL; i++) {
        if (y <= -10) {
            y += 10;
            continue;
        }
        if (y >= 50) {
            break;
        }
        if (i == gDebugData->menuOption) {
            int sineCol = 128 + (32 * sins(gGameTimer * 0x400));
            rdpq_font_style(gFonts[FONT_ARIAL], 0, &(rdpq_fontstyle_t) { .color = RGBA32(255, sineCol, sineCol, 255),});
        } else {
            rdpq_font_style(gFonts[FONT_ARIAL], 0, &(rdpq_fontstyle_t) { .color = RGBA32(255, 255, 255, 255),});
        }
        rdpq_text_printf(NULL, FONT_ARIAL, 10, 66 + y, sPuppyPrintPageStrings[i]);
        y += 10;
    }
    rdpq_font_style(gFonts[FONT_ARIAL], 0, &(rdpq_fontstyle_t) { .color = RGBA32(255, 255, 255, 255),});
}

void render_profiler(void) {
    switch (gDebugData->menuPage) {
        case PAGE_MINIMAL:
            profiler_draw_minimal();
            break;
        case PAGE_OVERVIEW:
            profiler_draw_overview();
            break;
        case PAGE_BREAKDOWN:
            profiler_draw_breakdown();
            break;
        case PAGE_OBJECTS:
            profiler_draw_objects();
            break;
    }
    if (gDebugData->menuOpen) {
        profiler_draw_menu();
    }
}

#endif