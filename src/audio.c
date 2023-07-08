#include <libdragon.h>

#include "audio.h"
#include "../include/global.h"

#include "debug.h"
#include "main.h"
#include "math_util.h"
#include "camera.h"

static wav64_t sSoundTable[SOUND_TOTAL];

static char sSoundChannelNum = 0;
static char sSoundPrioTable[32];

#define CHANNEL_MAX_NUM 32

void set_sound_channel_count(int channelCount) {
    mixer_close();
    mixer_init(channelCount);
    sSoundChannelNum = channelCount;
}

void init_audio(void) {
    audio_init(AUDIO_FREQUENCY, MIXER_BUFFER_SIZE);
    mixer_init(16);
    sSoundChannelNum = 16;
    bzero(&sSoundPrioTable, sizeof(sSoundPrioTable));
    wav64_open(&sSoundTable[SOUND_LASER], "rom:/laser.wav64");
    wav64_open(&sSoundTable[SOUND_CANNON], "rom:/cannon.wav64");
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

static int find_sound_channel(int priority) {
    static int pris[CHANNEL_MAX_NUM] = { 0 };

    // Find a free channel
    int lowest = 0;
    int ch = -1;
    for (int i = 0; i < CHANNEL_MAX_NUM; i++) {
        if (!mixer_ch_playing(i + CHANNEL_DYNAMIC)) {
            ch = i;
            break;
        }
        if (pris[i] < pris[lowest]) {
            lowest = i;
        }
    }

    // If no free channel, use the lowest priority channel
    if (ch == -1) {
        ch = lowest;
    }

    // Remember the current priority
    pris[ch] = priority;
    
    // Return the channel ID
    return ch + CHANNEL_DYNAMIC;
}

void play_sound_global(int soundID) {
    wav64_play(&sSoundTable[soundID], 0);
}

int get_sound_pan(int channel, float pos[3]) {
    float volume = 1.0f;
    float pan;
    float dist = SQR(10.0f);
    volume = 1.0f - (DIST3(pos, gCamera->pos) / dist);
    if (gConfig.soundMode == SOUND_MONO) {
        pan = 0.5f;
    } else {
        float absX;
        float absZ;

        absX = (pos[0] < 0 ? -pos[0] : pos[0]);
        if (absX > dist) {
            absX = dist;
        }

        absZ = (pos[1] < 0 ? -pos[1] : pos[1]);
        if (absZ > dist) {
            absZ = dist;
        }

        if (pos[0] == 0.0f && pos[1] == 0.0f) {
            pan = 0.5f;
        } else if (pos[0] >= 0.0f && absX >= absZ) {
            pan = 1.0f - (2 * dist - absX) / (3.0f * (2 * dist - absZ));
        } else if (pos[0] < 0 && absX > absZ) {
            pan = (2 * dist - absX) / (3.0f * (2 * dist - absZ));
        } else {
            pan = 0.5f + pos[0] / (6.0f * absZ);
        }
    }
    if (volume > 0.0f) {
        mixer_ch_set_vol_pan(channel, volume, pan);
        return 1;
    } else {
        return 0;
    }
}

void play_sound_spatial(int soundID, float pos[3]) {
    int channel = find_sound_channel(10);
    if (get_sound_pan(channel, pos) == 0) {
        return;
    }
    mixer_ch_set_freq(channel, AUDIO_FREQUENCY * 2);
    wav64_play(&sSoundTable[soundID], channel);
}