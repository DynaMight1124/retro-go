/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "abi.h"
#include "selftest.h"

extern void ndrc_rv32_test_clobber(void);
extern uint32_t ndrc_rv32_test_enter(void *, void *, uint32_t *);
#define CHECK(x) do { if (!(x)) return __LINE__; } while (0)

int ndrc_rv32_abi_selftest(void *code, size_t size, int (*publish)(void *, size_t))
{
  static const uint32_t masks[] = { 0, 0x7ff, 0x555, 0x2aa, 1u << NDRC_RV32_CC_SLOT,
    (1u << NDRC_RV32_HOST_REGS) - 1 };
  static const uint32_t cycles_in[] = { 0, 0xffffffff, 0x80000000, 0x12345678 };
  struct rv32_emitter e;
  uint32_t local[28], cycles, seen = 0;
  unsigned i, j, k;
  CHECK(code && size >= 16384 && !((uintptr_t)code & 3) && publish);
  for (i = 0; i < NDRC_RV32_HOST_REGS; i++) {
    unsigned reg = rv32_host_reg(i);
    CHECK(reg < 32 && !(seen & (1u << reg)));
    seen |= 1u << reg;
    if (i < 8) CHECK(reg == RV_A0 + i);
  }
  CHECK(!(seen & ((1u << RV_ZERO) | (1u << RV_RA) | (1u << RV_SP) |
    (1u << RV_GP) | (1u << RV_TP) | (1u << RV_S0) |
    (1u << NDRC_RV32_AUX_REG) | (15u << RV_T3))));
  CHECK(rv32_host_reg(NDRC_RV32_CC_SLOT) == RV_S1);
  CHECK(NDRC_RV32_MEM_ADDR_SLOT >= NDRC_RV32_FIRST_SAVED_SLOT);
  CHECK(NDRC_RV32_MEM_VALUE_SLOT <= NDRC_RV32_LAST_SAVED_SLOT);
  CHECK(NDRC_RV32_MEM_ADDR_SLOT != NDRC_RV32_MEM_VALUE_SLOT);
  CHECK(rv32_mem_addr_reg() == RV_S2);
  CHECK(rv32_mem_value_reg() == RV_S3);
  CHECK(rv32_host_reg(NDRC_RV32_HOST_REGS) == 32);

  for (k = 0; k < sizeof(masks) / sizeof(masks[0]); k++) {
    rv32_init(&e, code, size);
    for (i = 0; i < NDRC_RV32_CC_SLOT; i++)
      rv32_li(&e, rv32_host_reg(i), 0x200 + i);
    rv32_store(&e, RV_WORD, RV_S1, RV_S0, 22 * 4);
    rv32_li(&e, RV_T3, 0x31); rv32_li(&e, RV_T4, 0x32); rv32_li(&e, RV_T5, 0x33);
    rv32_store(&e, RV_WORD, RV_SP, RV_S0, 26 * 4);
    rv32_call_preserved(&e, (uint32_t)(uintptr_t)ndrc_rv32_test_clobber, masks[k]);
    for (i = 0; i < NDRC_RV32_HOST_REGS; i++)
      rv32_store(&e, RV_WORD, rv32_host_reg(i), RV_S0, (int32_t)(i * 4));
    rv32_store(&e, RV_WORD, RV_T3, RV_S0, 23 * 4);
    rv32_store(&e, RV_WORD, RV_T4, RV_S0, 24 * 4);
    rv32_store(&e, RV_WORD, RV_T5, RV_S0, 25 * 4);
    rv32_store(&e, RV_WORD, RV_SP, RV_S0, 27 * 4);
    rv32_imm(&e, RV_ADD, RV_S1, RV_S1, -17);
    rv32_li(&e, RV_A0, 0x321);
    rv32_abs_jump(&e, RV_ZERO, RV_T6, (uint32_t)(uintptr_t)ndrc_rv32_leave);
    CHECK(e.error == RV_OK && publish(code, e.count * 4) == 0);
    for (j = 0; j < sizeof(cycles_in) / sizeof(cycles_in[0]); j++) {
      for (i = 0; i < 28; i++) local[i] = 0xdeadbeef;
      cycles = cycles_in[j];
      CHECK(ndrc_rv32_test_enter(code, local, &cycles) == 0);
      CHECK(cycles == cycles_in[j] - 17);
      for (i = 0; i < NDRC_RV32_CC_SLOT; i++) {
        uint32_t expected = i < 11 && !(masks[k] & (1u << i)) ? 0x600 + i : 0x200 + i;
        CHECK(local[i] == expected);
      }
      CHECK(local[NDRC_RV32_CC_SLOT] == cycles_in[j] &&
        local[22] == cycles_in[j]);
      CHECK(local[23] == 0x31 && local[24] == 0x32 && local[25] == 0x33);
      CHECK(local[26] == local[27] && !(local[26] & 15));
    }
  }

  /* A whole helper bridge must fail atomically if its stack-save sequence
   * would overflow the output buffer; no partially generated prologue. */
  local[0] = 0x12345678;
  rv32_init(&e, local, 4);
  rv32_call_preserved(&e, 0x40000000, 0x7ff);
  CHECK(e.error == RV_NO_SPACE && e.count == 0 && local[0] == 0x12345678);
  rv32_init(&e, local, sizeof(local));
  rv32_call_preserved(&e, 0x40000000, 1u << NDRC_RV32_HOST_REGS);
  CHECK(e.error == RV_BAD_OPERAND && e.count == 0 && local[0] == 0x12345678);
  return 0;
}
