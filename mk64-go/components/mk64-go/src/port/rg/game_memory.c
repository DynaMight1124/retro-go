#include <stdint.h>

/* The original game uses a fixed, contiguous pool with upward and downward
 * allocation. Keep its address stable while the ROM assets occupy a separate
 * resident PSRAM region. */
__attribute__((section(".ext_ram.bss"), aligned(16)))
uint8_t gPortMemoryPool[0x300000];
