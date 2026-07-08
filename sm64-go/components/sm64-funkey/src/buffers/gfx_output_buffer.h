#ifndef GFX_OUTPUT_BUFFER_H
#define GFX_OUTPUT_BUFFER_H

#include <PR/ultratypes.h>

#ifdef VERSION_EU
#define GFX_OUTPUT_BUFFER_SIZE (0x2fc0 * sizeof(u64))
#else
#define GFX_OUTPUT_BUFFER_SIZE (0x3e00 * sizeof(u64))
#endif

extern u64 *gGfxSPTaskOutputBuffer;

void sm64_init_gfx_output_buffer(void);

#endif // GFX_OUTPUT_BUFFER_H
