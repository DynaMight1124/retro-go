/* SPDX-License-Identifier: GPL-2.0-or-later
 * RV32 host-slot adapter for assemble_alu.h. Not a complete new_dynarec backend.
 * The including translation unit owns rv32_output and emit_loadreg/branch stubs.
 */
#include "rv32/flags.h"

#define HOST_REGS NDRC_RV32_HOST_REGS
#define HOST_TEMPREG NDRC_RV32_HOST_REGS

static unsigned rv32_hr(int slot)
{
  return slot == HOST_TEMPREG ? RV_T6 : rv32_host_reg((unsigned)slot);
}

#define RV32_BINARY(name, op) \
  static void name(int a, int b, int d) { \
    rv32_alu(&rv32_output, op, rv32_hr(d), rv32_hr(a), rv32_hr(b)); }
RV32_BINARY(emit_add, RV_ADD)
RV32_BINARY(emit_sub, RV_SUB)
RV32_BINARY(emit_and, RV_AND)
RV32_BINARY(emit_or, RV_OR)
RV32_BINARY(emit_xor, RV_XOR)
RV32_BINARY(emit_set_if_less32, RV_SLT)
RV32_BINARY(emit_set_if_carry32, RV_SLTU)
RV32_BINARY(emit_shl, RV_SLL)
RV32_BINARY(emit_shr, RV_SRL)
RV32_BINARY(emit_sar, RV_SRA)
#undef RV32_BINARY

static void emit_mov(int a, int d) { rv32_imm(&rv32_output, RV_ADD, rv32_hr(d), rv32_hr(a), 0); }
static void emit_movimm(uint32_t v, int d) { rv32_li(&rv32_output, rv32_hr(d), v); }
static void emit_zeroreg(int d) { emit_movimm(0, d); }
static void emit_not(int a, int d) { rv32_imm(&rv32_output, RV_XOR, rv32_hr(d), rv32_hr(a), -1); }
static void emit_neg(int a, int d) { rv32_alu(&rv32_output, RV_SUB, rv32_hr(d), RV_ZERO, rv32_hr(a)); }
static void emit_set_gz32(int a, int d) { rv32_alu(&rv32_output, RV_SLT, rv32_hr(d), RV_ZERO, rv32_hr(a)); }
static void emit_set_nz32(int a, int d) { rv32_alu(&rv32_output, RV_SLTU, rv32_hr(d), RV_ZERO, rv32_hr(a)); }

#define RV32_SHIFT(name, op) \
  static void name(int a, unsigned shift, int d) { \
    rv32_shift(&rv32_output, op, rv32_hr(d), rv32_hr(a), shift); }
RV32_SHIFT(emit_shlimm, RV_SLL)
RV32_SHIFT(emit_shrimm, RV_SRL)
RV32_SHIFT(emit_sarimm, RV_SRA)
#undef RV32_SHIFT

static void rv32_alu_immediate(unsigned op, int a, int32_t value, int d)
{
  if (value >= -2048 && value <= 2047)
    rv32_imm(&rv32_output, op, rv32_hr(d), rv32_hr(a), value);
  else {
    /* Shared ALU callers keep input in an allocator slot, not HOST_TEMPREG.
     * Do not silently destroy a scratch-register input on a future new caller. */
    if (!rv32_check(&rv32_output, a != HOST_TEMPREG) || !rv32_reserve(&rv32_output, 3)) return;
    rv32_li(&rv32_output, RV_T6, (uint32_t)value);
    rv32_alu(&rv32_output, op, rv32_hr(d), rv32_hr(a), RV_T6);
  }
}
#define RV32_IMMEDIATE(name, op) \
  static void name(int a, int32_t v, int d) { rv32_alu_immediate(op, a, v, d); }
RV32_IMMEDIATE(emit_addimm, RV_ADD)
RV32_IMMEDIATE(emit_andimm, RV_AND)
RV32_IMMEDIATE(emit_orimm, RV_OR)
RV32_IMMEDIATE(emit_xorimm, RV_XOR)
RV32_IMMEDIATE(emit_slti32, RV_SLT)
RV32_IMMEDIATE(emit_sltiu32, RV_SLTU)
#undef RV32_IMMEDIATE

static void emit_adds(int a, int b, int d)
{ rv32_flag_arithmetic(&rv32_output, RV_ADD, rv32_hr(d), rv32_hr(a), rv32_hr(b)); }
static void emit_subs(int a, int b, int d)
{ rv32_flag_arithmetic(&rv32_output, RV_SUB, rv32_hr(d), rv32_hr(a), rv32_hr(b)); }
static void emit_negs(int a, int d)
{ rv32_flag_arithmetic(&rv32_output, RV_SUB, rv32_hr(d), RV_ZERO, rv32_hr(a)); }
static void emit_addimm_and_set_flags3(int a, int32_t value, int d)
{
  if (!rv32_check(&rv32_output, a != HOST_TEMPREG) || !rv32_reserve(&rv32_output, 13)) return;
  rv32_li(&rv32_output, RV_T6, (uint32_t)value);
  rv32_flag_arithmetic(&rv32_output, RV_ADD, rv32_hr(d), rv32_hr(a), RV_T6);
}
static void host_tempreg_acquire(void) {}
static void host_tempreg_release(void) {}
