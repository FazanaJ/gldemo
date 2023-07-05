#include <libdragon.h>

#include "hud.h"
#include "../include/global.h"

#include "object.h"
#include "main.h"
#include "input.h"

void render_health(void) {
    if (gPlayer && gPlayer->data) {
        PlayerData *data = (PlayerData *) gPlayer->data;

        rdpq_font_begin(RGBA32(255, 0, 0, 255));
        rdpq_font_position(16, 16);
        rdpq_font_printf(gCurrentFont, "HP: %d/%d", data->health, data->healthMax);
        rdpq_font_end();
    }
}


void render_hud(void) {
    //render_health();

    if (gCurrentController == -1) {
        rdpq_font_begin(RGBA32(255, 0, 0, 255));
        rdpq_font_position((gFrameBuffers->width / 2) - 40, (gFrameBuffers->height / 2) - 40);
        rdpq_font_print(gCurrentFont, "Press Start");
        rdpq_font_end();
    }
}