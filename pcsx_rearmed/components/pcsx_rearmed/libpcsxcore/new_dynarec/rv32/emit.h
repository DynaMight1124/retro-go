/* SPDX-License-Identifier: GPL-2.0-or-later
 * RV32IM emission primitives for the experimental new_dynarec backend.
 * Register numbers here are hardware registers, NOT new_dynarec host slots.
 * Like gpSP's riscv_codegen.h, emit instructions directly without Lightning.
 */
#ifndef NDRC_RV32_EMIT_H
#define NDRC_RV32_EMIT_H

#include <stddef.h>
#include <stdint.h>

enum rv32_reg {
  RV_ZERO, RV_RA, RV_SP, RV_GP, RV_TP, RV_T0, RV_T1, RV_T2,
  RV_S0, RV_S1, RV_A0, RV_A1, RV_A2, RV_A3, RV_A4, RV_A5,
  RV_A6, RV_A7, RV_S2, RV_S3, RV_S4, RV_S5, RV_S6, RV_S7,
  RV_S8, RV_S9, RV_S10, RV_S11, RV_T3, RV_T4, RV_T5, RV_T6
};
enum rv32_alu {
  RV_ADD = 0, RV_SLL, RV_SLT, RV_SLTU, RV_XOR, RV_SRL, RV_OR, RV_AND,
  RV_SUB = 0x100, RV_SRA = 0x105,
  RV_MUL = 8, RV_MULH, RV_MULHSU, RV_MULHU,
  RV_DIV, RV_DIVU, RV_REM, RV_REMU
};
enum rv32_cond { RV_EQ = 0, RV_NE = 1, RV_LT = 4, RV_GE, RV_LTU, RV_GEU };
enum rv32_width { RV_BYTE = 0, RV_HALF, RV_WORD, RV_BYTE_U = 4, RV_HALF_U };
enum rv32_error { RV_OK, RV_BAD_OPERAND, RV_NO_SPACE, RV_BAD_PATCH };

struct rv32_emitter {
  uint32_t *code;
  size_t count, capacity; /* words */
  enum rv32_error error; /* sticky, including in NDEBUG builds */
};

static inline void rv32_init(struct rv32_emitter *e, void *code, size_t bytes)
{
  e->code = (uint32_t *)code;
  e->count = 0;
  e->capacity = bytes / 4;
  e->error = (!code || ((uintptr_t)code & 3) || (bytes & 3))
    ? RV_BAD_OPERAND : RV_OK;
}

static inline int rv32_check(struct rv32_emitter *e, int valid)
{
  if (!valid && e->error == RV_OK)
    e->error = RV_BAD_OPERAND;
  return e->error == RV_OK;
}

static inline int rv32_reserve(struct rv32_emitter *e, size_t words)
{
  if (e->error == RV_OK && words > e->capacity - e->count)
    e->error = RV_NO_SPACE;
  return e->error == RV_OK;
}

static inline void rv32_word(struct rv32_emitter *e, uint32_t insn)
{
  if (rv32_reserve(e, 1)) e->code[e->count++] = insn;
}

/* Encode only after validation. Unsigned shifts avoid signed-overflow UB. */
static inline uint32_t rv32_i(unsigned op, unsigned rd, unsigned f3,
                              unsigned rs, int32_t imm)
{
  return ((uint32_t)imm & 0xfff) << 20 | rs << 15 | f3 << 12 | rd << 7 | op;
}

static inline uint32_t rv32_b(unsigned cond, unsigned rs1, unsigned rs2, int32_t off)
{
  uint32_t u = (uint32_t)off;
  return (u & 0x1000) << 19 | (u & 0x7e0) << 20 | rs2 << 20 | rs1 << 15 |
    cond << 12 | (u & 0x1e) << 7 | (u & 0x800) >> 4 | 0x63;
}

static inline int rv32_cond_valid(unsigned cond)
{
  return cond == RV_EQ || cond == RV_NE || (cond >= RV_LT && cond <= RV_GEU);
}

