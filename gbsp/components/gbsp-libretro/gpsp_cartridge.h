#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Capacity hints, not a replacement for runtime SRAM/Flash/EEPROM detection.
 * Preserve database-required 128 KiB and normal SDK Flash identification.
 * Call only at ROM startup on already loaded bytes; no paging or allocation. */
static inline unsigned gpsp_flash_capacity_kb(const uint8_t *rom, size_t size,
                                             unsigned current)
{
    if (current == 128) return 128;
    if (!rom) return 64;
    for (size_t i = 0; i < size; ++i) {
        if (rom[i] != 'F') continue;
        size_t left = size - i;
        if (left >= 9 && !memcmp(rom + i, "FLASH1M_V", 9)) return 128;
    }
    return 64;
}
