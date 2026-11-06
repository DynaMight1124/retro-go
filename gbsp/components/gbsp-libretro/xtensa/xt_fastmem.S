/* xt_fastmem.S - the loads of translated code, in internal RAM, call0 ABI.
 *
 * 2026-10-13: translated code called its C load helpers with callx8 (a
 * window rotation, the helper's prologue and region switch, retw). Inlining
 * the access in the block lost (dynarec branch gba-mem: the code grew from
 * 9 to 45-120 bytes a load and translated code runs from PSRAM through the
 * 32 KB instruction cache). Here the block keeps its call (l32i + callx0, the
 * same 6 bytes as l32i + callx8), and the callee is this short routine:
 * IWRAM, EWRAM and game pak reads done here, anything else (I/O, VRAM,
 * palette, OAM, BIOS, open bus, misaligned, a game pak page not loaded) goes
 * to the C helper with callx8 from here, which returns the same value.
 *
 * In: a10 = address, a11 = PC (for the helper), a0 = return address (callx0;
 * xt_enter keeps its own a0 in reg[XT_SLOT_A0]), a2 = &reg[0].
 * Out: a10 = the value. Scratch: a8, a9, a13 (a call clobbers them anyway).
 */
#include "xt_fastmem.h"

    .section .iram1.xtfastmem, "ax", @progbits

/* name, alignment bits (0/1/2), load, sign-extend a byte, C helper slot */
.macro XT_FASTLD name, al, ld, sx, cslot
    .align 4
    .global \name
    .type \name, @function
\name:
    extui   a9, a10, 24, 8              /* region */
    bnei    a9, 3, .L\name\()_ew
    .if \al
    extui   a13, a10, 0, \al
    bnez    a13, .L\name\()_slow
    .endif
    extui   a9, a10, 0, 15              /* IWRAM: a & 0x7FFF */
    l32i    a13, a2, XT_SLOT_IWRAM * 4
    add     a9, a9, a13
    \ld     a10, a9, 0
    .if \sx
    sext    a10, a10, 7
    .endif
    ret
.L\name\()_ew:
    bnei    a9, 2, .L\name\()_rom
    .if \al
    extui   a13, a10, 0, \al
    bnez    a13, .L\name\()_slow
    .endif
    slli    a9, a10, 14                 /* EWRAM: a & 0x3FFFF */
    srli    a9, a9, 14
    l32i    a13, a2, XT_SLOT_EWRAM * 4
    add     a9, a9, a13
    \ld     a10, a9, 0
    .if \sx
    sext    a10, a10, 7
    .endif
    ret
.L\name\()_rom:
    addi    a13, a9, -8                 /* game pak: regions 8..12 */
    bgeui   a13, 5, .L\name\()_slow
    .if \al
    extui   a13, a10, 0, \al
    bnez    a13, .L\name\()_slow
    .endif
    extui   a9, a10, 15, 13             /* a >> 15 (the address is < 0x10000000 here) */
    l32i    a13, a2, XT_SLOT_MAPRD * 4
    addx4   a9, a9, a13
    l32i    a9, a9, 0                   /* memory_map_read[a >> 15] */
    beqz    a9, .L\name\()_slow
    extui   a13, a10, 0, 15
    add     a9, a9, a13
    \ld     a10, a9, 0
    .if \sx
    sext    a10, a10, 7
    .endif
    ret
.L\name\()_slow:
    l32i    a8, a2, \cslot * 4
    callx8  a8                          /* the C helper: a10 = address, a11 = PC */
    ret
    .size \name, . - \name
.endm

    XT_FASTLD xt_fast_ld_u8,  0, l8ui,  0, XT_SLOT_CLD_U8
    XT_FASTLD xt_fast_ld_s8,  0, l8ui,  1, XT_SLOT_CLD_S8
    XT_FASTLD xt_fast_ld_u16, 1, l16ui, 0, XT_SLOT_CLD_U16
    XT_FASTLD xt_fast_ld_s16, 1, l16si, 0, XT_SLOT_CLD_S16
    XT_FASTLD xt_fast_ld_u32, 2, l32i,  0, XT_SLOT_CLD_U32

/* The stores (2026-10-14): translated code's IWRAM, EWRAM and VRAM writes,
 * same call0 scheme as the loads. A RAM word that holds translated code
 * (its tag is not 0), a pending STM redirect (reg[XT_SLOT_SMCPEND]) and any
 * other region go to the C entry with callx8 (it
 * stores the PC and the cycles in reg[] and may redirect). The fast path
 * skips them: they are read only on a redirect.
 *
 * In: a10 = address, a11 = value, a12 = PC, a13 = cycles, a0 = return
 * address, a2 = &reg[0]. Returns only to go on (a10 undefined); a redirect
 * jumps to its target from here (xt_fast_st_redirect).
 * Scratch: a8, a9, a14, a15.
 */

