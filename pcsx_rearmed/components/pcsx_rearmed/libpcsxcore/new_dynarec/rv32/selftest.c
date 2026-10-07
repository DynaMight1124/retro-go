/* SPDX-License-Identifier: GPL-2.0-or-later
 * Shared by the freestanding QEMU tests and optional ESP32-P4 boot diagnostic.
 * No emulator state or GNU Lightning functions are used here.
 */
#include "emit.h"
#include "selftest.h"

#if !defined(__riscv) || __riscv_xlen != 32
#error These execution tests require RV32
#endif

extern const uint32_t ndrc_rv32_oracle[], ndrc_rv32_oracle_end[];
#define CHECK(x) do { if (!(x)) return __LINE__; } while (0)
#define PUBLISH() do { CHECK(e.error == RV_OK); \
  CHECK(publish(code, e.count * 4) == 0); } while (0)
#define RET() rv32_jalr(&e, RV_ZERO, RV_RA, 0)

static void oracle_emit(struct rv32_emitter *e)
{
  unsigned op;
  for (op = 0; op < 16; op++) rv32_alu(e, op, RV_A0, RV_A1, RV_A2);
  rv32_alu(e, RV_SUB, RV_A0, RV_A1, RV_A2);
  rv32_alu(e, RV_SRA, RV_A0, RV_A1, RV_A2);
  rv32_imm(e, RV_ADD, RV_A0, RV_A1, -2048);
  rv32_imm(e, RV_ADD, RV_A0, RV_A1, 2047);
  rv32_imm(e, RV_SLT, RV_A0, RV_A1, -1);
  rv32_imm(e, RV_SLTU, RV_A0, RV_A1, -1);
  rv32_imm(e, RV_XOR, RV_A0, RV_A1, -1);
  rv32_imm(e, RV_OR, RV_A0, RV_A1, 42);
  rv32_imm(e, RV_AND, RV_A0, RV_A1, 255);
  rv32_shift(e, RV_SLL, RV_A0, RV_A1, 31);
  rv32_shift(e, RV_SRL, RV_A0, RV_A1, 0);
  rv32_shift(e, RV_SRA, RV_A0, RV_A1, 31);
  rv32_upper(e, RV_T6, 0x80000000, 0);
  rv32_upper(e, RV_T5, 0xfffff000, 1);
  rv32_load(e, RV_BYTE, RV_A0, RV_A1, -2048);
  rv32_load(e, RV_HALF, RV_A0, RV_A1, -2);
  rv32_load(e, RV_WORD, RV_A0, RV_A1, 2044);
  rv32_load(e, RV_BYTE_U, RV_A0, RV_A1, 2047);
  rv32_load(e, RV_HALF_U, RV_A0, RV_A1, 2046);
  rv32_store(e, RV_BYTE, RV_A0, RV_A1, -2048);
  rv32_store(e, RV_HALF, RV_A0, RV_A1, -2);
  rv32_store(e, RV_WORD, RV_A0, RV_A1, 2044);
  rv32_branch(e, RV_EQ, RV_A0, RV_A1, -4096);
  rv32_branch(e, RV_NE, RV_A0, RV_A1, 4092);
  rv32_branch(e, RV_LT, RV_A0, RV_A1, -4);
  rv32_branch(e, RV_GE, RV_A0, RV_A1, 4);
  rv32_branch(e, RV_LTU, RV_A0, RV_A1, -2048);
  rv32_branch(e, RV_GEU, RV_A0, RV_A1, 2048);
  rv32_jal(e, RV_RA, -1048576);
  rv32_jal(e, RV_ZERO, 1048572);
  rv32_jalr(e, RV_RA, RV_T6, -2048);
  rv32_jalr(e, RV_ZERO, RV_RA, 0);
  rv32_abs_jump(e, RV_RA, RV_T6, 0xfffff800);
}

