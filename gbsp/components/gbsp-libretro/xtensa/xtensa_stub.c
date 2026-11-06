/* gameplaySP - Xtensa backend, runtime (esp32-emu-turbo)
 *
 * What x86/x86_stub.S does in assembly, in C: the ARM state, the memory
 * handlers called by translated code (same regions, masks, mirrors and SMC
 * checks as the x86 ones), the update and indirect branch trampolines, CPSR
 * and SPSR writes, SWI, and the entry (xt_enter, generated at init) and exit
 * (retw) of translated code. A handler that must leave the current block
 * returns the instruction address to jump to (0: go on), with the cycles
 * left in reg[XT_CYC_SLOT]. Flags live in reg[REG_N_FLAG..REG_V_FLAG]. */

#include "gpsp_log.h"
#include <string.h>
#include <stdio.h>
#include "common.h"
#include "gba_memory.h"
#include "cpu.h"
#include "main.h"
#include "cheats.h"
#include "xjit_emit.h"
#include "xtensa/xtensa_emit.h"

cpu_alert_type function_cc write_io_register8(u32 address, u32 value);
cpu_alert_type function_cc write_io_register16(u32 address, u32 value);
cpu_alert_type function_cc write_io_register32(u32 address, u32 value);
extern u32 rom_cache_watermark;
#define XT_INITIAL_ROM_WATERMARK 16   /* cpu_threaded.c INITIAL_ROM_WATERMARK */

#if defined(ESP_PLATFORM)
#include "esp_cache.h"
#include "esp_cpu.h"
#include "esp_attr.h"
#define XT_EXT_BSS EXT_RAM_BSS_ATTR
#else
#define XT_EXT_BSS
#endif

/* ---- ARM state (cpu.cpp defines these only without HAVE_DYNAREC) ---------- */
u32 reg[REG_MAX];
u32 spsr[6];
u32 reg_mode[7][7];
u16 oam_ram[512];
u16 palette_ram[512];
u16 palette_ram_converted[512];
#ifndef RETRO_GO
XT_EXT_BSS u8 ewram[(1024 * 256) << SMC_DETECTION];
XT_EXT_BSS u8 vram[1024 * 96];
#endif
u8 iwram[(1024 * 32) << SMC_DETECTION];   /* internal RAM: the game's stack and hot code (PSRAM: 3-4% slower) */
u8 *memory_map_read[8 * 1024];
u16 io_registers[512];

xj_emit_t xt_es;
int xt_fastmem = 1;                  /* see xtensa_emit.h; gba_nofast on the card turns it off */
int xt_cycfix = 1;                   /* see xt_store_pc; gba_nocycfix on the card turns it off. With it a
                                        per-block SMC run is exact against a full-flush one (Doom, Crash,
                                        NFS state hashes, board 2026-10-14) */
int xt_redirtest = 0;                /* gba_redirtest: a needless redirect every 1000th store (timing-sensitivity probe) */
_Static_assert(XT_CYC_SLOT == XT_CYC_SLOT_ASM, "xt_fastmem.S takes store redirects with this slot");
int xt_faststore = 1;                /* xt_fastmem.S's stores; gba_nofastst on the card turns them off */
xt_cold_t xt_cold[XT_COLD_MAX + 1];   /* the block being translated (xt_emit_cold) */
int xt_cold_n;

/* the out-of-line emitters behind XT() (xtensa_emit.h) */
u8 *xto_retw(u8 *p)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_retw(&e);
  if (!xj_ok(&e)) xt_emit_error("retw", 0);
  return p + e.pos;
}

u8 *xto_nop(u8 *p)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_nop(&e);                 /* 3 bytes: xt_smc_invalidate overwrites it with a j */
  if (!xj_ok(&e) || e.pos != 3) xt_emit_error("nop", e.pos);
  return p + e.pos;
}

u8 *xto_callx0(u8 *p, int a0)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_callx0(&e, a0);
  if (!xj_ok(&e)) xt_emit_error("callx0", 0);
  return p + e.pos;
}

u8 *xto_callx8(u8 *p, int a0)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_callx8(&e, a0);
  if (!xj_ok(&e)) xt_emit_error("callx8", 0);
  return p + e.pos;
}

u8 *xto_jx(u8 *p, int a0)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_jx(&e, a0);
  if (!xj_ok(&e)) xt_emit_error("jx", 0);
  return p + e.pos;
}

u8 *xto_ssl(u8 *p, int a0)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_ssl(&e, a0);
  if (!xj_ok(&e)) xt_emit_error("ssl", 0);
  return p + e.pos;
}

u8 *xto_ssr(u8 *p, int a0)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_ssr(&e, a0);
  if (!xj_ok(&e)) xt_emit_error("ssr", 0);
  return p + e.pos;
}

u8 *xto_ssai(u8 *p, int a0)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_ssai(&e, a0);
  if (!xj_ok(&e)) xt_emit_error("ssai", 0);
  return p + e.pos;
}

u8 *xto_j(u8 *p, int a0)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_j(&e, a0);
  if (!xj_ok(&e)) xt_emit_error("j", 0);
  return p + e.pos;
}

