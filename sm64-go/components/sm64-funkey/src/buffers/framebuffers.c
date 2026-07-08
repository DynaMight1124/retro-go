#include <ultra64.h>
#include <rg_system.h>
#include "config.h"

// 0x70800 bytes refactored to pointers
u16 (*gFrameBuffersPtr)[SCREEN_WIDTH * SCREEN_HEIGHT];

#define gFrameBuffers gFrameBuffersPtr

void sm64_init_framebuffers(void) {
    gFrameBuffersPtr = rg_alloc(3 * SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16), MEM_SLOW);
}
