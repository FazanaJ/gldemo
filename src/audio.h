#pragma once

enum SoundIDs {
    SOUND_LASER,

    SOUND_TOTAL
};

void init_audio(void);
void audio_loop(int updateRate, float updateRateF);
void play_sound_global(int soundID);