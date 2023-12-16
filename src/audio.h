#pragma once

#define MIXER_BUFFER_SIZE 2
#define AUDIO_FREQUENCY 22050

enum SoundChannelIDs {
    CHANNEL_GLOBAL,
    CHANNEL_MENU,
    CHANNEL_ENV1,
    CHANNEL_ENV2,
    CHANNEL_ENV3,
    CHANNEL_DYNAMIC
};

enum SoundIDs {
    SOUND_LASER,
    SOUND_CANNON,
    SOUND_STEP_STONE,
    SOUND_MENU_CLICK,
    SOUND_SHELL1,

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

extern float gMusicVolume;
extern float gSoundVolume;

void init_audio(void);
void audio_loop(int updateRate, float updateRateF);
void set_sound_channel_count(int channelCount);
void play_sound_global(int soundID);
void play_sound_spatial(int soundID, float pos[3]);
void set_background_music(int seqID, int fadeTime);
void play_sound_spatial_pitch(int soundID, float pos[3], float pitch);
void set_music_volume(float volume);
void sound_channel_off(int channel);
void sound_channel_on(int channel);
