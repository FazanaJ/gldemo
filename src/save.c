#include <libdragon.h>

#include "save.h"
#include "../include/global.h"

#include "main.h"
#include "menu.h"
#include "audio.h"
#include "assets.h"

const eepfs_entry_t eeprom_16k_files[1] = {
    {"config.dat", sizeof(ConfigBits)},
};

void write_config(void) {
    ConfigBits config;
    config.graphics = gConfig.graphics;
    config.regionMode = gConfig.regionMode;
    config.screenMode = gConfig.screenMode;
    config.vsync = gConfig.vsync;
    config.soundMode = gConfig.soundMode;
    config.musicVolume = gConfig.musicVolume;
    config.soundVolume = gConfig.soundVolume;
    config.magic = SAVE_MAGIC_NUMBER;
    eepfs_write("config.dat", &config, sizeof(ConfigBits));
    debugf("Config saved.\n");
}
