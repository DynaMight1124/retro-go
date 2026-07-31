/***************************************************************************
    Load OutRun ROM Set.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <stdint.h>
#include "globals.h"
#include "roms.h"
#include "hwvideo/hwtiles.h"
#include "hwvideo/hwsprites.h"
#include "hwvideo/hwroad.h"

RomLoader Roms_rom0;
RomLoader Roms_rom1;
RomLoader Roms_tiles;
RomLoader Roms_sprites;
RomLoader Roms_road;
RomLoader Roms_z80;
RomLoader Roms_pcm;
RomLoader Roms_j_rom0;
RomLoader Roms_j_rom1;
RomLoader* Roms_rom0p;
RomLoader* Roms_rom1p;

int jap_rom_status = -1;

uint8_t Roms_load_revb_roms()
{
    // Implementation moved to app_main in main.c for memory efficiency
    return 1;
}

uint8_t Roms_load_japanese_roms()
{
    return 0; // Stub for now to save memory
}

uint8_t Roms_load_pcm_rom(uint8_t fixed_rom)
{
    return 1; // Already loaded in load_revb_roms
}
