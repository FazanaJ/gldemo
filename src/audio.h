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

    SOUND_TOTAL
};

void init_audio(void);
void audio_loop(int updateRate, float updateRateF);
void set_sound_channel_count(int channelCount);
void play_sound_global(int soundID);
void play_sound_spatial(int soundID, float pos[3]);