static uint32_t reference(unsigned op, uint32_t a, uint32_t b)
{
  switch (op) {
  case RV_ADD: return a + b;
  case RV_SUB: return a - b;
  case RV_SLL: return a << (b & 31);
  case RV_SLT: return (int32_t)a < (int32_t)b;
  case RV_SLTU: return a < b;
  case RV_XOR: return a ^ b;
  case RV_SRL: return a >> (b & 31);
  case RV_SRA: return (uint32_t)((int32_t)a >> (b & 31));
  case RV_OR: return a | b;
  case RV_AND: return a & b;
  case RV_MUL: return a * b;
  case RV_MULH: return (uint32_t)((uint64_t)((int64_t)(int32_t)a * (int32_t)b) >> 32);
  case RV_MULHSU: return (uint32_t)((uint64_t)((int64_t)(int32_t)a * (int64_t)b) >> 32);
  case RV_MULHU: return (uint32_t)(((uint64_t)a * b) >> 32);
  case RV_DIV:
    if (!b) return UINT32_MAX;
    if (a == 0x80000000 && b == UINT32_MAX) return a;
    return (uint32_t)((int32_t)a / (int32_t)b);
  case RV_DIVU: return b ? a / b : UINT32_MAX;
  case RV_REM:
    if (!b) return a;
    if (a == 0x80000000 && b == UINT32_MAX) return 0;
    return (uint32_t)((int32_t)a % (int32_t)b);
  case RV_REMU: return b ? a % b : a;
  default: return 0;
  }
}

static int condition(unsigned cond, uint32_t a, uint32_t b)
{
  switch (cond) {
  case RV_EQ: return a == b;
  case RV_NE: return a != b;
  case RV_LT: return (int32_t)a < (int32_t)b;
  case RV_GE: return (int32_t)a >= (int32_t)b;
  case RV_LTU: return a < b;
  default: return a >= b;
  }
}

static __attribute__((noinline)) uint32_t call_helper(uint32_t a, uint32_t b)
{
  uintptr_t stack;
  __asm__ volatile ("mv %0, sp" : "=r"(stack));
  return (stack & 15) ? 0xbad : ((a ^ 0x87654321) + b);
}