/* name, size bits (0/1/2), store, tag load, check the pending STM redirect,
   VRAM fast path, C entry slot */
.macro XT_FASTST name, sz, st, tl, pend, vr, cslot
    .align 4
    .global \name
    .type \name, @function
\name:
    .if \pend
    l32i    a8, a2, XT_SLOT_SMCPEND * 4
    bnez    a8, .L\name\()_slow
    .endif
    extui   a9, a10, 24, 8              /* region */
    bnei    a9, 3, .L\name\()_ew
    .if \sz
    extui   a14, a10, \sz, 15 - \sz     /* IWRAM: a & 0x7FFF, aligned down */
    slli    a14, a14, \sz
    .else
    extui   a14, a10, 0, 15
    .endif
    l32i    a15, a2, XT_SLOT_IWRAM * 4
    add     a14, a14, a15
    addmi   a8, a14, -32768             /* its tag: iwram[a] */
    \tl     a8, a8, 0
    bnez    a8, .L\name\()_slow
    \st     a11, a14, 0
    ret
.L\name\()_ew:
    bnei    a9, 2, .L\name\()_vr
    slli    a14, a10, 14                /* EWRAM: a & 0x3FFFF, aligned down */
    srli    a14, a14, 14 + \sz
    .if \sz
    slli    a14, a14, \sz
    .endif
    l32i    a15, a2, XT_SLOT_EWTAG * 4
    add     a8, a14, a15                /* its tag: ewram[a + 0x40000] */
    \tl     a8, a8, 0
    bnez    a8, .L\name\()_slow
    l32i    a15, a2, XT_SLOT_EWRAM * 4
    add     a14, a14, a15
    \st     a11, a14, 0
    ret
.L\name\()_vr:
    .if \vr
    bnei    a9, 6, .L\name\()_slow
    .if \sz
    extui   a14, a10, \sz, 17 - \sz     /* VRAM: a & 0x1FFFF, aligned down */
    slli    a14, a14, \sz
    .else
    extui   a14, a10, 1, 16             /* VRAM, 8-bit: the halfword holding it, */
    slli    a14, a14, 1                 /* the byte written to both halves (16-bit bus) */
    extui   a11, a11, 0, 8
    slli    a15, a11, 8
    or      a11, a11, a15
    .endif
    extui   a15, a14, 15, 2
    bnei    a15, 3, .L\name\()_vf
    addmi   a14, a14, -32768            /* 0x18000.. mirrors 0x10000.. */
.L\name\()_vf:
    extui   a15, a14, 15, 2             /* gbsp_vram_dirty[a >> 15] |= 1 << ((a >> 10) & 31) */
    l32i    a8, a2, XT_SLOT_VDIRTY * 4
    addx4   a8, a15, a8
    l32i    a9, a8, 0
    extui   a15, a14, 10, 5
    ssl     a15
    movi    a15, 1
    sll     a15, a15
    or      a9, a9, a15
    s32i    a9, a8, 0
    l32i    a8, a2, XT_SLOT_VDANY * 4
    movi    a9, 1
    s8i     a9, a8, 0                   /* gbsp_vram_dirty_any = 1 */
    l32i    a8, a2, XT_SLOT_VRAM * 4
    add     a14, a14, a8
    .if \sz
    \st     a11, a14, 0
    .else
    s16i    a11, a14, 0
    .endif
    ret
    .endif
.L\name\()_slow:
    l32i    a8, a2, \cslot * 4
    callx8  a8                          /* the C entry: a10..a13 as received */
    bnez    a10, xt_fast_st_redirect
    ret
    .size \name, . - \name
.endm

/* A redirect is taken here, not at the call site (2026-10-14): the block's
 * store is then l32i + callx0 with no "beqz a10; j cold" after it (6 bytes
 * less a store, and the cold stub gone), and the fast paths return without
 * a10 = 0. Same as the cold stub: the cycles the C entry left, then the
 * target (translated code; a0 is restored from reg[XT_SLOT_A0] by the exit). */
    .align 4
    .global xt_fast_st_redirect
    .type xt_fast_st_redirect, @function
xt_fast_st_redirect:
    l32i    a3, a2, XT_CYC_SLOT_ASM * 4
    jx      a10
    .size xt_fast_st_redirect, . - xt_fast_st_redirect

    XT_FASTST xt_fast_st_u8,   0, s8i,  l8ui,  1, 1, XT_SLOT_CST_U8
    XT_FASTST xt_fast_st_u16,  1, s16i, l16ui, 1, 1, XT_SLOT_CST_U16
    XT_FASTST xt_fast_st_u32,  2, s32i, l32i,  1, 1, XT_SLOT_CST_U32
    XT_FASTST xt_fast_st_au32, 2, s32i, l32i,  0, 1, XT_SLOT_CST_AU32
