#ifndef FRAMEBUFFERS_H
#define FRAMEBUFFERS_H

#include <PR/ultratypes.h>
#include "config.h"

// 0x70800 bytes refactored to pointers
extern u16 (*gFrameBuffersPtr)[SCREEN_WIDTH * SCREEN_HEIGHT];

#define gFrameBuffers gFrameBuffersPtr
#define gFrameBuffer0 gFrameBuffers[0]
#define gFrameBuffer1 gFrameBuffers[1]
#define gFrameBuffer2 gFrameBuffers[2]

void sm64_init_framebuffers(void);

#endif // FRAMEBUFFERS_H
