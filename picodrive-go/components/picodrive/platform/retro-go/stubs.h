#ifndef RETRO_GO_STUBS_H
#define RETRO_GO_STUBS_H

#include <stdint.h>
#include <stddef.h>

// Function declarations for linker satisfaction
uint32_t PicoRead8_32x(uint32_t a);
uint32_t PicoRead16_32x(uint32_t a);
void PicoWrite8_32x(uint32_t a, uint32_t d);
void PicoWrite16_32x(uint32_t a, uint32_t d);
extern int (*PicoScan32xBegin)(unsigned int num);
extern int (*PicoScan32xEnd)(unsigned int num);

#endif