u8 *xto_mov(u8 *p, int a0, int a1)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  if (a0 == a1)
    return p;
  xj_mov_n(&e, a0, a1);   /* density forms: the code is fetched from PSRAM */
  if (!xj_ok(&e)) xt_emit_error("mov", 0);
  return p + e.pos;
}

u8 *xto_movi(u8 *p, int a0, int a1)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  if (a1 >= -32 && a1 <= 95)
    xj_movi_n(&e, a0, a1);
  else
    xj_movi(&e, a0, a1);
  if (!xj_ok(&e)) xt_emit_error("movi", 0);
  return p + e.pos;
}

u8 *xto_nsau(u8 *p, int a0, int a1)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_nsau(&e, a0, a1);
  if (!xj_ok(&e)) xt_emit_error("nsau", 0);
  return p + e.pos;
}

u8 *xto_beqz(u8 *p, int a0, int a1)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_beqz(&e, a0, a1);
  if (!xj_ok(&e)) xt_emit_error("beqz", 0);
  return p + e.pos;
}

u8 *xto_bnez(u8 *p, int a0, int a1)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_bnez(&e, a0, a1);
  if (!xj_ok(&e)) xt_emit_error("bnez", 0);
  return p + e.pos;
}

u8 *xto_bgez(u8 *p, int a0, int a1)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_bgez(&e, a0, a1);
  if (!xj_ok(&e)) xt_emit_error("bgez", 0);
  return p + e.pos;
}

u8 *xto_l32r(u8 *p, int a0, int a1)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_l32r(&e, a0, a1);
  if (!xj_ok(&e)) xt_emit_error("l32r", 0);
  return p + e.pos;
}

u8 *xto_entry(u8 *p, int a0, int a1)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_entry(&e, a0, a1);
  if (!xj_ok(&e)) xt_emit_error("entry", 0);
  return p + e.pos;
}

u8 *xto_sll(u8 *p, int a0, int a1)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_sll(&e, a0, a1);
  if (!xj_ok(&e)) xt_emit_error("sll", 0);
  return p + e.pos;
}

u8 *xto_srl(u8 *p, int a0, int a1)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_srl(&e, a0, a1);
  if (!xj_ok(&e)) xt_emit_error("srl", 0);
  return p + e.pos;
}

u8 *xto_sra(u8 *p, int a0, int a1)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_sra(&e, a0, a1);
  if (!xj_ok(&e)) xt_emit_error("sra", 0);
  return p + e.pos;
}

