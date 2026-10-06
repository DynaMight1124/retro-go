#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
    MK64_ROM_OK,
    MK64_ROM_NOT_FOUND,
    MK64_ROM_INVALID,
    MK64_ROM_UNSUPPORTED_REGION,
    MK64_ROM_IO_ERROR,
} mk64_rom_result_t;

typedef struct {
    FILE *file;
    uint32_t size;
    uint8_t byte_order;
} mk64_rom_t;

/* Open the launcher-selected ROM. Reads return canonical big-endian N64 bytes. */
mk64_rom_result_t mk64_rom_open(mk64_rom_t *rom, const char *path);
bool mk64_rom_read(mk64_rom_t *rom, uint32_t offset, void *dst, size_t length);
void mk64_rom_close(mk64_rom_t *rom);
