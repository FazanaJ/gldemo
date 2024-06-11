#include <libdragon.h>
#include <malloc.h>

#include "../../include/global.h"

#include "../main.h"
#include "../assets.h"
#include "../render.h"
#include "../input.h"
#include "../debug.h"
#include "../hud.h"
#include "../save.h"
#include "../audio.h"
#include "../menu.h"
#include "../save.h"

static const resolution_t RESOLUTION_304x224 = {SCREEN_WIDTH, SCREEN_HEIGHT, false};
static const resolution_t RESOLUTION_384x224 = {SCREEN_WIDTH_16_10, SCREEN_HEIGHT, false};
static const resolution_t RESOLUTION_408x224 = {SCREEN_WIDTH_16_9, SCREEN_HEIGHT, false};

static const resolution_t sVideoModes[] = {
    RESOLUTION_304x224,
    RESOLUTION_384x224,
    RESOLUTION_408x224
};

void read_config(void) {
    int region = get_tv_type();
    int write = false;
    if (region == PAL50) {
        gIsPal = true;
    }
    ConfigBits config;
    eepfs_read("cfg.dat", &config, sizeof(ConfigBits));
    debugf("Magic Number is: 0x%X\n", config.magic);
    if (config.magic != SAVE_MAGIC_NUMBER) {
        bzero(&config, sizeof(ConfigBits));
        gConfig.regionMode = region;
        gConfig.musicVolume = 10;
        gConfig.soundVolume = 10;
        gConfig.soundMode = SOUND_STEREO;
        gConfig.subtitles = true;
        config.magic = SAVE_MAGIC_NUMBER;
        write = true;
        debugf("Config failed to load: Generating new.\n");
    } else {
        gConfig.regionMode = config.regionMode;
        gConfig.musicVolume = config.musicVolume;
        gConfig.soundVolume = config.soundVolume;
        gConfig.soundMode = config.soundMode;
        gConfig.subtitles = config.subtitles;
        debugf("Config loaded.\n");
    }
    gConfig.graphics = config.graphics;
    gConfig.screenMode = config.screenMode;
    gConfig.vsync = config.vsync;
    gMusicVolume = (float) gConfig.musicVolume / (float) 10.0f;
    gSoundVolume = (float) gConfig.soundVolume / (float) 10.0f;
    if (write) {
        save_config_write();
    }
}

void error_print(char *file) {
    rdpq_init();
    display_init(RESOLUTION_640x480, DEPTH_16_BPP, 1, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    surface_t *disp = display_get();
    rdpq_attach(disp, NULL);
    rdpq_set_mode_copy(true);
    sprite_t *background = sprite_load(asset_dir(file, DFS_SPRITE));
    rdpq_mode_tlut(TLUT_RGBA16);
    rdpq_tex_upload_tlut(sprite_get_palette(background), 0, 256);
    rdpq_sprite_blit(background, 0, 0, NULL);
    rdpq_detach_show();
    while (1);
}

void init_save_data(void) {
    const eeprom_type_t eeprom_type = eeprom_present();
    if (eeprom_type != EEPROM_16K) {
        debugf("Eeprom doesn't exist!\n");
        error_print("save_error.ci8");
    }
    int result = eepfs_init(eeprom_16k_files, sizeof(eeprom_16k_files) / sizeof(eepfs_entry_t));
    if (result != EEPFS_ESUCCESS) {
        debugf("Eeprom failed: %d\n", result);
        return;
    }
    if (eepfs_verify_signature() == 0) {
        /* If not, erase it and start from scratch */
        //debugf("Filesystem signature is invalid!\n");
        //debugf("Wiping EEPROM...\n" );
        //eepfs_wipe();
    }

    int fileSize = 0;
    for (int i = 0; i < sizeof(eeprom_16k_files) / sizeof(eepfs_entry_t); i++) {
        fileSize += eeprom_16k_files[i].size;
    }

    debugf("Eeprom initialised. Size: %X bytes.\n", fileSize);

    read_config();
}

#ifdef PUPPYPRINT_DEBUG
void init_debug(void) {
    debug_init_isviewer();
    debug_init_usblog();
    rspq_profile_start();
    //console_init();
    //console_set_render_mode(RENDER_MANUAL);
    //rdpq_debug_start();
    //rdpq_debug_log(true);
    gDebugData = malloc(sizeof(DebugData));
    bzero(gDebugData, sizeof(DebugData));
    gDebugData->enabled = false;
    //gDebugData->debugMeshes[0] = MODEL_LOAD("debugcylinder");
}
#endif

extern int __boot_tvtype;

void set_region_type(int region) {
    if (region == PAL60) {
        region = NTSC60;
    }
    __boot_tvtype = region; // Writes to osTvType
    display_init(sVideoModes[(unsigned) gConfig.screenMode], DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    //gZBuffer = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());
    gZBuffer.flags = FMT_RGBA16 | SURFACE_FLAGS_OWNEDBUFFER;
    gZBuffer.width = display_get_width();
    gZBuffer.height = display_get_height();
    gZBuffer.stride = TEX_FORMAT_PIX2BYTES(FMT_RGBA16, display_get_width());
    gZBuffer.buffer = (void *) ((0x80800000 - 0x10000) - ((display_get_width() * display_get_height()) * 2));
#if OPENGL
    gl_init();
#elif TINY3D
    t3d_init();
    rdpq_init();
#endif
    init_renderer();
}

void init_memory(void) {
    timer_init();
    asset_init_compression(3);
    if (get_memory_size() == 0x400000) {
        error_print("memory_error.ci8");
    }
}

void init_controller(void) {
    joypad_init();
    bzero(&gInputData, sizeof(Input));
    gInputData.pak = 0;
}

void init_audio(void) {
    audio_init(AUDIO_FREQUENCY, MIXER_BUFFER_SIZE);
    mixer_init(24);
    gSoundChannelNum = 24;
    bzero(&gSoundPrioTable, sizeof(gSoundPrioTable));

    for (int i = 0; i < SOUND_TOTAL; i++) {
        wav64_open(&sSoundTable[i].sound, asset_dir(sSoundTable[i].path, DFS_WAV64));
    }
    for (int i = 0; i < VOICE_TOTAL; i++) {
        wav64_open(&sVoiceTable[i].sound.sound, asset_dir(sVoiceTable[i].sound.path, DFS_WAV64));
    }
    //set_background_music(1, 0);
    for (int i = 0; i < CHANNEL_MAX_NUM; i++) {
        gChannelVol[i] = 1.0f;
        mixer_ch_set_limits(i, 16, 22050, 0);
    }
    mixer_ch_set_limits(CHANNEL_VOICE, 16, 12000, 0);
}

void init_game(void) {
    init_memory();
    init_controller();
    init_hud();
    init_audio();
    init_debug();
    init_save_data();
    save_find_paks();
    set_region_type(gConfig.regionMode);
    load_font(FONT_ARIAL);
    load_font(FONT_MVBOLI);
    gGlobalTimer = 0;
    gGameTimer = 0;
}