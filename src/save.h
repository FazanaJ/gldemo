#pragma once

#include "../include/global.h"

#define SAVE_MAGIC_NUMBER 0x14 // ¡Qué grande eres magic!
#define EXPECTED_MEMSIZE sizeof(ConfigBits)

enum PakErrorCodes {
    PAK_ERROR_WRONGTYPE = -3,
    PAK_BAD,
    PAK_DETECTED = 0,
};

extern const eepfs_entry_t eeprom_16k_files[1];

void save_find_paks(void);
void init_save_data(void);
void save_config_write(void);