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

static const resolution_t RESOLUTION_304x224 = {320, 240, false};
static const resolution_t RESOLUTION_384x224 = {384, 240, false};
static const resolution_t RESOLUTION_408x224 = {424, 240, false};

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
    eepfs_read("config.dat", &config, sizeof(ConfigBits));
    if (config.magic != SAVE_MAGIC_NUMBER) {
        bzero(&config, sizeof(ConfigBits));
        gConfig.regionMode = region;
        gConfig.musicVolume = 9;
        gConfig.soundVolume = 9;
        gConfig.soundMode = SOUND_STEREO;
        config.magic = SAVE_MAGIC_NUMBER;
        write = true;
        debugf("Config failed to load: Generating new.\n");
    } else {
        gConfig.regionMode = config.regionMode;
        gConfig.musicVolume = config.musicVolume;
        gConfig.soundVolume = config.soundVolume;
        gConfig.soundMode = config.soundMode;
        debugf("Config loaded.\n");
    }
    gConfig.antiAliasing = config.antiAliasing;
    gConfig.dedither = config.dedither;
    gConfig.screenMode = config.screenMode;
    gConfig.frameCap = config.frameCap;
    gMusicVolume = (float) gConfig.musicVolume / (float) 9.0f;
    gSoundVolume = (float) gConfig.soundVolume / (float) 9.0f;
    if (write) {
        write_config();
    }
}

void save_doesnt_exist(void) {
    rdpq_init();
    display_init(RESOLUTION_640x480, DEPTH_16_BPP, 1, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    surface_t *disp = display_get();
    rdpq_attach(disp, NULL);
    rdpq_set_mode_copy(true);
    sprite_t *background = sprite_load(asset_dir("save_error.ci8", DFS_SPRITE));
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
        save_doesnt_exist();
    }
    int result = eepfs_init(eeprom_16k_files, sizeof(eeprom_16k_files) / sizeof(eepfs_entry_t));
    if (result != EEPFS_ESUCCESS) {
        debugf("Eeprom failed: %d\n", result);
        return;
    }
    if (eepfs_verify_signature() == 0) {
        /* If not, erase it and start from scratch */
        debugf("Filesystem signature is invalid!\n");
        debugf("Wiping EEPROM...\n" );
        eepfs_wipe();
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
}
#endif

void set_region_type(int region) {
    if (region == PAL60) {
        region = NTSC60;
    }
    *(uint32_t*) 0x80000300 = region; // Writes to osTvType
    display_init(sVideoModes[(unsigned) gConfig.screenMode], DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
    //gZBuffer = surface_alloc(FMT_RGBA16, display_get_width(), display_get_height());
    gZBuffer.flags = FMT_RGBA16 | SURFACE_FLAGS_OWNEDBUFFER;
    gZBuffer.width = display_get_width();
    gZBuffer.height = display_get_height();
    gZBuffer.stride = TEX_FORMAT_PIX2BYTES(FMT_RGBA16, display_get_width());
    gZBuffer.buffer = (void *) ((0x80800000 - 0x10000) - ((display_get_width() * display_get_height()) * 2));
    gl_init();
    init_renderer();
}

void memory_error_screen(void) {
    if (get_memory_size() == 0x400000) {
        rdpq_init();
        display_init(RESOLUTION_640x480, DEPTH_16_BPP, 1, GAMMA_NONE, ANTIALIAS_RESAMPLE_FETCH_ALWAYS);
        surface_t *disp = display_get();
        rdpq_attach(disp, NULL);
        rdpq_set_mode_copy(true);
        sprite_t *background = sprite_load(asset_dir("memory_error.ci8", DFS_SPRITE));
        rdpq_mode_tlut(TLUT_RGBA16);
        rdpq_tex_upload_tlut(sprite_get_palette(background), 0, 256);
        rdpq_sprite_blit(background, 0, 0, NULL);
        rdpq_detach_show();
        while (1);
    }
}

void init_memory(void) {
    timer_init();
    memory_error_screen();
}

void init_controller(void) {
    controller_init();
    bzero(&gInputData, sizeof(Input));
    gInputData.pak = 0;
}

void init_game(void) {
    init_memory();
    init_controller();
    init_hud();
    init_audio();
    init_debug();
    init_save_data();
    set_region_type(gConfig.regionMode);
    load_font(FONT_ARIAL);
    load_font(FONT_MVBOLI);
    gGlobalTimer = 0;
    gGameTimer = 0;
}