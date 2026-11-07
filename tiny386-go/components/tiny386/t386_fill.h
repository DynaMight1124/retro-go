#pragma once
#include <stdint.h>
/* Diagnostic reasons only; these do not change the fill eligibility rules.
 * WRITE_MODE is retained for archived traces; all four modes now qualify
 * when the remaining zero-input/range/register guards pass. */
enum {
    T386_FILL_INPUT=1u<<0,T386_FILL_RANGE=1u<<1,T386_FILL_BANK=1u<<2,
    T386_FILL_CHAIN4=1u<<3,T386_FILL_ODD_EVEN=1u<<4,T386_FILL_WRITE_MODE=1u<<5,
    T386_FILL_SET_RESET=1u<<6,T386_FILL_ROTATE=1u<<7,T386_FILL_BIT_MASK=1u<<8,
    T386_FILL_MAPPING=1u<<9,T386_FILL_PLANE_MASK=1u<<10,T386_FILL_VRAM=1u<<11,
    T386_FILL_PCI=1u<<12,T386_FILL_CALLBACK=1u<<13,T386_FILL_DECLINED=1u<<14,
    T386_FILL_TEXT_PLANE=1u<<15,T386_FILL_TEXT_CALLBACK=1u<<16
};
typedef struct {
    unsigned reason,addr,count,state_valid;
    int bank;
    uint8_t sr2,sr4,gr0,gr1,gr3,gr4,gr5,gr6,gr8;
} T386FillReject;
/* addr is local to VGA at its API; PC converts it to guest physical address.
 * state_valid distinguishes a VGA snapshot from a CPU/PC routing rejection. */
