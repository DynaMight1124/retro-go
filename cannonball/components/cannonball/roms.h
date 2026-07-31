/***************************************************************************
    Load OutRun ROM Set.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once
#include <stdint.h>
#include "romloader.h"

// Western ROMs
extern RomLoader Roms_rom0;
extern RomLoader Roms_rom1;
extern RomLoader Roms_tiles;
extern RomLoader Roms_sprites;
extern RomLoader Roms_road;
extern RomLoader Roms_z80;
extern RomLoader Roms_pcm;

// Japanese ROMs
extern RomLoader Roms_j_rom0;
extern RomLoader Roms_j_rom1;

// Paged Roms (Swap between Jap and Western)
extern RomLoader* Roms_rom0p;
extern RomLoader* Roms_rom1p;

uint8_t Roms_load_revb_roms();
uint8_t Roms_load_japanese_roms();
uint8_t Roms_load_pcm_rom(uint8_t);

