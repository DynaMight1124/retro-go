#include <ultra64.h>
#include <rg_system.h>
#include "buffers.h"

/* Buffers refactored to use pointers for PSRAM allocation via rg_alloc */

u8 *gDecompressionHeap;
u8 *gAudioHeap;

ALIGNED8 u8 gIdleThreadStack[0x800];
ALIGNED8 u8 gThread3Stack[0x2000];
ALIGNED8 u8 gThread4Stack[0x2000];
ALIGNED8 u8 gThread5Stack[0x2000];
#ifdef VERSION_SH
ALIGNED8 u8 gThread6Stack[0x2000];
#endif

// 0x400 bytes
ALIGNED8 u8 gGfxSPTaskStack[SP_DRAM_STACK_SIZE8];
// 0xc00 bytes for f3dex, 0x900 otherwise
ALIGNED8 u8 gGfxSPTaskYieldBuffer[OS_YIELD_DATA_SIZE];

struct SaveBuffer *gSaveBufferPtr;
struct GfxPool *gGfxPools;

#ifdef VERSION_JP
u8 *gAudioSPTaskYieldBuffer;
#endif

#if !defined(F3DEX_GBI_SHARED) && !defined(VERSION_EU)
u8 *gUnusedThread2Stack;
#endif

void sm64_init_buffers(void) {
    gDecompressionHeap = rg_alloc(0xD000, MEM_SLOW);
    
#if defined(VERSION_EU) || defined(VERSION_SH)
    gAudioHeap = rg_alloc(DOUBLE_SIZE_ON_64_BIT(0x31200) - 0x3800, MEM_SLOW);
#else
    gAudioHeap = rg_alloc(DOUBLE_SIZE_ON_64_BIT(0x31200), MEM_SLOW);
#endif

    gSaveBufferPtr = rg_alloc(sizeof(struct SaveBuffer), MEM_SLOW);
    gGfxPools = rg_alloc(sizeof(struct GfxPool) * GFX_NUM_POOLS, MEM_SLOW);

#ifdef VERSION_JP
    gAudioSPTaskYieldBuffer = rg_alloc(OS_YIELD_AUDIO_SIZE, MEM_SLOW);
#endif

#if !defined(F3DEX_GBI_SHARED) && !defined(VERSION_EU)
    gUnusedThread2Stack = rg_alloc(0x1400, MEM_SLOW);
#endif

    if (!gDecompressionHeap || !gAudioHeap || !gSaveBufferPtr || !gGfxPools
#ifdef VERSION_JP
        || !gAudioSPTaskYieldBuffer
#endif
#if !defined(F3DEX_GBI_SHARED) && !defined(VERSION_EU)
        || !gUnusedThread2Stack
#endif
    ) {
        RG_PANIC("Failed to allocate SM64 engine buffers!");
    }
}
