#include "../pico_int.h"

unsigned int SekCycleCntS68k;
unsigned int SekCycleAimS68k;

// All Sub-CPU context and logic is now centrally managed in pico/sek.c 
// to ensure consistent FAME core integration.
// Macros in pico_int.h handle Interrupts, EndRun and CyclesDone.

/* context bridge */
extern M68K_CONTEXT PicoCpuFS68k;
