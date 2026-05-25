#ifndef EMU2413_H
#define EMU2413_H

#include <stdint.h>

typedef struct OPLL_ {
    uint32_t rate;
} OPLL;

OPLL *OPLL_new(uint32_t clk, uint32_t rate);
void OPLL_delete(OPLL *opll);
void OPLL_reset(OPLL *opll);
void OPLL_setChipType(OPLL *opll, uint32_t type);
void OPLL_setRate(OPLL *opll, uint32_t rate);
void OPLL_writeIO(OPLL *opll, uint32_t reg, uint32_t val);
int16_t OPLL_calc(OPLL *opll);

#endif
