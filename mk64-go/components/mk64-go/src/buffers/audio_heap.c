#include <PR/ultratypes.h>
#include <macros.h>
#include <mk64.h>

#include "audio_heap.h"
#include "audio/data.h"

#ifdef RETRO_GO
__attribute__((section(".ext_ram.bss"), aligned(64)))
#endif
ALIGNED8 u8 gAudioHeap[DOUBLE_SIZE_ON_64_BIT(AUDIO_HEAP_SIZE)];
