#include <PR/ultratypes.h>
#include <rg_system.h>
#include "gfx_output_buffer.h"

u64 *gGfxSPTaskOutputBuffer;

void sm64_init_gfx_output_buffer(void) {
    gGfxSPTaskOutputBuffer = rg_alloc(GFX_OUTPUT_BUFFER_SIZE, MEM_SLOW);
}
