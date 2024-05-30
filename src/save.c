#include <libdragon.h>

#include "save.h"
#include "../include/global.h"

#include "main.h"
#include "menu.h"
#include "audio.h"
#include "assets.h"

char gSavePaks[4];
char gControllerPaks[4];

void save_find_paks(void) {
    debugf("Searching for controller paks...\n");
    for (int i = 0; i < 4; i++) {
        int mem = validate_mempak(i);
        gSavePaks[i] = mem;
        gControllerPaks[i] = joypad_get_accessory_type(i);
        if (mem == 0) {
            debugf("Controller Pak #%d is detected.\n", i + 1);
        }
    }
}

const eepfs_entry_t eeprom_16k_files[1] = {
    {"config.dat", sizeof(ConfigBits)},
};

void save_config_write(void) {
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