u8 *xto_add(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_add_n(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("add", 0);
  return p + e.pos;
}

u8 *xto_addi(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  if (a2 == 0)
    return xto_mov(p, a0, a1);
  if (a2 == -1 || (a2 >= 1 && a2 <= 15))
    xj_addi_n(&e, a0, a1, a2);
  else
    xj_addi(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("addi", 0);
  return p + e.pos;
}

u8 *xto_addmi(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_addmi(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("addmi", 0);
  return p + e.pos;
}

u8 *xto_and(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_and(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("and", 0);
  return p + e.pos;
}

u8 *xto_l32i(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  if (a2 >= 0 && a2 <= 60 && !(a2 & 3))
    xj_l32i_n(&e, a0, a1, a2);
  else
    xj_l32i(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("l32i", 0);
  return p + e.pos;
}

u8 *xto_s32i(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  if (a2 >= 0 && a2 <= 60 && !(a2 & 3))
    xj_s32i_n(&e, a0, a1, a2);
  else
    xj_s32i(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("s32i", 0);
  return p + e.pos;
}

u8 *xto_mull(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_mull(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("mull", 0);
  return p + e.pos;
}

u8 *xto_mulsh(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_mulsh(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("mulsh", 0);
  return p + e.pos;
}

u8 *xto_muluh(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_muluh(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("muluh", 0);
  return p + e.pos;
}

u8 *xto_or(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_or(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("or", 0);
  return p + e.pos;
}

u8 *xto_saltu(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_saltu(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("saltu", 0);
  return p + e.pos;
}

u8 *xto_slli(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_slli(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("slli", 0);
  return p + e.pos;
}

u8 *xto_srai(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_srai(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("srai", 0);
  return p + e.pos;
}

u8 *xto_src(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_src(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("src", 0);
  return p + e.pos;
}

u8 *xto_srli(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_srli(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("srli", 0);
  return p + e.pos;
}

u8 *xto_sub(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_sub(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("sub", 0);
  return p + e.pos;
}

u8 *xto_xor(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_xor(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("xor", 0);
  return p + e.pos;
}

u8 *xto_bltui(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_bltui(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("bltui", 0);
  return p + e.pos;
}

u8 *xto_bgeui(u8 *p, int a0, int a1, int a2)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_bgeui(&e, a0, a1, a2);
  if (!xj_ok(&e)) xt_emit_error("bgeui", 0);
  return p + e.pos;
}

u8 *xto_extui(u8 *p, int a0, int a1, int a2, int a3)
{
  xj_emit_t e;
  xj_init(&e, p, 16);
  xj_extui(&e, a0, a1, a2, a3);
  if (!xj_ok(&e)) xt_emit_error("extui", 0);
  return p + e.pos;
}
u32 xt_exec_delta;
u8 *xt_exit_stub;
static u32 (*xt_enter)(s32 cycles, u32 target, u32 *regs);

void xt_emit_error(const char *what, int line)
{
  RG_LOGE("dynarec: Xtensa emitter: %s (line %d)", what, line);
}

#define XT_EXEC(p) ((u32)(uintptr_t)(p) + xt_exec_delta)

#ifdef RETRO_GO
/* video.cpp's line renderer: VRAM changes wait for the lines already emulated;
   palette changes are picked up per line (gbsp_pal_dirty) */
extern volatile u32 gbsp_rq, gbsp_rd;
extern u8 gbsp_pal_dirty;
void gbsp_render_sync(void);
/* video.cpp keeps a VRAM copy for the renderer: mark the written 1 KB page
   (a is the folded VRAM offset) */
extern u32 gbsp_vram_dirty[3];
extern u8 gbsp_vram_dirty_any;
#define xt_vram_write() (gbsp_vram_dirty[a >> 15] |= 1u << ((a >> 10) & 31), gbsp_vram_dirty_any = 1)
#define xt_pal_write()  (gbsp_pal_dirty = 1)
#else
#define xt_vram_write()
#define xt_pal_write()
#endif

/* ---- flags -------------------------------------------------------------- */
static void collapse_flags(void)
{
  reg[REG_CPSR] = (reg[REG_N_FLAG] << 31) | (reg[REG_Z_FLAG] << 30) | (reg[REG_C_FLAG] << 29) |
                  (reg[REG_V_FLAG] << 28) | (reg[REG_CPSR] & 0xFF);
}
static void extract_flags_regs(void)
{
  reg[REG_N_FLAG] = reg[REG_CPSR] >> 31;
  reg[REG_Z_FLAG] = (reg[REG_CPSR] >> 30) & 1;
  reg[REG_C_FLAG] = (reg[REG_CPSR] >> 29) & 1;
  reg[REG_V_FLAG] = (reg[REG_CPSR] >> 28) & 1;
}

/* ---- jumping back into translated code ---------------------------------- */
static u32 lookup_pc(void)
{
  u8 *p = (reg[REG_CPSR] & 0x20) ? block_lookup_address_thumb(reg[REG_PC])
                                 : block_lookup_address_arm(reg[REG_PC]);
  return XT_EXEC(p);
}
static u32 xt_exit(void) { return XT_EXEC(xt_exit_stub); }

/* x86_update_gba */
XT_HOT static u32 xt_update_gba(u32 pc)
{
  u32 r;
  reg[REG_PC] = pc;
  collapse_flags();
  r = update_gba((s32)reg[XT_CYC_SLOT]);
  if (r & 0x80000000)
    return xt_exit();
  reg[XT_CYC_SLOT] = r & 0x7FFF;
  if (r & 0x40000000)
    return lookup_pc();
  return 0;
}

XT_HOT static u32 xt_indirect_arm(u32 address)   { return XT_EXEC(block_lookup_address_arm(address)); }
XT_HOT static u32 xt_indirect_thumb(u32 address) { return XT_EXEC(block_lookup_address_thumb(address)); }
XT_HOT static u32 xt_indirect_dual(u32 address)  { return XT_EXEC(block_lookup_address_dual(address)); }

/* ---- memory: loads (x86 load_stubs) -------------------------------------- */
#define rd8(p)  (*(const u8 *)(p))
/* the masks keep every access aligned (unaligned ones take the slow path):
   plain loads, no memcpy (ESP-IDF builds with -fno-builtin-memcpy) */
#define rd16(p) (*(const u16 *)(p))
#define rd32(p) (*(const u32 *)(p))
#define wr16(p, v) (*(u16 *)(p) = (v))
#define wr32(p, v) (*(u32 *)(p) = (v))

/* the region, or 16+ when unaligned for this width (-> the C slow path) */
#define XT_LOAD(name, type, width, almask, slowfn)                           \
XT_HOT u32 xt_load_##name(u32 address, u32 pc)                               \
{                                                                            \
  u32 r = address >> 24;                                                     \
  if (r > 15 || (address & (almask)))                                        \
    goto slow;                                                               \
  switch (r)                                                                 \
  {                                                                          \
  case 2: return (type)rd##width(&ewram[address & (0x3FFFF & ~(almask))]);   \
  case 3: return (type)rd##width(&iwram[0x8000 + (address & (0x7FFF & ~(almask)))]); \
  case 4: return (type)rd##width((u8 *)io_registers + (address & (0x3FF & ~(almask)))); \
  case 5: return (type)rd##width((u8 *)palette_ram + (address & (0x3FF & ~(almask)))); \
  case 6:                                                                    \
  {                                                                          \
    u32 a = address & (0x1FFFF & ~(almask));                                 \
    if (a >= 0x18000) a -= 0x8000;                                           \
    return (type)rd##width(&vram[a]);                                        \
  }                                                                          \
  case 7: return (type)rd##width((u8 *)oam_ram + (address & (0x3FF & ~(almask)))); \
  case 8: case 9: case 10: case 11: case 12:                                 \
  {                                                                          \
    u8 *map = memory_map_read[address >> 15];                                \
    if (!map) goto slow;                                                     \
    return (type)rd##width(map + (address & 0x7FFF));                        \
  }                                                                          \
  }                                                                          \
slow:                                                                        \
  reg[REG_PC] = pc;                                                          \
  return slowfn(address);                                                    \
}
XT_LOAD(u32, u32, 32, 3, read_memory32)
XT_LOAD(u16, u16, 16, 1, read_memory16)
XT_LOAD(s16, s16, 16, 1, read_memory16s)
XT_LOAD(u8, u8, 8, 0, read_memory8)
XT_LOAD(s8, s8, 8, 0, read_memory8s)

/* ---- memory: stores (x86 write_stubs) ------------------------------------ */
/* write_epilogue: act on the alerts of an I/O write, then go on at reg[PC] */
XT_HOT static u32 xt_write_epilogue(u32 alert)
{
  collapse_flags();
  if (alert & CPU_ALERT_SMC)
  {
#ifdef GBAPROF
    { extern u32 xt_prof_smc_alert; xt_prof_smc_alert++; }
#endif
    flush_translation_cache_ram();
  }
  if (alert & CPU_ALERT_IRQ)
    check_and_raise_interrupts();
  if (alert & CPU_ALERT_HALT)
  {
    u32 r = update_gba((s32)reg[XT_CYC_SLOT]);
    if (r & 0x80000000)
      return xt_exit();
    reg[XT_CYC_SLOT] = r & 0x7FFF;
  }
  return lookup_pc();
}
/* GBAPROF: where the game rewrites translated code - CPU stores by IWRAM
   512-byte bucket (64) and EWRAM 4 KB bucket (64), I/O alerts (DMA) apart */
/* 2026-10-13: a store into translated code that leaves the value as it was
   does not flush: the translation still matches the code (exact). A game
   that copies the same routines into RAM again would otherwise retranslate
   all its RAM code each time. SMC_SAME counts those stores (GBAPROF). */
#ifdef GBAPROF
u32 xt_prof_smc_same;
#define SMC_SAME(tag) ((tag) ? (void)xt_prof_smc_same++ : (void)0)
#else
#define SMC_SAME(tag) ((void)0)
#endif
#ifdef GBAPROF
u32 xt_prof_smc_cpu, xt_prof_smc_alert, xt_prof_smc_iw[64], xt_prof_smc_ew[64], xt_prof_ram_flush;
#define SMC_NOTE(a) (xt_prof_smc_cpu++, ((a) >> 24) == 3 ? xt_prof_smc_iw[((a) >> 9) & 63]++ : xt_prof_smc_ew[((a) >> 12) & 63]++)
#else
#define SMC_NOTE(a) ((void)0)
#endif
/* A store of an STM/PUSH other than the last (STORE_ALIGNED_U32) that
   changes translated code: its blocks are invalidated at once, the redirect
   waits for the instruction's last store (an STM is not left half done).
   Only the last word used to be checked, so code rewritten by the other words
   of an STM went on running (hidden by the full flush when the last word
   hit code too; Doom's per-block A/B showed it, 2026-10-14). */
#define xt_smc_pending reg[XT_SLOT_SMCPEND]   /* read by xt_fastmem.S's stores too */
XT_HOT static void xt_smc_note(u32 address)
{
  extern int xt_smc_invalidate(u32 address);
  SMC_NOTE(address);
  if (xt_smc_invalidate(address))
    xt_smc_pending = 1;
}

XT_HOT static u32 xt_smc_write(u32 address)
{
  extern int xt_smc_invalidate(u32 address);
  SMC_NOTE(address);
  if (!xt_smc_invalidate(address) && !xt_smc_pending)   /* only the blocks that hold the address */
    return 0;                        /* none live: the code goes on */
  xt_smc_pending = 0;
  return lookup_pc();
}

/* palette: the x86 stub's conversion (bit 15 lands in green's low bit) */
XT_HOT static void xt_store_palette16b(u32 a, u32 v)
{
  u32 c;
  v &= 0xFFFF;
  ((u8 *)palette_ram)[a] = v; ((u8 *)palette_ram)[a + 1] = v >> 8;   /* may be odd */
  c = ((v << 11) & 0xF800) | ((v & 0x03E0) << 1) | ((v >> 10) & 0x3F);
  ((u8 *)palette_ram_converted)[a] = c; ((u8 *)palette_ram_converted)[a + 1] = c >> 8;
  xt_pal_write();
}
XT_HOT static void xt_store_vram16(u32 a, u32 v)
{
  if (a >= 0x18000) a -= 0x8000;
  xt_vram_write();
  wr16(&vram[a], v);
}

XT_HOT static u32 xt_store_u32_body(u32 address, u32 value)
{
  u32 a;
  switch (address >> 24)
  {
  case 2:
    a = address & 0x3FFFC;
    if (rd32(&ewram[a + 0x40000]) && rd32(&ewram[a]) != value)
      return wr32(&ewram[a], value), xt_smc_write(address);
    SMC_SAME(rd32(&ewram[a + 0x40000]));
    wr32(&ewram[a], value);
    return 0;
  case 3:
    a = address & 0x7FFC;
    if (rd32(&iwram[a]) && rd32(&iwram[0x8000 + a]) != value)
      return wr32(&iwram[0x8000 + a], value), xt_smc_write(address);
    SMC_SAME(rd32(&iwram[a]));
    wr32(&iwram[0x8000 + a], value);
    return 0;
  case 4:
  {
    cpu_alert_type al = write_io_register32(address & 0x3FC, value);
    return al ? xt_write_epilogue(al) : 0;
  }
  case 5:
    a = address & 0x3FF;
    xt_store_palette16b(a, value);
    xt_store_palette16b(a + 2, value >> 16);
    return 0;
  case 6:
    a = address & 0x1FFFC;
    if (a >= 0x18000) a -= 0x8000;
    xt_vram_write();
    wr32(&vram[a], value);
    return 0;
  case 7:
    reg[OAM_UPDATED] = 1;
    wr32((u8 *)oam_ram + (address & 0x3FC), value);
    return 0;
  }
  return 0;   /* BIOS, gamepak, EEPROM/backup (32-bit): ignored */
}

XT_HOT static u32 xt_store_aligned_u32_body(u32 address, u32 value)
{
  u32 a;
  switch (address >> 24)
  {
  case 2:
    a = address & 0x3FFFC;
    if (rd32(&ewram[a + 0x40000]) && rd32(&ewram[a]) != value)
      wr32(&ewram[a], value), xt_smc_note(address & ~3u);
    else
      wr32(&ewram[a], value);
    return 0;
  case 3:
    a = address & 0x7FFC;
    if (rd32(&iwram[a]) && rd32(&iwram[0x8000 + a]) != value)
      wr32(&iwram[0x8000 + a], value), xt_smc_note(address & ~3u);
    else
      wr32(&iwram[0x8000 + a], value);
    return 0;
  case 4: case 5: case 6: case 7:
    return xt_store_u32_body(address, value);
  }
  return 0;
}

XT_HOT static u32 xt_store_u16_body(u32 address, u32 value)
{
  u32 a;
  switch (address >> 24)
  {
  case 2:
    a = address & 0x3FFFE;
    if (rd16(&ewram[a + 0x40000]) && rd16(&ewram[a]) != (value & 0xFFFF))
      return wr16(&ewram[a], value), xt_smc_write(address);
    SMC_SAME(rd16(&ewram[a + 0x40000]));
    wr16(&ewram[a], value);
    return 0;
  case 3:
    a = address & 0x7FFE;
    if (rd16(&iwram[a]) && rd16(&iwram[0x8000 + a]) != (value & 0xFFFF))
      return wr16(&iwram[0x8000 + a], value), xt_smc_write(address);
    SMC_SAME(rd16(&iwram[a]));
    wr16(&iwram[0x8000 + a], value);
    return 0;
  case 4:
  {
    cpu_alert_type al = write_io_register16(address & 0x3FE, value);
    return al ? xt_write_epilogue(al) : 0;
  }
  case 5: xt_store_palette16b(address & 0x3FF, value); return 0;
  case 6: xt_store_vram16(address & 0x1FFFE, value); return 0;
  case 7:
    reg[OAM_UPDATED] = 1;
    wr16((u8 *)oam_ram + (address & 0x3FE), value);
    return 0;
  case 8: write_gpio(address & 0xFF, value & 0xFFFF); return 0;
  case 13: write_eeprom(address, value); return 0;
  }
  return 0;
}

XT_HOT static u32 xt_store_u8_body(u32 address, u32 value)
{
  u32 a, d = (value & 0xFF) | ((value & 0xFF) << 8);   /* 16-bit bus: the byte twice */
  switch (address >> 24)
  {
  case 2:
    a = address & 0x3FFFF;
    if (ewram[a + 0x40000] && ewram[a] != (u8)value)
      return ewram[a] = value, xt_smc_write(address);
    SMC_SAME(ewram[a + 0x40000]);
    ewram[a] = value;
    return 0;
  case 3:
    a = address & 0x7FFF;
    if (iwram[a] && iwram[0x8000 + a] != (u8)value)
      return iwram[0x8000 + a] = value, xt_smc_write(address);
    SMC_SAME(iwram[a]);
    iwram[0x8000 + a] = value;
    return 0;
  case 4:
  {
    cpu_alert_type al = write_io_register8(address & 0x3FF, value);
    return al ? xt_write_epilogue(al) : 0;
  }
  case 5: xt_store_palette16b(address & 0x3FE, d); return 0;
  case 6: xt_store_vram16(address & 0x1FFFE, d); return 0;
  case 7:
    reg[OAM_UPDATED] = 1;
    wr16((u8 *)oam_ram + (address & 0x3FE), d);
    return 0;
  case 14: write_backup(address & 0xFFFF, value & 0xFF); return 0;
  }
  return 0;
}

/* the entry points the translated code calls: the PC (0: none) and the cycles
   left come as arguments and land in reg[] here, as the x86 code stored them
   before the call (xtensa_emit_ops.h, xt_store_pc) */
#define XT_STORE_ENTRY(name)                                                  \
XT_HOT static u32 xt_store_##name(u32 address, u32 value, u32 pc, u32 cycles) \
{                                                                            \
  u32 r_;                                                                    \
  if (pc)                                                                    \
    reg[REG_PC] = pc;                                                        \
  reg[XT_CYC_SLOT] = cycles;                                                 \
  r_ = xt_store_##name##_body(address, value);                               \
  if (xt_redirtest && !r_ && pc && !xt_smc_pending)                          \
  {                                                                          \
    static u32 n_;                                                           \
    if (++n_ % 1000 == 0)                                                    \
      return lookup_pc();                                                    \
  }                                                                          \
  if (!r_ && xt_smc_pending && !XT_STORE_IS_ALIGNED_##name)                  \
  {                                                                          \
    /* an earlier word of this STM rewrote code: leave now */                \
    xt_smc_pending = 0;                                                      \
    return lookup_pc();                                                      \
  }                                                                          \
  return r_;                                                                 \
}
#define XT_STORE_IS_ALIGNED_u8 0
#define XT_STORE_IS_ALIGNED_u16 0
#define XT_STORE_IS_ALIGNED_u32 0
#define XT_STORE_IS_ALIGNED_aligned_u32 1
XT_STORE_ENTRY(u8)
XT_STORE_ENTRY(u16)
XT_STORE_ENTRY(u32)
XT_STORE_ENTRY(aligned_u32)

/* ---- CPSR / SPSR / SWI ------------------------------------------------------- */
static u32 xt_read_cpsr(void)
{
  collapse_flags();
  return reg[REG_CPSR];
}
static u32 xt_read_spsr(void)
{
  collapse_flags();
  return spsr[reg[CPU_MODE] & 0xF];
}

u32 execute_store_cpsr_body(void)
{
  set_cpu_mode(cpu_modes[reg[REG_CPSR] & 0xF]);
  if((io_registers[REG_IE] & io_registers[REG_IF]) &&
      io_registers[REG_IME] && ((reg[REG_CPSR] & 0x80) == 0))
  {
    REG_MODE(MODE_IRQ)[6] = reg[REG_PC] + 4;
    REG_SPSR(MODE_IRQ) = reg[REG_CPSR];
    reg[REG_CPSR] = (reg[REG_CPSR] & 0xFFFFFF00) | 0xD2;
    set_cpu_mode(MODE_IRQ);
    return 0x00000018;
  }
  return 0;
}

static u32 xt_store_cpsr(u32 new_cpsr, u32 mask_user, u32 mask_sys)
{
  u32 mask = (reg[CPU_MODE] & 0x10) ? mask_sys : mask_user, r;
  reg[REG_CPSR] = (new_cpsr & mask) | (reg[REG_CPSR] & ~mask);
  if (!(mask & 0xFF))
  {
    extract_flags_regs();
    return 0;
  }
  r = execute_store_cpsr_body();
  extract_flags_regs();
  return r ? XT_EXEC(block_lookup_address_arm(r)) : 0;
}

static void xt_store_spsr(u32 value, u32 mask)
{
  u32 *s = &spsr[reg[CPU_MODE] & 0xF];
  *s = (value & mask) | (*s & ~mask);
}

u32 execute_spsr_restore(u32 address)
{
  if(reg[CPU_MODE] != MODE_USER && reg[CPU_MODE] != MODE_SYSTEM)
  {
    reg[REG_CPSR] = REG_SPSR(reg[CPU_MODE]);
    extract_flags_regs();
    set_cpu_mode(cpu_modes[reg[REG_CPSR] & 0xF]);

    if((io_registers[REG_IE] & io_registers[REG_IF]) &&
     io_registers[REG_IME] && ((reg[REG_CPSR] & 0x80) == 0))
    {
      REG_MODE(MODE_IRQ)[6] = reg[REG_PC] + 4;
      REG_SPSR(MODE_IRQ) = reg[REG_CPSR];
      reg[REG_CPSR] = 0xD2;
      address = 0x00000018;
      set_cpu_mode(MODE_IRQ);
    }

    if(reg[REG_CPSR] & 0x20)
      address |= 0x01;
  }
  return address;
}

static void xt_swi(u32 pc)
{
  collapse_flags();
  reg[REG_BUS_VALUE] = 0xe3a02004;
  REG_MODE(MODE_SUPERVISOR)[6] = pc;
  REG_SPSR(MODE_SUPERVISOR) = reg[REG_CPSR];
  reg[REG_CPSR] = (reg[REG_CPSR] & ~0x3F) | 0x13 | 0x80;
  set_cpu_mode(MODE_SUPERVISOR);
}

/* the x86 inline division (skipped on a zero divisor) */
static void xt_hle_div_common(s32 num, s32 den)
{
  s32 q, r;
  if (!den)
    return;
  q = num / den;
  r = num % den;
  reg[0] = q;
  reg[1] = r;
  reg[3] = q < 0 ? -q : q;
}
static void xt_hle_div(void)     { xt_hle_div_common(reg[0], reg[1]); }
static void xt_hle_div_arm(void) { xt_hle_div_common(reg[1], reg[0]); }

/* ---- m4a mixer (cpu.cpp, RETRO_GO) ------------------------------------------- */
#ifdef RETRO_GO
u32 m4a_dynarec_run(u32 pc, s32 *cycles);
void m4a_dynarec_check(void);
static u32 xt_m4a(u32 pc)
{
  s32 cycles = (s32)reg[XT_CYC_SLOT];
  u32 next = m4a_dynarec_run(pc, &cycles);
  if (next == 0xFFFFFFFF)
    return 0;
  reg[XT_CYC_SLOT] = cycles;
  return XT_EXEC(block_lookup_address_arm(next));
}
#else
static u32 xt_m4a(u32 pc) { return 0; }
#endif

/* ---- caches ------------------------------------------------------------------ */
#ifdef GBAPROF
u32 xt_prof_syncs, xt_prof_sync_cycles;
#endif
void platform_cache_sync(void *baseaddr, void *endptr)
{
#if defined(ESP_PLATFORM)
  uintptr_t a = (uintptr_t)baseaddr & ~(uintptr_t)63, b = ((uintptr_t)endptr + 63) & ~(uintptr_t)63;
  if (b <= a)
    return;
#ifdef GBAPROF
  u32 t0 = esp_cpu_get_cycle_count();
#endif
#ifndef XT_IRAM_CACHE   /* internal RAM: no cache in the way */
  esp_cache_msync((void *)a, b - a, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
  esp_cache_msync((void *)(a + xt_exec_delta), b - a, ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_TYPE_INST);
#endif
  __asm__ volatile("memw; isync");
#ifdef GBAPROF
  xt_prof_syncs++;
  xt_prof_sync_cycles += esp_cpu_get_cycle_count() - t0;
#endif
#endif
}

/* ---- init and entry ------------------------------------------------------------ */
void init_emitter(bool must_swap)
{
  u8 *translation_ptr = &rom_translation_cache[XT_INITIAL_ROM_WATERMARK];
  static void *const fn[XT_FN_COUNT] = {
    [XT_FN_UPDATE_GBA] = xt_update_gba,
    [XT_FN_INDIRECT_ARM] = xt_indirect_arm,
    [XT_FN_INDIRECT_THUMB] = xt_indirect_thumb,
    [XT_FN_INDIRECT_DUAL] = xt_indirect_dual,
    [XT_FN_LOAD_U8] = xt_load_u8,
    [XT_FN_LOAD_S8] = xt_load_s8,
    [XT_FN_LOAD_U16] = xt_load_u16,
    [XT_FN_LOAD_S16] = xt_load_s16,
    [XT_FN_LOAD_U32] = xt_load_u32,
    [XT_FN_STORE_U8] = xt_store_u8,
    [XT_FN_STORE_U16] = xt_store_u16,
    [XT_FN_STORE_U32] = xt_store_u32,
    [XT_FN_STORE_ALIGNED_U32] = xt_store_aligned_u32,
    [XT_FN_READ_CPSR] = xt_read_cpsr,
    [XT_FN_READ_SPSR] = xt_read_spsr,
    [XT_FN_STORE_CPSR] = xt_store_cpsr,
    [XT_FN_STORE_SPSR] = xt_store_spsr,
    [XT_FN_SPSR_RESTORE] = execute_spsr_restore,
    [XT_FN_SWI] = xt_swi,
    [XT_FN_PROCESS_CHEATS] = process_cheats,
    [XT_FN_HLE_DIV] = xt_hle_div,
    [XT_FN_HLE_DIV_ARM] = xt_hle_div_arm,
    [XT_FN_M4A] = xt_m4a,
  };
  for (int i = 0; i < XT_FN_COUNT; i++)
    reg[XT_FN_SLOT + i] = (u32)(uintptr_t)fn[i];
  /* the fast loads (xt_fastmem.S): their bases, and the C helpers they fall
     back to; the table's load entries become the call0 routines */
  reg[XT_SLOT_IWRAM] = (u32)(uintptr_t)&iwram[0x8000];
  reg[XT_SLOT_EWRAM] = (u32)(uintptr_t)ewram;
  reg[XT_SLOT_MAPRD] = (u32)(uintptr_t)memory_map_read;
  reg[XT_SLOT_CLD_U8] = (u32)(uintptr_t)xt_load_u8;
  reg[XT_SLOT_CLD_S8] = (u32)(uintptr_t)xt_load_s8;
  reg[XT_SLOT_CLD_U16] = (u32)(uintptr_t)xt_load_u16;
  reg[XT_SLOT_CLD_S16] = (u32)(uintptr_t)xt_load_s16;
  reg[XT_SLOT_CLD_U32] = (u32)(uintptr_t)xt_load_u32;
  reg[XT_SLOT_SMCPEND] = 0;
  reg[XT_SLOT_EWTAG] = (u32)(uintptr_t)&ewram[0x40000];
  reg[XT_SLOT_VRAM] = (u32)(uintptr_t)vram;
#ifdef RETRO_GO
  reg[XT_SLOT_VDIRTY] = (u32)(uintptr_t)gbsp_vram_dirty;
  reg[XT_SLOT_VDANY] = (u32)(uintptr_t)&gbsp_vram_dirty_any;
#else
  {
    static u32 xt_vdirty_none[3];
    static u8 xt_vdany_none;
    reg[XT_SLOT_VDIRTY] = (u32)(uintptr_t)xt_vdirty_none;
    reg[XT_SLOT_VDANY] = (u32)(uintptr_t)&xt_vdany_none;
  }
#endif
  reg[XT_SLOT_CST_U8] = (u32)(uintptr_t)xt_store_u8;
  reg[XT_SLOT_CST_U16] = (u32)(uintptr_t)xt_store_u16;
  reg[XT_SLOT_CST_U32] = (u32)(uintptr_t)xt_store_u32;
  reg[XT_SLOT_CST_AU32] = (u32)(uintptr_t)xt_store_aligned_u32;
  if (xt_faststore)
  {
    extern u32 xt_fast_st_u8(void), xt_fast_st_u16(void), xt_fast_st_u32(void), xt_fast_st_au32(void);
    reg[XT_FN_SLOT + XT_FN_STORE_U8] = (u32)(uintptr_t)xt_fast_st_u8;
    reg[XT_FN_SLOT + XT_FN_STORE_U16] = (u32)(uintptr_t)xt_fast_st_u16;
    reg[XT_FN_SLOT + XT_FN_STORE_U32] = (u32)(uintptr_t)xt_fast_st_u32;
    reg[XT_FN_SLOT + XT_FN_STORE_ALIGNED_U32] = (u32)(uintptr_t)xt_fast_st_au32;
  }
  if (xt_fastmem)
  {
    extern u32 xt_fast_ld_u8(void), xt_fast_ld_s8(void), xt_fast_ld_u16(void), xt_fast_ld_s16(void), xt_fast_ld_u32(void);
    reg[XT_FN_SLOT + XT_FN_LOAD_U8] = (u32)(uintptr_t)xt_fast_ld_u8;
    reg[XT_FN_SLOT + XT_FN_LOAD_S8] = (u32)(uintptr_t)xt_fast_ld_s8;
    reg[XT_FN_SLOT + XT_FN_LOAD_U16] = (u32)(uintptr_t)xt_fast_ld_u16;
    reg[XT_FN_SLOT + XT_FN_LOAD_S16] = (u32)(uintptr_t)xt_fast_ld_s16;
    reg[XT_FN_SLOT + XT_FN_LOAD_U32] = (u32)(uintptr_t)xt_fast_ld_u32;
  }

  /* xt_enter(cycles, target, regs): entry a1,32; the registers of
     translated code; jx target */
  translation_ptr = (u8 *)(((uintptr_t)translation_ptr + 3) & ~(uintptr_t)3);
  xt_enter = (void *)(uintptr_t)XT_EXEC(translation_ptr);
  XT(entry, 1, 32);
  XT(mov, 9, 3);            /* target */
  XT(mov, reg_cycles, 2);   /* a3 = cycles */
  XT(mov, reg_base, 4);     /* a2 = &reg[0] */
  XT(s32i, 0, reg_base, XT_SLOT_A0 * 4);   /* a0: callx0 into the fast loads overwrites it */
  translation_ptr = xt_sync_from_mem(translation_ptr);   /* r0..r2 -> a4/a6/a7 */
  XT(jx, 9);
  /* leaving translated code: back to xt_enter's caller */
  xt_exit_stub = translation_ptr;
  translation_ptr = xt_sync_to_mem(translation_ptr);
  XT(l32i, 0, reg_base, XT_SLOT_A0 * 4);
  XT(retw);
  translation_ptr = (u8 *)(((uintptr_t)translation_ptr + 15) & ~(uintptr_t)15);
  platform_cache_sync(rom_translation_cache, translation_ptr);
  rom_cache_watermark = (u32)(translation_ptr - rom_translation_cache);
  init_bios_hooks();
}

/* set when the game keeps rewriting its own code in RAM: every rewrite
   drops and retranslates the block (gpSP's SMC handling), which then costs
   far more than interpreting it; the app switches to execute_arm */
int xt_give_up;
static void xt_check_smc_storm(void)
{
  extern u32 xt_ram_translations;
  extern void xt_smc_frame(void);
  xt_smc_frame();
  static int storm;
  const u32 n = xt_ram_translations;
  storm = n > 1000 ? storm + 1 : 0;
  xt_ram_translations = 0;
  if ((storm >= 3 || n > 20000) && !xt_give_up)
  {
    xt_give_up = 1;
    RG_LOGW("dynarec: RAM code rewritten continuously (%u blocks/frame), using the interpreter", (unsigned)n);
  }
}

u32 execute_arm_translate(u32 cycles)
{
  u32 target;
  xt_check_smc_storm();
  /* no translated code runs here: the RAM cache can go back to its start
     (flush_translation_cache_ram only appends when called from helpers) */
  /* rewound at 3/4 (was 1/2): the last quarter (160 KB) is the room for one
     frame's translations, as a flush from a helper only appends */
  if (ram_translation_ptr >= ram_translation_cache + RAM_TRANSLATION_CACHE_SIZE / 4 * 3)
  {
    extern int xt_ram_rewind;
    xt_ram_rewind = 1;
    flush_translation_cache_ram();
    xt_ram_rewind = 0;
  }
#ifdef RETRO_GO
  m4a_dynarec_check();   /* once a frame, as execute_arm does */
#endif
  extract_flags_regs();
  reg[XT_CYC_SLOT] = cycles;
  if (reg[CPU_HALT_STATE])
  {
    /* cpu_sleep_loop */
    u32 r = update_gba((s32)cycles);
    if (r & 0x80000000)
      return 0;
    reg[XT_CYC_SLOT] = r & 0x7FFF;
  }
  target = lookup_pc();
  xt_enter((s32)reg[XT_CYC_SLOT], target, reg);
  return 0;
}
