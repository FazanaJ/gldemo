#include <libdragon.h>

#include "audio.h"
#include "../include/global.h"

#include "debug.h"

static wav64_t sSoundTable[SOUND_TOTAL];

void init_audio(void) {
    audio_init(22050, 4);
	mixer_init(16);
    wav64_open(&sSoundTable[SOUND_LASER], "rom:/laser.wav64");
}

void audio_loop(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    if (audio_can_write()) {    	
		short *buf = audio_write_begin();
		mixer_poll(buf, audio_get_buffer_length());
		audio_write_end();
	}
    get_time_snapshot(PP_AUDIO, DEBUG_SNAPSHOT_1_END);
}

void play_sound_global(int soundID) {
    wav64_play(&sSoundTable[soundID], 0);
}