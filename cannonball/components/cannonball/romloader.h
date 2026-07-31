/***************************************************************************
    Binary File Loader. 
    
    Handles loading an individual binary file to memory.
    Supports reading bytes, words and longs from this area of memory.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once
#include <stdint.h>

enum 
{
    ROMLOADER_NORMAL = 1, 
    ROMLOADER_INTERLEAVE2 = 2, 
    ROMLOADER_INTERLEAVE4 = 4
};

typedef struct
{
    uint8_t* rom;

    // Size of rom
    uint32_t length;

    // Successfully loaded
    uint8_t loaded;
} RomLoader;


void RomLoader_create(RomLoader* romLoader);
void RomLoader_init(RomLoader* romLoader, uint32_t);
int RomLoader_load(RomLoader* romLoader, const char* filename, const int offset, const int length, const int expected_crc, const uint8_t mode/* = NORMAL*/);
int RomLoader_load_binary(RomLoader* romLoader, const char* filename);
void RomLoader_unload(RomLoader* romLoader);

// ----------------------------------------------------------------------------
// Used by translated 68000 Code
// ----------------------------------------------------------------------------
uint32_t RomLoader_read32IncP(RomLoader* romLoader, uint32_t* addr);
uint16_t RomLoader_read16IncP(RomLoader* romLoader, uint32_t* addr);
uint8_t RomLoader_read8IncP(RomLoader* romLoader, uint32_t* addr);
uint32_t RomLoader_read32(RomLoader* romLoader, uint32_t addr);
uint16_t RomLoader_read16(RomLoader* romLoader, uint32_t addr);
uint8_t RomLoader_read8(RomLoader* romLoader, uint32_t addr);


// ----------------------------------------------------------------------------
// Used by translated Z80 Code
// Note that the endian is reversed compared with the 68000 code.
// ----------------------------------------------------------------------------
uint16_t RomLoader_read16IncP_addr16(RomLoader* romLoader, uint16_t* addr);
uint8_t RomLoader_read8IncP_addr16(RomLoader* romLoader, uint16_t* addr);
uint16_t RomLoader_read16_addr16(RomLoader* romLoader, uint16_t addr);
uint8_t RomLoader_read8_addr16(RomLoader* romLoader, uint16_t addr);
