#include <ultra64.h>
#include <rg_system.h>
#include "config.h"
#include "zbuffer.h"

u16 *gZBuffer;

void sm64_init_zbuffer(void) {
    gZBuffer = rg_alloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u16), MEM_SLOW);
    if (!gZBuffer) {
        RG_PANIC("Failed to allocate SM64 native zbuffer!");
    }
}
