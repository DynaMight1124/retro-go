/* gameplaySP - Xtensa LX7 (ESP32-S3) dynamic recompiler backend
 *
 * esp32-emu-turbo. A port of the x86 backend (x86/x86_emit.h): the ARM state
 * lives in memory (reg[], flags in reg[REG_N_FLAG..REG_V_FLAG] as 0/1), and
 * every instruction goes through the same steps the x86 backend emits, so
 * the translated code does what the x86 dynarec's does, cycle accounting
 * included (the reference: retro-go/gbajit-test/x86ref). The x86 flag
 * register is replaced by explicit computations (saltu, nsau, extui).
 *
 * The x86 stub's memory handlers, update and branch trampolines are C
 * functions in xtensa_stub.c, called from translated code with callx8
 * through a table in reg[REG_USERDEF..]; a handler that must leave the block
 * (IRQ, halt, self-modifying code, end of frame) returns the address to jump
 * to, 0 otherwise.
 *
 * Instructions are encoded by components/xjit (xjit_emit.h, checked against
 * the assembler). Translated code runs inside the window of xt_enter (a
 * generated windowed function): a0 = its return address on entry, saved in
 * reg[XT_SLOT_A0] and restored by the exit stub before retw, since the
 * loads callx0 into xt_fastmem.S and overwrite it (backtraces inside
 * translated code are therefore wrong), a1 = stack, a2 = &reg[0], a3 = cycles left, a4 = x86 esi (t0),
 * a5 = the block's start PC; a10/a11/a12 = x86 eax/edx/ecx (callx8
 * arguments, a10 the result); a8, a9, a13..a15 scratch. a8..a15 do not
 * survive a call, a2..a7 do.
 *
 * The translation caches are written through their data address and run from
 * their instruction address, xt_exec_delta apart (both for internal RAM and
 * for PSRAM mapped executable): every code address handed to translated code
 * or to jx is converted.
 */

#ifndef XTENSA_EMIT_H
#define XTENSA_EMIT_H

#include "xjit_emit.h"

/* ---- host registers ---------------------------------------------------- */
#define reg_base    2
#define reg_cycles  3
#define reg_t0      8    /* x86 esi: only held between two helper calls here */
#define reg_pcbase  5
/* guest registers kept in host registers in all translated code: a4, a6, a7
   survive callx8 (the helper calls), so r0..r2 never go to reg[] except
   where C code reads or writes them (xt_sync_*: HLE divide, m4a, cheats,
   entering and leaving translated code) */
#define XT_MAP_R0   4
#define XT_MAP_R1   6
#define XT_MAP_R2   7
#define XT_MAPPED   3
#define reg_a0      10
#define reg_a1      11
#define reg_a2      12
#define reg_rv      10
#define reg_arg0    10
#define reg_arg1    11
#define XT_CALLREG  8
#define XT_S0       9
#define XT_S1       13
#define XT_S2       14
#define XT_S3       15

/* reg[] slots used by this backend (REG_USERDEF.. is free for it) */
#define XT_CYC_SLOT (REG_USERDEF + 0)   /* cycles handed back by the C helpers */
#define XT_FN_SLOT  (REG_USERDEF + 1)   /* function table, see enum below */

enum
{
  XT_FN_UPDATE_GBA,
  XT_FN_INDIRECT_ARM,
  XT_FN_INDIRECT_THUMB,
  XT_FN_INDIRECT_DUAL,
  XT_FN_LOAD_U8,
  XT_FN_LOAD_S8,
  XT_FN_LOAD_U16,
  XT_FN_LOAD_S16,
  XT_FN_LOAD_U32,
  XT_FN_STORE_U8,
  XT_FN_STORE_U16,
  XT_FN_STORE_U32,
  XT_FN_STORE_ALIGNED_U32,
  XT_FN_READ_CPSR,
  XT_FN_READ_SPSR,
  XT_FN_STORE_CPSR,
  XT_FN_STORE_SPSR,
  XT_FN_SPSR_RESTORE,
  XT_FN_SWI,
  XT_FN_PROCESS_CHEATS,
  XT_FN_HLE_DIV,
  XT_FN_HLE_DIV_ARM,
  XT_FN_M4A,
  XT_FN_COUNT
};
#include "xt_fastmem.h"
_Static_assert(XT_FN_SLOT + XT_FN_COUNT == XT_SLOT_A0, "xt_fastmem.h slots must follow the function table");
_Static_assert(XT_SLOT_CST_AU32 < REG_MAX, "the backend's reg[] slots run past reg[REG_MAX]");
/* 1: loads call the call0 routines of xt_fastmem.S (callx0), 0: the C
   helpers (callx8); fixed when the emitter is initialised (gba_nofast) */
