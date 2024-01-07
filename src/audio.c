#include <libdragon.h>
#include <malloc.h>

#include "audio.h"
#include "../include/global.h"

#include "debug.h"
#include "main.h"
#include "math_util.h"
#include "camera.h"
#include "menu.h"
#include "hud.h"
#include "object.h"

char gSoundChannelNum = 0;
char gSoundPrioTable[32];
static short sNextSequenceID = -1;
static short sCurrentSequenceID = -1;
static short sSequenceFadeTimerSet;
static short sSequenceFadeTimer;
static int sChannelMask;
float gChannelVol[CHANNEL_MAX_NUM];
float gMusicVolume;
float gSoundVolume;
static xm64player_t sXMPlayer;

SoundData sSoundTable[] = {
    {"laser", 10},
    {"cannon", 10},
    {"stonestep", 10},
    {"mouseclick", 10},
    {"shell1", 10},
    {"textblip", 10},
};

VoiceData sVoiceTable[] = {
    {{"necromancy", 10}, "Necromancy may be legal in Cryodil,\nbut few will openly admit to practicing it,\nnow that the Mages Guild has banned it.", 420},
    {{"butwhy", 10}, "But why in the name of all hell am I quoting Oblivion?", 420},
};

SequenceData sSequenceTable[] = {
    {"ToysXM-8bit", 8},
    {"racer", 3},
};

void set_sound_channel_count(int channelCount) {
    mixer_close();
    mixer_init(channelCount);
    gSoundChannelNum = channelCount;
}

void set_music_volume(float volume) {
    xm64player_set_vol(&sXMPlayer, volume);
}

void sound_channel_off(int channel) {
    sChannelMask |= (1 << channel);
}

void sound_channel_on(int channel) {
    sChannelMask &= ~(1 << channel);
}

void update_sequence(int updateRate) {
    if (sNextSequenceID != sCurrentSequenceID) {
        sSequenceFadeTimer -= updateRate;
        if (sSequenceFadeTimer < 0) {
            SequenceData *s = &sSequenceTable[sNextSequenceID];
            sSequenceFadeTimer = 0;
            if (sXMPlayer.fh) {
                xm64player_close(&sXMPlayer);
            }
            xm64player_open(&sXMPlayer, asset_dir(s->seqPath, DFS_XM64));
            xm64player_set_vol(&sXMPlayer, gMusicVolume);
            xm64player_play(&sXMPlayer, gSoundChannelNum - s->channelCount);
            for (int i = gSoundChannelNum - s->channelCount; i < CHANNEL_MAX_NUM; i++) {
                gChannelVol[i] = 1.0f;
            }
            set_music_volume(gMusicVolume);
            sCurrentSequenceID = sNextSequenceID;
        } else {
            float fade;
            fade = ((float) sSequenceFadeTimer / (float) sSequenceFadeTimerSet) * gMusicVolume;
            if (fade < 0.0001f) {
                fade = 0.0001f;
            }
            xm64player_set_vol(&sXMPlayer, fade);
        }
    }
}

void update_sound(float updateRateF) {
    for (int i = 0; i < gSoundChannelNum; i++) {
        if (sChannelMask & (1 << i)) {
            if (gChannelVol[i] > 0.0f) {
                gChannelVol[i] -= 0.05f * updateRateF;
                if (gChannelVol[i] < 0.0f) {
                    gChannelVol[i] = 0.0f;
                }
                mixer_ch_set_vol(i, gChannelVol[i], gChannelVol[i]);
            }
        } else {
            if (gChannelVol[i] < 1.0f) {
                gChannelVol[i] += 0.05f * updateRateF;
                if (gChannelVol[i] > 1.0f) {
                    gChannelVol[i] = 1.0f;
                }
                mixer_ch_set_vol(i, gChannelVol[i], gChannelVol[i]);
            }
        }
    }
}

void audio_loop(int updateRate, float updateRateF) {
#ifdef PUPPYPRINT_DEBUG
    if (gDebugData && gDebugData->enabled) {
        DEBUG_SNAPSHOT_3();
        rspq_wait();
        get_time_snapshot(PP_HALT, DEBUG_SNAPSHOT_3_END);
    }
#endif
    DEBUG_SNAPSHOT_1();
    update_sound(updateRateF);
    update_sequence(updateRate);
    if (audio_can_write()) {    	
		short *buf = audio_write_begin();
		mixer_poll(buf, audio_get_buffer_length());
		audio_write_end();
	}
    get_time_snapshot(PP_AUDIO, DEBUG_SNAPSHOT_1_END);
#ifdef PUPPYPRINT_DEBUG
    if (gDebugData && gDebugData->enabled) {
        DEBUG_SNAPSHOT_3();
        rspq_wait();
        get_time_snapshot(PP_HALT, DEBUG_SNAPSHOT_3_END);
    }
#endif
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
    wav64_play(&sSoundTable[soundID].sound, CHANNEL_GLOBAL);
}

void play_sound_global_pitch(int soundID, float pitch) {
    wav64_play(&sSoundTable[soundID].sound, CHANNEL_GLOBAL);
    mixer_ch_set_freq(CHANNEL_GLOBAL, (32000 / 2) * pitch);
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

        absZ = (pos[2] < 0 ? -pos[2] : pos[2]);
        if (absZ > dist) {
            absZ = dist;
        }

        if (pos[0] == 0.0f && pos[2] == 0.0f) {
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
        mixer_ch_set_vol_pan(channel, volume * gSoundVolume, pan);
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

void voice_play(int voiceID, int subtitle) {
    mixer_ch_set_freq(CHANNEL_VOICE, AUDIO_FREQUENCY * 2);
    wav64_play(&sVoiceTable[voiceID].sound.sound, CHANNEL_VOICE);

    if (subtitle && gConfig.subtitles) {
        add_subtitle(sVoiceTable[voiceID].subtitle, sVoiceTable[voiceID].sound.sound.wave.len / 720);
    }
}

void voice_stop(void) {
    mixer_ch_stop(CHANNEL_VOICE);
}