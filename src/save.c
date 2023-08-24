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

void read_config(void) {
    int region = get_tv_type();
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
    sMusicVolume = (float) gConfig.musicVolume / (float) 9.0f;
    sSoundVolume = (float) gConfig.soundVolume / (float) 9.0f;
}

void write_config(void) {
    ConfigBits config;
    config.antiAliasing = gConfig.antiAliasing;
    config.dedither = gConfig.dedither;
    config.regionMode = gConfig.regionMode;
    config.screenMode = gConfig.screenMode;
    config.frameCap = gConfig.frameCap;
    config.soundMode = gConfig.soundMode;
    config.musicVolume = gConfig.musicVolume;
    config.soundVolume = gConfig.soundVolume;
    config.magic = SAVE_MAGIC_NUMBER;
    eepfs_write("config.dat", &config, sizeof(ConfigBits));
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

    int fileSize = 0;
    for (int i = 0; i < sizeof(eeprom_16k_files) / sizeof(eepfs_entry_t); i++) {
        fileSize += eeprom_16k_files[i].size;
    }

    debugf("Eeprom initialised. Size: %X bytes.\n", fileSize);

    read_config();
}