static inline void rv32_alu(struct rv32_emitter *e, unsigned op,
                            unsigned rd, unsigned rs1, unsigned rs2)
{
  if (rv32_check(e, rd < 32 && rs1 < 32 && rs2 < 32 &&
      (op < 16 || op == RV_SUB || op == RV_SRA)))
    rv32_word(e, (op >> 3) << 25 | rs2 << 20 | rs1 << 15 |
      (op & 7) << 12 | rd << 7 | 0x33);
}

/* Shift immediates use rv32_shift(), not this signed-12-bit API. */
static inline void rv32_imm(struct rv32_emitter *e, unsigned op,
                            unsigned rd, unsigned rs, int32_t imm)
{
  if (rv32_check(e, rd < 32 && rs < 32 && imm >= -2048 && imm <= 2047 &&
      (op == RV_ADD || op == RV_SLT || op == RV_SLTU ||
       op == RV_XOR || op == RV_OR || op == RV_AND)))
    rv32_word(e, rv32_i(0x13, rd, op, rs, imm));
}

static inline void rv32_shift(struct rv32_emitter *e, unsigned op,
                              unsigned rd, unsigned rs, unsigned shift)
{
  if (rv32_check(e, rd < 32 && rs < 32 && shift < 32 &&
      (op == RV_SLL || op == RV_SRL || op == RV_SRA)))
    rv32_word(e, rv32_i(0x13, rd, op & 7, rs,
      (int32_t)(shift | (op == RV_SRA ? 0x400 : 0))));
}

static inline void rv32_upper(struct rv32_emitter *e, unsigned rd,
                              uint32_t value, int pc_relative)
{
  if (rv32_check(e, rd < 32 && !(value & 0xfff)))
    rv32_word(e, value | rd << 7 | (pc_relative ? 0x17 : 0x37));
}

static inline void rv32_load(struct rv32_emitter *e, unsigned width,
                             unsigned rd, unsigned base, int32_t offset)
{
  if (rv32_check(e, rd < 32 && base < 32 && offset >= -2048 && offset <= 2047 &&
      (width <= RV_WORD || width == RV_BYTE_U || width == RV_HALF_U)))
    rv32_word(e, rv32_i(3, rd, width, base, offset));
}

static inline void rv32_store(struct rv32_emitter *e, unsigned width,
                              unsigned src, unsigned base, int32_t offset)
{
  if (rv32_check(e, src < 32 && base < 32 && offset >= -2048 && offset <= 2047 &&
      width <= RV_WORD)) {
    uint32_t u = (uint32_t)offset;
    rv32_word(e, (u & 0xfe0) << 20 | src << 20 | base << 15 |
      width << 12 | (u & 31) << 7 | 0x23);
  }
}

/* Offsets are relative to the address of the branch, not the next instruction.
 * This first backend emits only 32-bit code: reject halfword-only targets.
 */
static inline void rv32_branch(struct rv32_emitter *e, unsigned cond,
                               unsigned rs1, unsigned rs2, int32_t offset)
{
  if (rv32_check(e, rs1 < 32 && rs2 < 32 && rv32_cond_valid(cond) &&
      offset >= -4096 && offset <= 4092 && !((uint32_t)offset & 3)))
    rv32_word(e, rv32_b(cond, rs1, rs2, offset));
}

static inline void rv32_jal(struct rv32_emitter *e, unsigned link, int32_t offset)
{
  if (rv32_check(e, link < 32 && offset >= -1048576 && offset <= 1048572 &&
      !((uint32_t)offset & 3))) {
    uint32_t u = (uint32_t)offset;
    rv32_word(e, (u & 0x100000) << 11 | (u & 0x7fe) << 20 |
      (u & 0x800) << 9 | (u & 0xff000) | link << 7 | 0x6f);
  }
}

/* JALR can target compiled C code containing compressed instructions. */
static inline void rv32_jalr(struct rv32_emitter *e, unsigned link,
                             unsigned base, int32_t offset)
{
  if (rv32_check(e, link < 32 && base < 32 && offset >= -2048 && offset <= 2047))
    rv32_word(e, rv32_i(0x67, link, 0, base, offset));
}

