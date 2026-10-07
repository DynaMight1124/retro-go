/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NDRC_RV32_FLAGS_H
#define NDRC_RV32_FLAGS_H
#include "abi.h"

/* Private canonical NZCV state: t3 = result (N/Z), t4 = carry (0/1),
 * t5 = signed overflow (0/1). Subtraction carry means NO borrow, as on ARM.
 * This is host compiler predicate state, not emulated PSX architectural flags.
 * Non-flag-setting emitter operations must not use t3..t5 as scratch.
 */
enum rv32_predicate {
  RV_P_EQ, RV_P_NE, RV_P_CS, RV_P_CC, RV_P_MI, RV_P_PL, RV_P_VS, RV_P_VC,
  RV_P_HI, RV_P_LS, RV_P_GE, RV_P_LT, RV_P_GT, RV_P_LE
};

static inline int rv32_flag_operand(unsigned r)
{
  /* Accept zero, allocator registers and s0; never ABI/scratch registers. */
  return r == RV_ZERO || (r >= RV_T0 && r <= RV_S11);
}

/* rd=zero discards the result (CMP); source/destination aliasing is supported.
 * Reserve the entire sequence before touching output, also under NDEBUG.
 */
static inline void rv32_flag_arithmetic(struct rv32_emitter *e, unsigned op,
                                        unsigned rd, unsigned a, unsigned b)
{
  if (!rv32_check(e, (op == RV_ADD || op == RV_SUB) &&
      (rv32_flag_operand(rd) || rd == RV_T6) &&
      (rv32_flag_operand(a) || a == RV_T6) &&
      (rv32_flag_operand(b) || b == RV_T6)) || !rv32_reserve(e, 11)) return;
  rv32_imm(e, RV_ADD, RV_T3, a, 0);
  rv32_imm(e, RV_ADD, RV_T4, b, 0);
  rv32_alu(e, op, NDRC_RV32_AUX_REG, RV_T3, RV_T4);
  rv32_alu(e, RV_XOR, RV_T5, RV_T3, RV_T4);
  if (op == RV_ADD) {
    rv32_imm(e, RV_XOR, RV_T5, RV_T5, -1);
    rv32_alu(e, RV_SLTU, RV_T4, NDRC_RV32_AUX_REG, RV_T4);
  }
  else {
    rv32_alu(e, RV_SLTU, RV_T4, RV_T3, RV_T4);
    rv32_imm(e, RV_XOR, RV_T4, RV_T4, 1);
  }
  rv32_alu(e, RV_XOR, RV_T3, RV_T3, NDRC_RV32_AUX_REG);
  rv32_alu(e, RV_AND, RV_T5, RV_T5, RV_T3);
  rv32_shift(e, RV_SRL, RV_T5, RV_T5, 31);
  rv32_imm(e, RV_ADD, RV_T3, NDRC_RV32_AUX_REG, 0);
  if (rd != RV_ZERO) rv32_imm(e, RV_ADD, rd, NDRC_RV32_AUX_REG, 0);
}

/* AArch64-style TST: N/Z from AND, C=V=0. Do not assume ARM32's TST carry
 * behaviour is identical when eventually adapting shared compiler helpers.
 */
static inline void rv32_flag_test(struct rv32_emitter *e, unsigned a, unsigned b)
{
  if (!rv32_check(e,
      (rv32_flag_operand(a) || a == RV_T6) &&
      (rv32_flag_operand(b) || b == RV_T6)) ||
      !rv32_reserve(e, 3)) return;
  rv32_alu(e, RV_AND, RV_T3, a, b);
  rv32_li(e, RV_T4, 0);
  rv32_li(e, RV_T5, 0);
}

/* Materialize a predicate as 0/1 without consuming the saved NZCV state.
 * Subsequent branch or conditional move lowering can use this boolean.
 */
static inline void rv32_flag_predicate(struct rv32_emitter *e, unsigned pred, unsigned rd)
{
  if (!rv32_check(e, pred <= RV_P_LE && rd != RV_ZERO &&
      (rv32_flag_operand(rd) || (rd == RV_T6 && pred <= RV_P_LT))) ||
      !rv32_reserve(e, 6)) return;
  switch (pred) {
  case RV_P_EQ: case RV_P_NE:
    rv32_alu(e, RV_SLTU, rd, RV_ZERO, RV_T3);
    if (pred == RV_P_EQ) rv32_imm(e, RV_XOR, rd, rd, 1);
    break;
  case RV_P_CS: case RV_P_CC:
    rv32_imm(e, RV_XOR, rd, RV_T4, pred == RV_P_CC);
    break;
  case RV_P_MI: case RV_P_PL:
    rv32_shift(e, RV_SRL, rd, RV_T3, 31);
    if (pred == RV_P_PL) rv32_imm(e, RV_XOR, rd, rd, 1);
    break;
  case RV_P_VS: case RV_P_VC:
    rv32_imm(e, RV_XOR, rd, RV_T5, pred == RV_P_VC);
    break;
  case RV_P_HI: case RV_P_LS:
    rv32_alu(e, RV_SLTU, rd, RV_ZERO, RV_T3);
    rv32_alu(e, RV_AND, rd, rd, RV_T4);
    if (pred == RV_P_LS) rv32_imm(e, RV_XOR, rd, rd, 1);
    break;
  default:
    rv32_shift(e, RV_SRL, rd, RV_T3, 31);
    rv32_alu(e, RV_XOR, rd, rd, RV_T5); /* signed less: N != V */
    if (pred == RV_P_GE || pred == RV_P_GT) rv32_imm(e, RV_XOR, rd, rd, 1);
    if (pred == RV_P_GT || pred == RV_P_LE) {
      rv32_imm(e, RV_SLTU, RV_T6, RV_T3, 1); /* Z */
      if (pred == RV_P_GT) {
        rv32_imm(e, RV_XOR, RV_T6, RV_T6, 1);
        rv32_alu(e, RV_AND, rd, rd, RV_T6);
      }
      else rv32_alu(e, RV_OR, rd, rd, RV_T6);
    }
    break;
  }
}
#endif