extern int xt_fastmem;
extern int xt_faststore;
extern int xt_cycfix;

int m4a_dynarec_head(u32 pc);
extern u32 xt_exec_delta;
void platform_cache_sync(void *baseaddr, void *endptr);
extern u8 *xt_exit_stub;          /* data address of "retw" */
void xt_emit_error(const char *what, int line);

/* ---- one instruction at translation_ptr ---------------------------------- */
/* Out of line (xtensa_stub.c): the translator emits instructions in
   thousands of places, inlined encoders made its functions exceed the
   Xtensa l32r literal range. */
u8 *xto_retw(u8 *p);
u8 *xto_callx8(u8 *p, int a0);
u8 *xto_callx0(u8 *p, int a0);
u8 *xto_nop(u8 *p);
u8 *xto_jx(u8 *p, int a0);
u8 *xto_ssl(u8 *p, int a0);
u8 *xto_ssr(u8 *p, int a0);
u8 *xto_ssai(u8 *p, int a0);
u8 *xto_j(u8 *p, int a0);
u8 *xto_mov(u8 *p, int a0, int a1);
u8 *xto_movi(u8 *p, int a0, int a1);
u8 *xto_nsau(u8 *p, int a0, int a1);
u8 *xto_beqz(u8 *p, int a0, int a1);
u8 *xto_bnez(u8 *p, int a0, int a1);
u8 *xto_bgez(u8 *p, int a0, int a1);
u8 *xto_l32r(u8 *p, int a0, int a1);
u8 *xto_entry(u8 *p, int a0, int a1);
u8 *xto_sll(u8 *p, int a0, int a1);
u8 *xto_srl(u8 *p, int a0, int a1);
u8 *xto_sra(u8 *p, int a0, int a1);
u8 *xto_add(u8 *p, int a0, int a1, int a2);
u8 *xto_addi(u8 *p, int a0, int a1, int a2);
u8 *xto_addmi(u8 *p, int a0, int a1, int a2);
u8 *xto_and(u8 *p, int a0, int a1, int a2);
u8 *xto_l32i(u8 *p, int a0, int a1, int a2);
u8 *xto_s32i(u8 *p, int a0, int a1, int a2);
u8 *xto_mull(u8 *p, int a0, int a1, int a2);
u8 *xto_mulsh(u8 *p, int a0, int a1, int a2);
u8 *xto_muluh(u8 *p, int a0, int a1, int a2);
u8 *xto_or(u8 *p, int a0, int a1, int a2);
u8 *xto_saltu(u8 *p, int a0, int a1, int a2);
u8 *xto_slli(u8 *p, int a0, int a1, int a2);
u8 *xto_srai(u8 *p, int a0, int a1, int a2);
u8 *xto_src(u8 *p, int a0, int a1, int a2);
u8 *xto_srli(u8 *p, int a0, int a1, int a2);
u8 *xto_sub(u8 *p, int a0, int a1, int a2);
u8 *xto_xor(u8 *p, int a0, int a1, int a2);
u8 *xto_bltui(u8 *p, int a0, int a1, int a2);
u8 *xto_bgeui(u8 *p, int a0, int a1, int a2);
u8 *xto_extui(u8 *p, int a0, int a1, int a2, int a3);
#define XT(op, ...) translation_ptr = xto_##op(translation_ptr, ##__VA_ARGS__)

/* movi when the value fits 12 bits, else an inline literal:
   j over; [pad]; .word value; over: l32r ireg, value */