static inline int32_t rv32_low12(uint32_t value)
{
  return (int32_t)(value & 0x7ff) - (int32_t)(value & 0x800);
}

static inline void rv32_li(struct rv32_emitter *e, unsigned rd, uint32_t value)
{
  int32_t lo = rv32_low12(value);
  uint32_t hi = value - (uint32_t)lo;
  if (!rv32_check(e, rd < 32) || !rv32_reserve(e, hi && lo ? 2 : 1)) return;
  if (hi) rv32_upper(e, rd, hi, 0);
  if (lo || !hi) rv32_imm(e, RV_ADD, rd, hi ? rd : RV_ZERO, lo);
}

/* Fixed-size full-address transfers: valid across the entire RV32 address
 * space, including PSRAM -> flash C calls. Scratch is explicitly clobbered.
 * Keep this pair fixed-size so a later block-link patch cannot move code.
 */
static inline void rv32_abs_jump(struct rv32_emitter *e, unsigned link,
                                 unsigned scratch, uint32_t target)
{
  int32_t lo = rv32_low12(target);
  if (!rv32_check(e, link < 32 && scratch > 0 && scratch < 32 && !(target & 1)) ||
      !rv32_reserve(e, 2)) return;
  rv32_upper(e, scratch, target - (uint32_t)lo, 0);
  rv32_jalr(e, link, scratch, lo);
}

/* Inverted short branch over a full-address jump: no 4 KiB range assumption. */
static inline void rv32_abs_branch(struct rv32_emitter *e, unsigned cond,
                                   unsigned rs1, unsigned rs2, unsigned scratch,
                                   uint32_t target)
{
  if (!rv32_check(e, rv32_cond_valid(cond) && rs1 < 32 && rs2 < 32 &&
      scratch > 0 && scratch < 32 && !(target & 1)) || !rv32_reserve(e, 3)) return;
  rv32_branch(e, cond ^ 1, rs1, rs2, 12);
  rv32_abs_jump(e, RV_ZERO, scratch, target);
}

/* Patch a branch/JAL displacement without changing its size or operands.
 * As with rv32_patch_abs, execution must be quiescent and caches republished.
 */
static inline void rv32_patch_relative(struct rv32_emitter *e, size_t at, int32_t offset)
{
  struct rv32_emitter tmp;
  uint32_t word, patched = 0;
  if (e->error != RV_OK) return;
  if (at >= e->count) { e->error = RV_BAD_PATCH; return; }
  word = e->code[at];
  rv32_init(&tmp, &patched, sizeof(patched));
  if ((word & 0x7f) == 0x63)
    rv32_branch(&tmp, (word >> 12) & 7, (word >> 15) & 31, (word >> 20) & 31, offset);
  else if ((word & 0x7f) == 0x6f)
    rv32_jal(&tmp, (word >> 7) & 31, offset);
  else { e->error = RV_BAD_PATCH; return; }
  if (tmp.error != RV_OK) { e->error = tmp.error; return; }
  e->code[at] = patched;
}

/* Patch only unpublished/non-executing code. Caller must synchronize the code
 * cache afterwards. Multiword patching is NOT safe against concurrent execution.
 */
static inline void rv32_patch_abs(struct rv32_emitter *e, size_t at, uint32_t target)
{
  uint32_t a, b;
  unsigned scratch;
  int32_t lo;
  if (e->error != RV_OK) return;
  if (at >= e->count || e->count - at < 2 || (target & 1)) {
    e->error = RV_BAD_PATCH;
    return;
  }
  a = e->code[at]; b = e->code[at + 1];
  scratch = (a >> 7) & 31;
  if ((a & 0x7f) != 0x37 || (b & 0x707f) != 0x67 ||
      scratch == 0 || ((b >> 15) & 31) != scratch) {
    e->error = RV_BAD_PATCH;
    return;
  }
  lo = rv32_low12(target);
  e->code[at] = (target - (uint32_t)lo) | (a & 0xfff);
  e->code[at + 1] = ((uint32_t)lo & 0xfff) << 20 | (b & 0xfffff);
}

#endif
