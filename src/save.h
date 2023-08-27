#pragma once


#define SAVE_MAGIC_NUMBER 0x14 // ¡Qué grande eres magic!
#define EXPECTED_MEMSIZE sizeof(ConfigBits)

extern const eepfs_entry_t eeprom_16k_files[1];

void init_save_data(void);
void write_config(void);