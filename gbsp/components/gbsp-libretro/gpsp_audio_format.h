#pragma once
#include <stdint.h>

#define GPSP_AUDIO_SAMPLE_RATE 32768
/* ceil(samples per native 280896-cycle GBA frame at 16777216 Hz).
 * Also covers the ESP32/S3 60 FPS cadence. The old 547-frame cap could not
 * drain native P4 production (548.625 frames), causing accumulating backlog. */
#define GPSP_AUDIO_MAX_FRAMES ((uint32_t)((UINT64_C(32768) * 280896 + 16777216 - 1) / 16777216))