int ndrc_rv32_selftest(void *code, size_t size, int (*publish)(void *, size_t))
{
  static const uint32_t values[] = { 0, 1, 31, 32, 0x7ff, 0x800, 0xfffff800,
    0x7fffffff, 0x80000000, 0x80000800, 0xffffffff, 0x12345678 };
  static const unsigned ops[] = { RV_ADD, RV_SUB, RV_SLL, RV_SLT, RV_SLTU,
    RV_XOR, RV_SRL, RV_SRA, RV_OR, RV_AND, RV_MUL, RV_MULH, RV_MULHSU,
    RV_MULHU, RV_DIV, RV_DIVU, RV_REM, RV_REMU };
  static const unsigned conds[] = { RV_EQ, RV_NE, RV_LT, RV_GE, RV_LTU, RV_GEU };
  struct rv32_emitter e;
  uint32_t (*fn)(uint32_t, uint32_t) = (uint32_t (*)(uint32_t, uint32_t))code;
  unsigned i, j, k;
  uint32_t guard[3] = { 0x12345678, 0x12345678, 0x12345678 };
  size_t patch;
  CHECK(code && size >= 16384 && !((uintptr_t)code & 3) && publish);

  /* Compare against actual GNU assembler output, not another C encoder. */
  rv32_init(&e, code, size);
  oracle_emit(&e);
  CHECK(e.error == RV_OK);
  CHECK(e.count * 4 == (uintptr_t)ndrc_rv32_oracle_end - (uintptr_t)ndrc_rv32_oracle);
  for (i = 0; i < e.count; i++) CHECK(e.code[i] == ndrc_rv32_oracle[i]);

  /* Every ALU operation, including destination/source aliasing and x0. */
  for (k = 0; k < sizeof(ops) / sizeof(ops[0]); k++) {
    rv32_init(&e, code, size);
    rv32_alu(&e, ops[k], RV_A0, RV_A0, RV_A1);
    RET(); PUBLISH();
    for (i = 0; i < sizeof(values) / sizeof(values[0]); i++)
      for (j = 0; j < sizeof(values) / sizeof(values[0]); j++)
        CHECK(fn(values[i], values[j]) == reference(ops[k], values[i], values[j]));
  }

  /* Constants near sign-extension/upper-immediate wrap boundaries. */
  for (i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
    rv32_init(&e, code, size);
    rv32_li(&e, RV_A0, values[i]); RET(); PUBLISH();
    CHECK(fn(0, 0) == values[i]);
  }
  rv32_init(&e, code, size);
  rv32_li(&e, RV_ZERO, UINT32_MAX);
  rv32_imm(&e, RV_ADD, RV_A0, RV_ZERO, 0); RET(); PUBLISH();
  CHECK(fn(7, 8) == 0);

  for (k = 0; k < sizeof(conds) / sizeof(conds[0]); k++) {
    rv32_init(&e, code, size);
    rv32_branch(&e, conds[k], RV_A0, RV_A1, 12);
    rv32_li(&e, RV_A0, 0); RET();
    rv32_li(&e, RV_A0, 1); RET(); PUBLISH();
    for (i = 0; i < sizeof(values) / sizeof(values[0]); i++)
      for (j = 0; j < sizeof(values) / sizeof(values[0]); j++)
        CHECK(fn(values[i], values[j]) == (uint32_t)condition(conds[k], values[i], values[j]));
  }

  /* Backward branches: sum 1..10 with an explicit zero loop counter exit. */
  rv32_init(&e, code, size);
  rv32_li(&e, RV_A1, 0);
  rv32_alu(&e, RV_ADD, RV_A1, RV_A1, RV_A0);
  rv32_imm(&e, RV_ADD, RV_A0, RV_A0, -1);
  rv32_branch(&e, RV_NE, RV_A0, RV_ZERO, -8);
  rv32_imm(&e, RV_ADD, RV_A0, RV_A1, 0); RET(); PUBLISH();
  CHECK(fn(10, 0) == 55);

  /* Short branch and JAL fixups, including preservation of branch operands. */
  rv32_init(&e, code, size);
  rv32_branch(&e, RV_NE, RV_A0, RV_A1, 0);
  rv32_li(&e, RV_A0, 7); RET();
  rv32_li(&e, RV_A0, 42); RET();
  rv32_patch_relative(&e, 0, 12); PUBLISH();
  CHECK(fn(1, 2) == 42 && fn(1, 1) == 7);
  rv32_init(&e, code, size);
  rv32_jal(&e, RV_ZERO, 0);
  rv32_li(&e, RV_A0, 7); RET();
  rv32_li(&e, RV_A0, 42); RET();
  rv32_patch_relative(&e, 0, 12); PUBLISH(); CHECK(fn(0, 0) == 42);

  /* Signed/unsigned loads and narrow stores preserve neighbouring bytes. */
  for (k = 0; k <= RV_HALF_U; k++) {
    uint32_t data = 0xa5a58081;
    uint32_t expected = k == RV_BYTE ? 0xffffff81 : k == RV_HALF ? 0xffff8081 :
      k == RV_WORD ? data : k == RV_BYTE_U ? 0x81 : 0x8081;
    if (k == 3) continue;
    rv32_init(&e, code, size);
    rv32_load(&e, k, RV_A0, RV_A0, 0); RET(); PUBLISH();
    CHECK(fn((uint32_t)(uintptr_t)&data, 0) == expected);
  }
  for (k = 0; k <= RV_WORD; k++) {
    uint32_t data = 0xa5a5a5a5;
    uint32_t expected = k == RV_BYTE ? 0xa5a5a578 : k == RV_HALF ? 0xa5a55678 : 0x12345678;
    rv32_init(&e, code, size);
    rv32_store(&e, k, RV_A1, RV_A0, 0); RET(); PUBLISH();
    fn((uint32_t)(uintptr_t)&data, 0x12345678);
    CHECK(data == expected);
  }

  /* Far conditional transfer (>4 KiB), both paths, then repatch its target. */
  rv32_init(&e, code, size);
  rv32_abs_branch(&e, RV_EQ, RV_A0, RV_A1, RV_T6, (uint32_t)(uintptr_t)code + 6144);
  rv32_li(&e, RV_A0, 7); RET();
  while (e.count < 1536) rv32_word(&e, 0x00000013);
  rv32_li(&e, RV_A0, 42); RET();
  patch = e.count;
  rv32_li(&e, RV_A0, 99); RET(); PUBLISH();
  CHECK(fn(1, 1) == 42 && fn(1, 2) == 7);
  rv32_patch_abs(&e, 1, (uint32_t)(uintptr_t)code + (uint32_t)patch * 4);
  PUBLISH(); CHECK(fn(1, 1) == 99 && fn(1, 2) == 7);

  /* JIT -> C -> JIT with 16-byte stack alignment and return-address save. */
  rv32_init(&e, code, size);
  rv32_imm(&e, RV_ADD, RV_SP, RV_SP, -16);
  rv32_store(&e, RV_WORD, RV_RA, RV_SP, 12);
  rv32_abs_jump(&e, RV_RA, RV_T6, (uint32_t)(uintptr_t)call_helper);
  rv32_load(&e, RV_WORD, RV_RA, RV_SP, 12);
  rv32_imm(&e, RV_ADD, RV_SP, RV_SP, 16); RET(); PUBLISH();
  CHECK(fn(0x12345678, 42) == ((0x12345678u ^ 0x87654321u) + 42));

  /* Release-build safety: no partial multiword emission or truncated offsets. */
  rv32_init(&e, &guard[1], 4);
  rv32_li(&e, RV_A0, 0x12345678);
  CHECK(e.error == RV_NO_SPACE && e.count == 0);
  CHECK(guard[0] == 0x12345678 && guard[1] == 0x12345678 && guard[2] == 0x12345678);
  rv32_init(&e, &guard[1], 4);
  rv32_imm(&e, RV_ADD, RV_A0, RV_A0, 2048);
  rv32_li(&e, RV_A0, 1); /* error must remain sticky */
  CHECK(e.error == RV_BAD_OPERAND && e.count == 0 && guard[1] == 0x12345678);
  rv32_init(&e, &guard[1], 4);
  rv32_branch(&e, RV_EQ, RV_A0, RV_A1, 4096);
  CHECK(e.error == RV_BAD_OPERAND && e.count == 0);
  rv32_init(&e, &guard[1], 4);
  rv32_jal(&e, RV_RA, -1048580);
  CHECK(e.error == RV_BAD_OPERAND && e.count == 0);
  rv32_init(&e, &guard[1], 4);
  rv32_shift(&e, RV_SLL, RV_A0, RV_A1, 32);
  CHECK(e.error == RV_BAD_OPERAND && e.count == 0);
  rv32_init(&e, &guard[1], 4);
  rv32_alu(&e, RV_ADD, 32, RV_A0, RV_A1);
  CHECK(e.error == RV_BAD_OPERAND && e.count == 0);
  rv32_init(&e, code, size);
  rv32_li(&e, RV_A0, 1); RET();
  rv32_patch_abs(&e, 0, 0);
  CHECK(e.error == RV_BAD_PATCH && e.code[0] == 0x00100513);
  rv32_init(&e, code, size);
  rv32_branch(&e, RV_EQ, RV_A0, RV_A1, 4);
  guard[0] = e.code[0];
  rv32_patch_relative(&e, 0, 4096);
  CHECK(e.error == RV_BAD_OPERAND && e.code[0] == guard[0]);
  return 0;
}
