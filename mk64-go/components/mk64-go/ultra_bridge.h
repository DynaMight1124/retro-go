#pragma once

#include "rom.h"

/* ROM handle used by the N64 PI DMA shim while the game is running. */
void mk64_ultra_set_rom(mk64_rom_t *rom);

/* Retry native progress writes before menus/shutdown; no save-state claim. */
bool mk64_eeprom_flush(void);
