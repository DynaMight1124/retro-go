#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "rom.h"

/* Validates the ROM and caches the MIO0 blocks described by the PSP recipes. */
bool mk64_assets_prepare(mk64_rom_t *rom, bool *created,
                         char *error, size_t error_size);
