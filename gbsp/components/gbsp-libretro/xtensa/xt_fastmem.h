/* xt_fastmem.h - reg[] slots shared by the C backend and xt_fastmem.S.
 * Plain numbers: the assembler includes this file too. xtensa_emit.h checks
 * that they follow the function table (XT_FN_SLOT + XT_FN_COUNT == 56). */
#ifndef XT_FASTMEM_H
#define XT_FASTMEM_H
#define XT_SLOT_A0       56   /* xt_enter's a0: callx0 into the fast loads overwrites a0 */
#define XT_SLOT_IWRAM    57   /* iwram + 0x8000 (the data half) */
#define XT_SLOT_EWRAM    58   /* ewram */
#define XT_SLOT_MAPRD    59   /* memory_map_read (game pak pages) */
#define XT_SLOT_CLD_U8   60   /* the C load helpers, for the fast loads' slow path */
#define XT_SLOT_CLD_S8   61
#define XT_SLOT_CLD_U16  62
#define XT_SLOT_CLD_S16  63
#define XT_SLOT_CLD_U32  64
#define XT_SLOT_SMCPEND  65   /* an STM word rewrote code: redirect at its last store */
#define XT_SLOT_EWTAG    66   /* ewram + 0x40000 (the code tags) */
#define XT_SLOT_VRAM     67   /* vram */
#define XT_SLOT_VDIRTY   68   /* gbsp_vram_dirty (u32[3], one bit a 1 KB page) */
#define XT_SLOT_VDANY    69   /* gbsp_vram_dirty_any (u8) */
#define XT_SLOT_CST_U8   70   /* the C store entries, for the fast stores' slow path */
#define XT_SLOT_CST_U16  71
#define XT_SLOT_CST_U32  72
#define XT_SLOT_CST_AU32 73
/* reg[] slot of the cycles a C helper hands back (XT_CYC_SLOT = REG_USERDEF + 0) */
#define XT_CYC_SLOT_ASM  32
#endif