static __attribute__((noinline)) u8 *xt_load_imm32(u8 *translation_ptr, int ireg, u32 value)
{
  s32 v = (s32)value;
  if (v >= -2048 && v <= 2047)
  {
    XT(movi, ireg, v);
    return translation_ptr;
  }
  {
    u8 *j = translation_ptr;
    u32 lit = ((uintptr_t)(j + 3) + 3) & ~3u;       /* data address, word aligned */
    u8 *over = (u8 *)(uintptr_t)(lit + 4);
    XT(j, (int)(over - (j + 4)));
    while (translation_ptr < (u8 *)(uintptr_t)lit)
      *translation_ptr++ = 0;
    *(u32 *)translation_ptr = value;
    translation_ptr += 4;
    XT(l32r, ireg, -4);
  }
  return translation_ptr;
}
#define generate_load_imm(ireg, imm)                                          \
  translation_ptr = xt_load_imm32(translation_ptr, reg_##ireg, (u32)(imm))

/* ireg = reg_pcbase + delta when close to the block's start PC */
static __attribute__((noinline)) u8 *xt_load_pc(u8 *translation_ptr, int ireg, u32 new_pc, u32 stored_pc)
{
  s32 d = (s32)(new_pc - stored_pc);
  if (d >= -128 && d <= 127)
    XT(addi, ireg, reg_pcbase, d);
  else if (d >= -32768 && d < 32512)
  {
    s32 hi = (d + 128) & ~0xFF, lo = d - hi;
    XT(addmi, ireg, reg_pcbase, hi);
    if (lo)
      XT(addi, ireg, ireg, lo);
  }
  else
    translation_ptr = xt_load_imm32(translation_ptr, ireg, new_pc);
  return translation_ptr;
}
#define generate_load_pc(ireg, new_pc)                                        \
  translation_ptr = xt_load_pc(translation_ptr, reg_##ireg, (new_pc), stored_pc)

static inline int xt_host_of(u32 r)
{
  return r == 0 ? XT_MAP_R0 : r == 1 ? XT_MAP_R1 : r == 2 ? XT_MAP_R2 : -1;
}
static __attribute__((noinline)) u8 *xt_load_reg(u8 *translation_ptr, int h, u32 r)
{
  int m = xt_host_of(r);
  if (m >= 0)
    XT(mov, h, m);
  else
    XT(l32i, h, reg_base, r * 4);
  return translation_ptr;
}
static __attribute__((noinline)) u8 *xt_store_reg(u8 *translation_ptr, int h, u32 r)
{
  int m = xt_host_of(r);
  if (m >= 0)
    XT(mov, m, h);
  else
    XT(s32i, h, reg_base, r * 4);
  return translation_ptr;
}
static __attribute__((noinline)) u8 *xt_store_reg_i32(u8 *translation_ptr, u32 imm, u32 r)
{
  int m = xt_host_of(r);
  if (m >= 0)
    return xt_load_imm32(translation_ptr, m, imm);
  translation_ptr = xt_load_imm32(translation_ptr, XT_S0, imm);
  XT(s32i, XT_S0, reg_base, r * 4);
  return translation_ptr;
}
/* reg[0..2] <-> a4/a6/a7 around C code that reads or writes them */
static __attribute__((noinline)) u8 *xt_sync_to_mem(u8 *translation_ptr)
{
  XT(s32i, XT_MAP_R0, reg_base, 0);
  XT(s32i, XT_MAP_R1, reg_base, 4);
  XT(s32i, XT_MAP_R2, reg_base, 8);
  return translation_ptr;
}
static __attribute__((noinline)) u8 *xt_sync_from_mem(u8 *translation_ptr)
{
  XT(l32i, XT_MAP_R0, reg_base, 0);
  XT(l32i, XT_MAP_R1, reg_base, 4);
  XT(l32i, XT_MAP_R2, reg_base, 8);
  return translation_ptr;
}
#define generate_load_reg(ireg, reg_index)                                    \
  translation_ptr = xt_load_reg(translation_ptr, reg_##ireg, (reg_index))
#define generate_store_reg(ireg, reg_index)                                   \
  translation_ptr = xt_store_reg(translation_ptr, reg_##ireg, (reg_index))
#define generate_store_reg_i32(imm32, reg_index)                              \
  translation_ptr = xt_store_reg_i32(translation_ptr, (imm32), (reg_index))
#define reg_s0 XT_S0
#define reg_s1 XT_S1
#define reg_s2 XT_S2
#define reg_s3 XT_S3

#define generate_mov(ireg_dest, ireg_src)                                     \
  XT(mov, reg_##ireg_dest, reg_##ireg_src)

/* ireg += imm (any 32-bit value) */
static __attribute__((noinline)) u8 *xt_add_imm(u8 *translation_ptr, int ireg, u32 imm)
{
  s32 v = (s32)imm;
  if (v == 0)
    return translation_ptr;
  if (v >= -128 && v <= 127)
    XT(addi, ireg, ireg, v);
  else if (v >= -32768 && v < 32512)
  {
    s32 hi = (v + 128) & ~0xFF, lo = v - hi;
    XT(addmi, ireg, ireg, hi);
    if (lo)
      XT(addi, ireg, ireg, lo);
  }
  else
  {
    translation_ptr = xt_load_imm32(translation_ptr, XT_S3, imm);
    XT(add, ireg, ireg, XT_S3);
  }
  return translation_ptr;
}
#define generate_add_imm(ireg, imm)                                           \
  translation_ptr = xt_add_imm(translation_ptr, reg_##ireg, (u32)(imm))
#define generate_sub_imm(ireg, imm)                                           \
  translation_ptr = xt_add_imm(translation_ptr, reg_##ireg, (u32)-(s32)(imm))
#define generate_and_imm(ireg, imm)                                           \
  generate_load_imm(s3, (imm));                                               \
  XT(and, reg_##ireg, reg_##ireg, XT_S3)

/* ---- calls into C (xtensa_stub.c) ---------------------------------------- */
#define xt_call(fn)                                                           \
  XT(l32i, XT_CALLREG, reg_base, (XT_FN_SLOT + (fn)) * 4);                    \
  XT(callx8, XT_CALLREG)

/* after a helper that may redirect: a10 = 0 to go on, else where to jump
   (with the cycles it left in reg[XT_CYC_SLOT]) */
#define xt_jump_if_redirect()                                                 \
  {                                                                           \
    u8 *xt_go_on;                                                             \
    XT_FWD_B12(xt_go_on, beqz, reg_rv);                                       \
    XT(l32i, reg_cycles, reg_base, XT_CYC_SLOT * 4);                          \
    XT(jx, reg_rv);                                                           \
    xt_fwd_b12(xt_go_on, translation_ptr);                                    \
  }

/* the same, with the rare jump in the block's cold area: "beqz a10, +2; j cold"
   in the hot path, "l32i a3, slot; jx a10" out of line */
#define xt_redirect_cold()                                                    \
  if (xt_cold_n < XT_COLD_MAX)                                                \
  {                                                                           \
    XT(beqz, reg_rv, 2);                                                      \
    xt_cold[xt_cold_n].hot = translation_ptr;                                 \
    xt_cold[xt_cold_n++].kind = XT_COLD_REDIRECT;                             \
    XT(j, 0);                                                                 \
  }                                                                           \
  else                                                                        \
    xt_jump_if_redirect()

/* ---- cycle counter --------------------------------------------------------- */
#define generate_cycle_update()                                               \
  translation_ptr = xt_add_imm(translation_ptr, reg_cycles, (u32)-(s32)cycle_count); \
  cycle_count = 0

/* ---- block exits and patching ------------------------------------------------ */
/* A patchable exit: j over; [pad]; .word target; over: l32r a8; jx a8.
   The patch writes the target's instruction address into the word, and
   turns the first j into a direct jump when the target is within reach. */
#define xt_emit_exit_filler(writeback_location)                               \
  (writeback_location) = translation_ptr;                                     \
  translation_ptr = xt_load_imm32_force(translation_ptr, XT_CALLREG, 0);      \
  XT(jx, XT_CALLREG)

static __attribute__((noinline)) u8 *xt_load_imm32_force(u8 *translation_ptr, int ireg, u32 value)
{
  u8 *j = translation_ptr;
  u32 lit = ((uintptr_t)(j + 3) + 3) & ~3u;
  u8 *over = (u8 *)(uintptr_t)(lit + 4);
  XT(j, (int)(over - (j + 4)));
  while (translation_ptr < (u8 *)(uintptr_t)lit)
    *translation_ptr++ = 0;
  *(u32 *)translation_ptr = value;
  translation_ptr += 4;
  XT(l32r, ireg, -4);
  return translation_ptr;
}

/* dest: the exit's first j. Its target is "l32r a8; jx a8" with the literal
   word just before it, inline (xt_emit_exit_filler) or in the block's cold
   area (xt_emit_cold): write the literal, and turn the j into a direct jump
   when the target is within reach. */
static inline void xt_patch_exit(u8 *dest, u8 *target)
{
  u32 w = dest[0] | (dest[1] << 8) | (dest[2] << 16);
  s32 off = ((s32)(w << 8)) >> 14;                               /* j: offset bits 23..6 */
  u32 lit = (u32)(uintptr_t)(dest + 4 + off) - 4;
  s32 d = (s32)((uintptr_t)target - (uintptr_t)(dest + 4));   /* same in both aliases */
  *(u32 *)(uintptr_t)lit = (u32)(uintptr_t)target + xt_exec_delta;
  if (d >= -131072 && d <= 131071)
  {
    u32 w = (((u32)d & 0x3FFFF) << 6) | 6;                     /* j target */
    dest[0] = w; dest[1] = w >> 8; dest[2] = w >> 16;
  }
}
#define generate_branch_patch_unconditional(dest, offset)                     \
  xt_patch_exit((u8 *)(dest), (u8 *)(offset))

/* conditional skip: a j whose target is patched (the condition code
   branches over it when the instruction must run) */
static inline void xt_patch_j(u8 *dest, u8 *target)
{
  s32 d = (s32)((uintptr_t)target - (uintptr_t)(dest + 4));
  u32 w = (((u32)d & 0x3FFFF) << 6) | 6;
  if (d < -131072 || d > 131071)
    xt_emit_error("conditional skip out of range", __LINE__);
  dest[0] = w; dest[1] = w >> 8; dest[2] = w >> 16;
}
#define generate_branch_patch_conditional(dest, offset)                       \
  xt_patch_j((u8 *)(dest), (u8 *)(offset))

/* ---- forward branches inside one emitted sequence ---------------------------- */
/* emit a branch with displacement 0, patch it later with xt_fwd_here() */
#define XT_FWD_B12(var, op, s)  (var) = translation_ptr; XT(op, s, 0)
#define XT_FWD_B8(var, op, s, t) (var) = translation_ptr; XT(op, s, t, 0)
#define XT_FWD_J(var)           (var) = translation_ptr; XT(j, 0)
static inline void xt_fwd_b12(u8 *at, u8 *target)
{
  s32 d = (s32)(target - (at + 4));
  if (d < -2048 || d > 2047) xt_emit_error("b12 range", __LINE__);
  at[1] = (at[1] & 0x0F) | ((d & 15) << 4);
  at[2] = (d >> 4) & 0xFF;
}
static inline void xt_fwd_b8(u8 *at, u8 *target)
{
  s32 d = (s32)(target - (at + 4));
  if (d < -128 || d > 127) xt_emit_error("b8 range", __LINE__);
  at[2] = d & 0xFF;
}
#define xt_fwd_j(at, target) xt_patch_j(at, target)

/* ---- flags ------------------------------------------------------------------ */
#define check_generate_n_flag   (flag_status & 0x08)
#define check_generate_z_flag   (flag_status & 0x04)
#define check_generate_c_flag   (flag_status & 0x02)
#define check_generate_v_flag   (flag_status & 0x01)

/* flag = value (a host register holding 0/1) */
#define xt_store_flag(hreg, regnum) XT(s32i, hreg, reg_base, (regnum) * 4)

/* N and Z from a result */
static __attribute__((noinline)) u8 *xt_nz_flags(u8 *translation_ptr, int res, u32 flag_status)
{
  if (check_generate_z_flag)
  {
    XT(nsau, XT_S0, res);
    XT(extui, XT_S0, XT_S0, 5, 1);
    xt_store_flag(XT_S0, REG_Z_FLAG);
  }
  if (check_generate_n_flag)
  {
    XT(extui, XT_S0, res, 31, 1);
    xt_store_flag(XT_S0, REG_N_FLAG);
  }
  return translation_ptr;
}
#define update_logical_flags_of(res)                                          \
  translation_ptr = xt_nz_flags(translation_ptr, reg_##res, flag_status)

/* res = a + b (+ carry-in held in cin, or -1 for none): C and V as x86 add/adc */
static __attribute__((noinline)) u8 *xt_add_op(u8 *translation_ptr, int res, int a, int b, int cin, u32 flag_status)
{
  /* res may alias a */
  if (!check_generate_c_flag && !check_generate_v_flag)
  {
    XT(add, res, a, b);
    if (cin >= 0)
      XT(add, res, res, cin);
    return xt_nz_flags(translation_ptr, res, flag_status);
  }
  XT(add, XT_S1, a, b);
  if (check_generate_c_flag)
    XT(saltu, XT_S2, XT_S1, b);               /* carry of a + b */
  if (cin >= 0)
  {
    XT(add, XT_S1, XT_S1, cin);
    if (check_generate_c_flag)
    {
      XT(saltu, XT_S0, XT_S1, cin);           /* carry of + cin */
      XT(or, XT_S2, XT_S2, XT_S0);
    }
  }
  if (check_generate_c_flag)
    xt_store_flag(XT_S2, REG_C_FLAG);
  if (check_generate_v_flag)
  {
    XT(xor, XT_S2, a, XT_S1);                 /* (a ^ res) & (b ^ res) */
    XT(xor, XT_S0, b, XT_S1);
    XT(and, XT_S2, XT_S2, XT_S0);
    XT(extui, XT_S2, XT_S2, 31, 1);
    xt_store_flag(XT_S2, REG_V_FLAG);
  }
  XT(mov, res, XT_S1);
  translation_ptr = xt_nz_flags(translation_ptr, res, flag_status);
  return translation_ptr;
}

/* res = a - b (- borrow-in held in bin as 0/1, or -1 for none):
   ARM C = not borrow, V as x86 sub/sbb */
static __attribute__((noinline)) u8 *xt_sub_op(u8 *translation_ptr, int res, int a, int b, int bin, u32 flag_status)
{
  if (!check_generate_c_flag && !check_generate_v_flag)
  {
    XT(sub, res, a, b);
    if (bin >= 0)
      XT(sub, res, res, bin);
    return xt_nz_flags(translation_ptr, res, flag_status);
  }
  XT(sub, XT_S1, a, b);
  if (check_generate_c_flag)
    XT(saltu, XT_S2, a, b);                   /* borrow of a - b */
  if (bin >= 0)
  {
    if (check_generate_c_flag)
    {
      XT(saltu, XT_S0, XT_S1, bin);           /* borrow of - bin */
      XT(or, XT_S2, XT_S2, XT_S0);
    }
    XT(sub, XT_S1, XT_S1, bin);
  }
  if (check_generate_c_flag)
  {
    XT(movi, XT_S0, 1);
    XT(xor, XT_S2, XT_S2, XT_S0);
    xt_store_flag(XT_S2, REG_C_FLAG);
  }
  if (check_generate_v_flag)
  {
    XT(xor, XT_S2, a, b);                     /* (a ^ b) & (a ^ res) */
    XT(xor, XT_S0, a, XT_S1);
    XT(and, XT_S2, XT_S2, XT_S0);
    XT(extui, XT_S2, XT_S2, 31, 1);
    xt_store_flag(XT_S2, REG_V_FLAG);
  }
  XT(mov, res, XT_S1);
  translation_ptr = xt_nz_flags(translation_ptr, res, flag_status);
  return translation_ptr;
}

/* ---- cold code at the end of the block --------------------------------------- */
#ifdef XT_NO_COLD        /* debug: everything inline */
#define XT_COLD_MAX 0
#else
#define XT_COLD_MAX 40   /* x ~32 bytes: stays inside TRANSLATION_CACHE_LIMIT_THRESHOLD */
#endif
enum { XT_COLD_UPDATE, XT_COLD_EXIT, XT_COLD_REDIRECT };
typedef struct { u8 *hot; u32 target_pc; u32 kind; } xt_cold_t;
extern xt_cold_t xt_cold[XT_COLD_MAX + 1];
extern int xt_cold_n;

static __attribute__((noinline)) u8 *xt_emit_cold(u8 *translation_ptr, u32 stored_pc)
{
  for (int i = 0; i < xt_cold_n; i++)
  {
    xt_cold_t *c = &xt_cold[i];
    if (c->kind == XT_COLD_UPDATE)
    {
      /* the hot "j" lands here; x86_update_gba, then back after it */
      xt_patch_j(c->hot, translation_ptr);
      translation_ptr = xt_load_pc(translation_ptr, reg_a0, c->target_pc, stored_pc);
      XT(s32i, reg_cycles, reg_base, XT_CYC_SLOT * 4);
      xt_call(XT_FN_UPDATE_GBA);
      XT(l32i, reg_cycles, reg_base, XT_CYC_SLOT * 4);
      XT(bnez, reg_rv, 2);                              /* over the j */
      XT(j, (int)((c->hot + 3) - (translation_ptr + 4)));
      XT(jx, reg_rv);
    }
    else if (c->kind == XT_COLD_REDIRECT)
    {
      xt_patch_j(c->hot, translation_ptr);
      XT(l32i, reg_cycles, reg_base, XT_CYC_SLOT * 4);
      XT(jx, reg_rv);
    }
    else
    {
      /* .word target; l32r a8; jx a8 (xt_patch_exit fills the word) */
      while ((uintptr_t)translation_ptr & 3)
        *translation_ptr++ = 0;
      *(u32 *)translation_ptr = 0;
      translation_ptr += 4;
      xt_patch_j(c->hot, translation_ptr);
      XT(l32r, XT_CALLREG, -4);
      XT(jx, XT_CALLREG);
    }
  }
  xt_cold_n = 0;
  return translation_ptr;
}

#include "xtensa_emit_ops.h"

#endif
