/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "flags.h"
#include "selftest.h"
extern void ndrc_rv32_test_clobber(void);
#define CHECK(x) do { if (!(x)) return __LINE__; } while (0)

/* Derive expected carry/overflow with wide arithmetic, independently of the
 * bitwise formulas emitted by flags.h. Pack all fourteen conditions. */
static uint32_t expected(unsigned op, uint32_t a, uint32_t b)
{
  uint32_t r = op == RV_ADD ? a + b : op == RV_SUB ? a - b : a & b;
  int64_t wide = op == RV_ADD ? (int64_t)(int32_t)a + (int32_t)b :
    (int64_t)(int32_t)a - (int32_t)b;
  int n = (int)(r >> 31), z = r == 0;
  int c = op == RV_ADD ? ((uint64_t)a + b > UINT32_MAX) : op == RV_SUB ? a >= b : 0;
  int v = op == RV_AND ? 0 : (wide > INT32_MAX || wide < INT32_MIN);
  int lt = n != v;
  return (uint32_t)(z | (!z << 1) | (c << 2) | (!c << 3) | (n << 4) | (!n << 5) |
    (v << 6) | (!v << 7) | ((c && !z) << 8) | ((!c || z) << 9) |
    (!lt << 10) | (lt << 11) | ((!z && !lt) << 12) | ((z || lt) << 13));
}

int ndrc_rv32_flags_selftest(void *code, size_t size, int (*publish)(void *, size_t))
{
  static const uint32_t values[] = { 0, 1, 2, 0x7fffffff, 0x80000000,
    0x80000001, 0xfffffffe, 0xffffffff, 0xaaaaaaaa, 0x55555555 };
  static const unsigned ops[] = { RV_ADD, RV_SUB, RV_AND };
  static const unsigned dests[] = { RV_ZERO, RV_A0, RV_A1, RV_A3, RV_T6 };
  struct rv32_emitter e;
  uint32_t (*fn)(uint32_t, uint32_t, uint32_t *) = (uint32_t (*)(uint32_t, uint32_t, uint32_t *))code;
  unsigned op, d, bridge, i, j, p;
  uint32_t output, seed = 0x31415926;
  CHECK(code && size >= 16384 && !((uintptr_t)code & 3) && publish);
  for (op = 0; op < 3; op++)
    for (d = 0; d < sizeof(dests) / sizeof(dests[0]); d++)
      for (bridge = 0; bridge < 2; bridge++) {
    rv32_init(&e, code, size);
    /* Production code enters through ndrc_rv32_enter(), which preserves the
     * private s11 emitter scratch. Keep this directly C-callable fixture ABI
     * compliant so optimized test_main() may also keep state in s11. */
    rv32_imm(&e, RV_ADD, RV_SP, RV_SP, -16);
    rv32_store(&e, RV_WORD, NDRC_RV32_AUX_REG, RV_SP, 0);
    if (ops[op] == RV_AND) rv32_flag_test(&e, RV_A0, RV_A1);
    else {
      rv32_flag_arithmetic(&e, ops[op], dests[d], RV_A0, RV_A1);
      if (dests[d] != RV_ZERO) rv32_store(&e, RV_WORD, dests[d], RV_A2, 0);
    }
    /* Flags must survive both changes to their original operands and C calls. */
    rv32_li(&e, RV_A0, 0x1234);
    rv32_li(&e, RV_A1, 0x5678);
    if (bridge) rv32_call_preserved(&e, (uint32_t)(uintptr_t)ndrc_rv32_test_clobber, 0);
    rv32_li(&e, RV_A0, 0);
    for (p = 0; p <= RV_P_LE; p++) {
      rv32_flag_predicate(&e, p, RV_A1);
      rv32_shift(&e, RV_SLL, RV_A1, RV_A1, p);
      rv32_alu(&e, RV_OR, RV_A0, RV_A0, RV_A1);
    }
    rv32_load(&e, RV_WORD, NDRC_RV32_AUX_REG, RV_SP, 0);
    rv32_imm(&e, RV_ADD, RV_SP, RV_SP, 16);
    rv32_jalr(&e, RV_ZERO, RV_RA, 0);
    CHECK(e.error == RV_OK && publish(code, e.count * 4) == 0);
    for (i = 0; i < 10; i++) for (j = 0; j < 10; j++) {
      uint32_t want = ops[op] == RV_ADD ? values[i] + values[j] : values[i] - values[j];
      output = 0xdeadbeef;
      CHECK(fn(values[i], values[j], &output) == expected(ops[op], values[i], values[j]));
      CHECK(output == (ops[op] != RV_AND && dests[d] != RV_ZERO ? want : 0xdeadbeef));
    }
    for (i = 0; i < 256; i++) {
      uint32_t a, b;
      seed = seed * 1664525u + 1013904223u; a = seed;
      seed = seed * 1664525u + 1013904223u; b = seed;
      CHECK(fn(a, b, &output) == expected(ops[op], a, b));
    }
  }
  output = 0x12345678;
  rv32_init(&e, &output, 4);
  rv32_flag_arithmetic(&e, RV_ADD, RV_A0, RV_A0, RV_A1);
  CHECK(e.error == RV_NO_SPACE && !e.count && output == 0x12345678);
  rv32_init(&e, &output, 4);
  rv32_flag_predicate(&e, RV_P_EQ, RV_T3);
  CHECK(e.error == RV_BAD_OPERAND && !e.count && output == 0x12345678);

  /* Match assem_rv32's conditional far form. Ari64 initially supplies a
   * label sentinel, represented here by address zero, then patches the LUI /
   * JALR pair. The short branch must retain the inverse condition. */
  rv32_init(&e, code, size);
  rv32_imm(&e, RV_ADD, RV_SP, RV_SP, -16);
  rv32_store(&e, RV_WORD, NDRC_RV32_AUX_REG, RV_SP, 0);
  rv32_flag_test(&e, RV_A0, RV_A0);
  rv32_flag_predicate(&e, RV_P_EQ, NDRC_RV32_AUX_REG);
  rv32_branch(&e, RV_EQ, NDRC_RV32_AUX_REG, RV_ZERO, 12);
  {
    size_t far_at = e.count;
    size_t equal_at;
    rv32_abs_jump(&e, RV_ZERO, NDRC_RV32_AUX_REG, 0);
    rv32_li(&e, RV_A0, 7);
    rv32_load(&e, RV_WORD, NDRC_RV32_AUX_REG, RV_SP, 0);
    rv32_imm(&e, RV_ADD, RV_SP, RV_SP, 16);
    rv32_jalr(&e, RV_ZERO, RV_RA, 0);
    equal_at = e.count;
    rv32_li(&e, RV_A0, 42);
    rv32_load(&e, RV_WORD, NDRC_RV32_AUX_REG, RV_SP, 0);
    rv32_imm(&e, RV_ADD, RV_SP, RV_SP, 16);
    rv32_jalr(&e, RV_ZERO, RV_RA, 0);
    rv32_patch_abs(&e, far_at,
      (uint32_t)(uintptr_t)((uint8_t *)code + equal_at * 4));
  }
  CHECK(e.error == RV_OK && publish(code, e.count * 4) == 0);
  CHECK(fn(0, 0, &output) == 42 && fn(1, 0, &output) == 7);
  return 0;
}
