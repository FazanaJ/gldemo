#pragma once

void init_hud(void);
void process_hud(int updateRate, float updateRateF);
void render_hud(int updateRate, float updateRateF);
void add_subtitle(char *text, int timer);