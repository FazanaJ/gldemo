#pragma once

#include "../include/global.h"
#include "object.h"

#define MIXER_BUFFER_SIZE 2
#define AUDIO_FREQUENCY 22050
#define CHANNEL_MAX_NUM 32

enum SoundChannelIDs {
    CHANNEL_GLOBAL,
    CHANNEL_MENU,
    CHANNEL_ENV1,
    CHANNEL_ENV2,
    CHANNEL_ENV3,
    CHANNEL_VOICE,
    CHANNEL_DYNAMIC
};

enum VoiceIDs {
    VOICE_NECROMANCY,
    VOICE_BUTWHY,

    VOICE_TOTAL
};

#define VOICE_NULL 65535

enum SoundIDs {
    SOUND_LASER,
    SOUND_CANNON,
    SOUND_MENU_CLICK,
    SOUND_SHELL1,
    SOUND_TEXTBLIP,
    SOUND_KEYBOARD,
    SOUND_STEP_DIRT,
    SOUND_STEP_GLASS,
    SOUND_STEP_GRASS,
    SOUND_STEP_GRAVEL,
    SOUND_STEP_MESH,
    SOUND_STEP_METAL,
    SOUND_STEP_SAND,
    SOUND_STEP_SNOW,
    SOUND_STEP_STONE,
    SOUND_STEP_TILE,
    SOUND_STEP_WOOD,

    SOUND_TOTAL
};

typedef struct SequenceData {
    char *seqPath;
    char channelCount;
    char pad[3];
} SequenceData;

typedef struct SoundData {
    char *path;
    unsigned char priority;
    wav64_t sound;
} SoundData;

typedef struct VoiceData {
    SoundData sound;
    char *subtitle;
    unsigned short timer;
} VoiceData;

typedef struct AudioEmitter {
    float pos[3];
    float pitch;
    float volume;
    unsigned short soundID;
    // padding
} AudioEmitter;

extern SoundData sSoundTable[];
extern VoiceData sVoiceTable[];
extern SequenceData sSequenceTable[];
extern char gSoundChannelNum;
extern char gSoundPrioTable[32];
extern float gChannelVol[CHANNEL_MAX_NUM];

extern float gMusicVolume;
extern float gSoundVolume;

void init_audio(void);
void audio_loop(int updateRate, float updateRateF);
void set_sound_channel_count(int channelCount);
void play_sound_global(int soundID);
void play_sound_global_pitch(int soundID, float pitch);
void play_sound_spatial(int soundID, float pos[3]);
void set_background_music(int seqID, int fadeTime);
void play_sound_spatial_pitch(int soundID, float pos[3], float pitch);
void set_music_volume(float volume);
void sound_channel_off(int channel);
void sound_channel_on(int channel);
void voice_play(int voiceID, int subtitle);
void voice_stop(void);
