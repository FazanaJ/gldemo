#include <libdragon.h>

#include "save.h"
#include "../include/global.h"

#include "main.h"
#include "menu.h"
#include "audio.h"

#define SAVE_MAGIC_NUMBER 0x14 // ¡Qué grande eres magic!
#define EXPECTED_MEMSIZE sizeof(ConfigBits)

const eepfs_entry_t eeprom_16k_files[] = {
    {"config.dat", sizeof(ConfigBits)},
};

ConfigBits sSaveConfig;

void read_config(void) {
    int region = get_tv_type();
    if (region == PAL50) {
        gIsPal = true;
    }
    eepfs_read("config.dat", &sSaveConfig, sizeof(ConfigBits));
    if (sSaveConfig.magic != SAVE_MAGIC_NUMBER) {
        bzero(&sSaveConfig, sizeof(ConfigBits));
        gConfig.regionMode = region;
        gConfig.musicVolume = 9;
        gConfig.soundVolume = 9;
        gConfig.soundMode = SOUND_STEREO;
        sSaveConfig.magic = SAVE_MAGIC_NUMBER;
        debugf("Config failed to load: Generating new.\n");
    } else {
        gConfig.regionMode = sSaveConfig.regionMode;
        gConfig.musicVolume = sSaveConfig.musicVolume;
        gConfig.soundVolume = sSaveConfig.soundVolume;
        gConfig.soundMode = sSaveConfig.soundMode;
        debugf("Config loaded.\n");
    }
    gConfig.antiAliasing = sSaveConfig.antiAliasing;
    gConfig.dedither = sSaveConfig.dedither;
    gConfig.screenMode = sSaveConfig.screenMode;
    gConfig.frameCap = sSaveConfig.frameCap;
    sMusicVolume = (float) gConfig.musicVolume / (float) 9.0f;
    sSoundVolume = (float) gConfig.soundVolume / (float) 9.0f;
}

void write_config(void) {
    sSaveConfig.antiAliasing = gConfig.antiAliasing;
    sSaveConfig.dedither = gConfig.dedither;
    sSaveConfig.regionMode = gConfig.regionMode;
    sSaveConfig.screenMode = gConfig.screenMode;
    sSaveConfig.frameCap = gConfig.frameCap;
    sSaveConfig.soundMode = gConfig.soundMode;
    sSaveConfig.musicVolume = gConfig.musicVolume;
    sSaveConfig.soundVolume = gConfig.soundVolume;
    sSaveConfig.magic = SAVE_MAGIC_NUMBER;
    eepfs_write("config.dat", &sSaveConfig, sizeof(ConfigBits));
    debugf("Config saved.\n");
}

void init_save_data(void) {
    const eeprom_type_t eeprom_type = eeprom_present();
    if (eeprom_type != EEPROM_16K) {
        debugf("Eeprom doesn't exist!\n");
        return;
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

    read_config();
}