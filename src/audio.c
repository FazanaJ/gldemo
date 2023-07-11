#include <libdragon.h>
#include <malloc.h>

#include "audio.h"
#include "../include/global.h"

#include "debug.h"
#include "main.h"
#include "math_util.h"
#include "camera.h"

static char sSoundChannelNum = 0;
static char sSoundPrioTable[32];
static short sNextSequenceID = -1;
static short sCurrentSequenceID = -1;
static short sSequenceFadeTimerSet;
static short sSequenceFadeTimer;
float sMusicVolume;
float sSoundVolume;
static xm64player_t sXMPlayer;

#define CHANNEL_MAX_NUM 32

static SoundData sSoundTable[] = {
    {"rom:/laser.wav64", 10},
    {"rom:/cannon.wav64", 10},
    {"rom:/stonestep.wav64", 10},
};

static SequenceData sSequenceTable[] = {
    {"rom:/ToysXM-8bit.xm64", 8},
};

void set_sound_channel_count(int channelCount) {
    mixer_close();
    mixer_init(channelCount);
    sSoundChannelNum = channelCount;
}

void init_audio(void) {
    audio_init(AUDIO_FREQUENCY, MIXER_BUFFER_SIZE);
    mixer_init(24);
    sSoundChannelNum = 24;
    bzero(&sSoundPrioTable, sizeof(sSoundPrioTable));
    sMusicVolume = 1.0f;
    sSoundVolume = 1.0f;

    for (int i = 0; i < sizeof(sSoundTable) / sizeof(SoundData); i++) {
        wav64_open(&sSoundTable[i].sound, sSoundTable[i].path);
    }

    set_background_music(0, 0);
}

void set_music_volume(float volume) {
    xm64player_set_vol(&sXMPlayer, volume);
}

void update_sequence(int updateRate) {
    if (sNextSequenceID != sCurrentSequenceID) {
        sSequenceFadeTimer -= updateRate;
        if (sSequenceFadeTimer < 0) {
            SequenceData *s = &sSequenceTable[sNextSequenceID];
            sSequenceFadeTimer = 0;
            for (int i = 0; i < s->channelCount; i++) {
                mixer_ch_set_vol((sSoundChannelNum - s->channelCount) + i, sMusicVolume, sMusicVolume);
            }
            xm64player_open(&sXMPlayer, s->seqPath);
            xm64player_play(&sXMPlayer, s->channelCount);
            sCurrentSequenceID = sNextSequenceID;
        } else {
            float fade;
            fade = ((float) sSequenceFadeTimer / (float) sSequenceFadeTimerSet) * sMusicVolume;
            set_music_volume(fade);
            xm64player_set_vol(&sXMPlayer, fade);
        }
    }
}

void audio_loop(int updateRate, float updateRateF) {
    DEBUG_SNAPSHOT_1();
    update_sequence(updateRate);
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
    wav64_play(&sSoundTable[soundID].sound, 0);
}

int get_sound_pan(int channel, float pos[3]) {
    float volume = 1.0f;
    float pan;
    float dist = SQR(100.0f);
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
        mixer_ch_set_vol_pan(channel, volume * sSoundVolume, pan);
        return 1;
    } else {
        return 0;
    }
}

void play_sound_spatial(int soundID, float pos[3]) {
    int channel = find_sound_channel(sSoundTable[soundID].priority);
    if (get_sound_pan(channel, pos) == 0) {
        return;
    }
    mixer_ch_set_freq(channel, AUDIO_FREQUENCY * 2);
    wav64_play(&sSoundTable[soundID].sound, channel);
}

void play_sound_spatial_pitch(int soundID, float pos[3], float pitch) {
    int channel = find_sound_channel(sSoundTable[soundID].priority);
    if (get_sound_pan(channel, pos) == 0) {
        return;
    }
    mixer_ch_set_freq(channel, (AUDIO_FREQUENCY * 2) * pitch);
    wav64_play(&sSoundTable[soundID].sound, channel);
}

void set_background_music(int seqID, int fadeTime) {
    sNextSequenceID = seqID;
    sSequenceFadeTimerSet = fadeTime;
    sSequenceFadeTimer = fadeTime;
}