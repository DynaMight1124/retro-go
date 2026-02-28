/*
 * Cache operations
 *
 * Author: Gražvydas "notaz" Ignotas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif

#define CACHELINE_SZC #32

#if !defined(CONFIG_CPU_ARM926T) && !defined(CONFIG_CPU_ARM920T)
#error CPU not supported
#endif

.text
.align 2

.global warm_cop_clean_inval_d
.global warm_cop_clean_d

#ifdef CONFIG_CPU_ARM926T

warm_cop_clean_inval_d:
0:  mrc    p15, 0, r15, c7, c14, 3	@ test, clean and invalidate
    bne    0b
    bx     lr

warm_cop_clean_d:
0:  mrc    p15, 0, r15, c7, c10, 3	@ test and clean
    bne    0b
    bx     lr

#else

/* comes from Linux kernel code */
.macro warm_cop_wholecache_dop crm
    mov    r1,     #0x000000e0		@ 8 segments
1:  orr    r3, r1, #0xfc000000		@ 64 entries
2:  mcr    p15, 0, r3, c7, \crm, 2	@ D index op
    subs   r3, r3, #1<<26
    bcs    2b				@ entries 63 to 0
    subs   r1, r1, #1<<5
    bcs    1b				@ segments 7 to 0
    bx     lr
.endm

warm_cop_clean_inval_d:
    warm_cop_wholecache_dop c14

warm_cop_clean_d:
    warm_cop_wholecache_dop c10

#endif

.global warm_cop_inval_d
warm_cop_inval_d:
    mov    r0, #0
    mcr    p15, 0, r0, c7, c6, 0
    bx     lr


.global warm_cop_inval_i
warm_cop_inval_i:
    mov    r0, #0
    mcr    p15, 0, r0, c7, c5, 0
    bx     lr


.global warm_cop_drain_wb
warm_cop_drain_wb:
    mov    r0, #0
    mcr    p15, 0, r0, c7, c10, 4
    bx     lr


#define R_CLEAN_INVAL_D \
    mcr    p15, 0, r0, c7, c14, 1

#define R_CLEAN_D \
    mcr    p15, 0, r0, c7, c10, 1

#define R_INVAL_D \
    mcr    p15, 0, r0, c7, c6, 1

#define R_INVAL_I \
    mcr    p15, 0, r0, c7, c5, 1

#define WARM_COP_MK_RANGE_FUNC(name,f1,f2)	\
.global name					;\
name:						;\
    bic    r0, r0, CACHELINE_SZC-1		;\
0:  f1						;\
    f2						;\
    add    r0, r0, CACHELINE_SZC		;\
    subs   r1, r1, CACHELINE_SZC		;\
    bgt    0b					;\
    bx     lr

WARM_COP_MK_RANGE_FUNC(warm_cop_r_clean_d_inval_di, R_CLEAN_INVAL_D, R_INVAL_I)
WARM_COP_MK_RANGE_FUNC(warm_cop_r_clean_d_inval_d,  R_CLEAN_INVAL_D, )
WARM_COP_MK_RANGE_FUNC(warm_cop_r_clean_d_inval_i,  R_CLEAN_D,       R_INVAL_I)
WARM_COP_MK_RANGE_FUNC(warm_cop_r_clean_d,          R_CLEAN_D, )
WARM_COP_MK_RANGE_FUNC(warm_cop_r_inval_di,         R_INVAL_D, R_INVAL_I)
WARM_COP_MK_RANGE_FUNC(warm_cop_r_inval_d,          R_INVAL_D, )
WARM_COP_MK_RANGE_FUNC(warm_cop_r_inval_i,          R_INVAL_I, )


.global warm_drain_wb_inval_tlb
warm_drain_wb_inval_tlb:
    mov    r0, #0
    mcr    p15, 0, r0, c7, c10, 4
    mcr    p15, 0, r0, c8, c7, 0
    bx     lr


@ vim:filetype=armasm

