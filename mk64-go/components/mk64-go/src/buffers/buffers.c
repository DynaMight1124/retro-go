#include <ultra64.h>
#include <macros.h>
#include <mk64.h>
#include "buffers.h"

/**
 * @brief look like to be a buffer of decoded textures
 */
ALIGNED8 __attribute__((section(".ext_ram.bss"))) union_D_802BFB80 D_802BFB80;
ALIGNED8 __attribute__((section(".ext_ram.bss"))) struct_D_802DFB80 gEncodedKartTexture[2][2][8];
#ifdef AVOID_UB
ALIGNED8 __attribute__((section(".ext_ram.bss"))) struct_D_802F1F80 gPlayerPalettesList[2][4][8];
#else
ALIGNED8 u16 gPlayerPalettesList[2][4][0x100 * 8];
#endif

ALIGNED8 __attribute__((section(".ext_ram.bss"))) u16 gZBuffer[PORT_N64_FB_TEXELS];

#ifdef AVOID_UB
ALIGNED8 __attribute__((section(".ext_ram.bss"))) u16 gFramebuffers[3][PORT_N64_FB_TEXELS];
#else
u16 gFramebuffer0[PORT_N64_FB_TEXELS];
u16 gFramebuffer1[PORT_N64_FB_TEXELS];
u16 gFramebuffer2[PORT_N64_FB_TEXELS];
#endif
