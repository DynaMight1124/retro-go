#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "stdint.h"
#include "romloader.h"
#include "thirdparty/crc/crc.h"

#ifdef __APPLE__
#include "CoreFoundation/CoreFoundation.h"
#endif

#ifdef RETRO_GO
char rom_base_path[256] = "";
#endif

int RomLoader_filesize(const char* filename)
{
    char fullpath[512];
#ifdef RETRO_GO
    snprintf(fullpath, sizeof(fullpath), "%s/%s", rom_base_path, filename);
#else
    strncpy(fullpath, filename, sizeof(fullpath));
#endif
    FILE* file = fopen(fullpath, "rb");
    if (!file) return 0;
    fseek(file, 0L, SEEK_END);
    uint32_t size = ftell(file);
    fclose(file);
    return size; 
}

void RomLoader_create(RomLoader* romLoader)
{
    romLoader->loaded = 0;
    romLoader->rom = NULL;
}

void RomLoader_init(RomLoader* romLoader, uint32_t length)
{
    romLoader->length = length;
    if (romLoader->rom) free(romLoader->rom);
    romLoader->rom = (uint8_t*)malloc(length);
    if (!romLoader->rom) {
        fprintf(stderr, "RomLoader: Failed to allocate %d bytes for rom\n", (int)length);
    }
    romLoader->loaded = 0;
}

void RomLoader_unload(RomLoader* romLoader)
{
    if (romLoader->rom) free(romLoader->rom);
    romLoader->rom = NULL;
    romLoader->loaded = 0;
}

int RomLoader_load(RomLoader* romLoader, const char* filename, const int offset, const int length, const int expected_crc, const uint8_t interleave)
{
    FILE* file;
    uint32_t i;

    char fullpath[512];
#ifdef RETRO_GO
    snprintf(fullpath, sizeof(fullpath), "%s/%s", rom_base_path, filename);
#else
    strncpy(fullpath, filename, sizeof(fullpath));
#endif
    // Open rom file
	file = fopen(fullpath, "rb");

    if (!file)
    {
        romLoader->loaded = 0;
        return 1; // fail
    }

    const int CHUNK_SIZE = 4096;
    uint8_t* chunk = (uint8_t*)malloc(CHUNK_SIZE);
    if (!chunk) {
        fprintf(stderr, "RomLoader: Failed to allocate chunk buffer\n");
        fclose(file);
        return 1;
    }

    uint32_t bytes_read = 0;
    while (bytes_read < length)
    {
        uint32_t to_read = length - bytes_read;
        if (to_read > CHUNK_SIZE) to_read = CHUNK_SIZE;

        int read = fread(chunk, 1, to_read, file);
        if (read <= 0) break;

        if (romLoader->rom) {
            for (i = 0; i < read; i++)
            {
                romLoader->rom[((bytes_read + i) * interleave) + offset] = chunk[i];
            }
        }
        bytes_read += read;
    }

    free(chunk);
    fclose(file);
    romLoader->loaded = 1;
    return 0; // success
}

int RomLoader_load_binary(RomLoader* romLoader, const char* filename)
{
    char fullpath[512];
#ifdef RETRO_GO
    snprintf(fullpath, sizeof(fullpath), "%s/%s", rom_base_path, filename);
#else
    strncpy(fullpath, filename, sizeof(fullpath));
#endif

    FILE* file = fopen(fullpath, "rb");

    if (!file)
    {
        romLoader->loaded = 0;
        return 1; // fail
    }

    romLoader->length = RomLoader_filesize(filename);
    if (romLoader->rom) free(romLoader->rom);
    romLoader->rom = (uint8_t*)malloc(romLoader->length);
    
    if (!romLoader->rom) {
        fprintf(stderr, "RomLoader: Failed to allocate %d bytes for rom\n", (int)romLoader->length);
        fclose(file);
        return 1;
    }

    fread(romLoader->rom, romLoader->length, 1, file);
    fclose(file);

    romLoader->loaded = 1;
    return 0; // success
}

uint32_t RomLoader_read32(RomLoader* romLoader, uint32_t addr)
{
    if (!romLoader->rom || addr + 3 >= romLoader->length) return 0;
    return (romLoader->rom[addr] << 24) | (romLoader->rom[addr+1] << 16) | (romLoader->rom[addr+2] << 8) | (romLoader->rom[addr+3]);
}

uint16_t RomLoader_read16(RomLoader* romLoader, uint32_t addr)
{
    if (!romLoader->rom || addr + 1 >= romLoader->length) return 0;
    return (romLoader->rom[addr] << 8) | (romLoader->rom[addr+1]);
}

uint8_t RomLoader_read8(RomLoader* romLoader, uint32_t addr)
{
    if (!romLoader->rom || addr >= romLoader->length) return 0;
    return romLoader->rom[addr];
}

uint32_t RomLoader_read32IncP(RomLoader* romLoader, uint32_t* addr)
{
    uint32_t data = RomLoader_read32(romLoader, *addr);
    *addr += 4;
    return data;
}

uint16_t RomLoader_read16IncP(RomLoader* romLoader, uint32_t* addr)
{
    uint16_t data = RomLoader_read16(romLoader, *addr);
    *addr += 2;
    return data;
}

uint8_t RomLoader_read8IncP(RomLoader* romLoader, uint32_t* addr)
{
    if (!romLoader->rom || *addr >= romLoader->length) return 0;
    return romLoader->rom[(*addr)++];
}

uint16_t RomLoader_read16IncP_addr16(RomLoader* romLoader, uint16_t* addr)
{
    if (!romLoader->rom || *addr + 1 >= romLoader->length) return 0;
    uint16_t data = (romLoader->rom[*addr+1] << 8) | (romLoader->rom[*addr]);
    *addr += 2;
    return data;
}

uint8_t RomLoader_read8IncP_addr16(RomLoader* romLoader, uint16_t* addr)
{
    if (!romLoader->rom || *addr >= romLoader->length) return 0;
    return romLoader->rom[(*addr)++];
}

uint16_t RomLoader_read16_addr16(RomLoader* romLoader, uint16_t addr)
{
    if (!romLoader->rom || addr + 1 >= romLoader->length) return 0;
    return (romLoader->rom[addr+1] << 8) | (romLoader->rom[addr]);
}

uint8_t RomLoader_read8_addr16(RomLoader* romLoader, uint16_t addr)
{
    if (!romLoader->rom || addr >= romLoader->length) return 0;
    return romLoader->rom[addr];
}
