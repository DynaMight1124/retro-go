#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rom.h"
#include "course_layout.h"

/* Stable PSRAM address for the eventual game asset symbols. */
extern uint8_t gMk64AssetRegion[MK64_ASSET_REGION_SIZE];

/* Loads all ROM recipe classes into the resident region. */
bool mk64_resident_load(mk64_rom_t *rom, char *error, size_t error_size);
