/* SPDX-License-Identifier: GPL-2.0-or-later
 * Initial RV32 host backend for the complete Ari64 new_dynarec pipeline.
 *
 * Keep correctness first: control transfers use fixed-size absolute sequences
 * and condition state uses the tested software NZCV representation. Once the
 * full core executes, hot local transfers can be relaxed to JAL/B-type forms.
 */
#include "rv32/emit.h"
#include "rv32/flags.h"

static void emit_readword_indexed(int offset, int rs, int rt);

static unsigned rv32_hr(unsigned slot)
{
  if (slot == HOST_TEMPREG) return RV_T6;
  if (slot == FP) return RV_S0;
  assert(slot < HOST_REGS);
  return rv32_host_reg(slot);
}

static void rv32_emit_done(struct rv32_emitter *e)
{
  assert(e->error == RV_OK);
  out += e->count * 4;
}

#define RV32_EMIT_BEGIN(words) \
  struct rv32_emitter e; \
  rv32_init(&e, NDRC_WRITE_OFFSET(out), (words) * 4u)

static void output_w32(u_int word)
{
  *((u_int *)NDRC_WRITE_OFFSET(out)) = word;
  out += 4;
}

static void alloc_rv32_reg(struct regstat *cur, int i, signed char reg, int hr)
{
  int dirty = 0;
  (void)i;
  for (int n = 0; n < HOST_REGS; n++) {
    if (cur->regmap[n] == reg) {
      dirty = (cur->dirty >> n) & 1;
      cur->regmap[n] = -1;
    }
  }
  cur->regmap[hr] = reg;
  cur->dirty &= ~(1u << hr);
  cur->dirty |= (u_int)dirty << hr;
  cur->isconst &= ~(1u << hr);
}

static void alloc_cc(struct regstat *cur, int i)
{
  alloc_rv32_reg(cur, i, CCREG, HOST_CCREG);
}

static void alloc_cc_optional(struct regstat *cur, int i)
{
  if (cur->regmap[HOST_CCREG] < 0) {
    alloc_rv32_reg(cur, i, CCREG, HOST_CCREG);
    cur->noevict &= ~(1u << HOST_CCREG);
  }
}

static void set_jump_target(void *addr, void *target)
{
  uint32_t *p = NDRC_WRITE_OFFSET(addr);
  struct rv32_emitter e;
  intptr_t relative;
  uint32_t word;
  int found = 0;

  /* Software predicates precede their patchable branch. Locate that branch
   * while keeping the public patch address at the start of the sequence. */
  for (unsigned i = 0; i < 8; i++, p++) {
    if ((p[0] & 0x7f) == 0x63 || (p[0] & 0x7f) == 0x6f ||
        ((p[0] & 0x7f) == 0x37 && (p[1] & 0x707f) == 0x67)) {
      found = 1;
      break;
    }
  }
  assert(found);
  relative = (u_char *)target - (u_char *)((u_char *)addr +
    ((u_char *)p - (u_char *)NDRC_WRITE_OFFSET(addr)));
  word = p[0];

  /* A conditional far jump is an inverted branch over LUI/JALR. Patch the
   * absolute pair, never the local skip branch, even when target is nearby. */
  if ((p[0] & 0x7f) == 0x63 && (p[1] & 0x7f) == 0x37 &&
      (p[2] & 0x707f) == 0x67) {
    rv32_init(&e, p + 1, 8);
    e.count = 2;
    rv32_patch_abs(&e, 0, (uint32_t)(uintptr_t)target);
  }
  else if ((word & 0x7f) == 0x63 && relative >= -4096 && relative <= 4092) {
    rv32_init(&e, p, 4);
    rv32_branch(&e, (word >> 12) & 7, (word >> 15) & 31,
      (word >> 20) & 31, (int32_t)relative);
  }
  else if ((word & 0x7f) == 0x6f && relative >= -1048576 &&
           relative <= 1048572) {
    rv32_init(&e, p, 4);
    rv32_jal(&e, (word >> 7) & 31, (int32_t)relative);
  }
  else if ((p[0] & 0x7f) == 0x37 && (p[1] & 0x707f) == 0x67) {
    rv32_init(&e, p, 8);
    e.count = 2;
    rv32_patch_abs(&e, 0, (uint32_t)(uintptr_t)target);
  }
  else
    assert(0);
  assert(e.error == RV_OK);
}

static void set_jump_target_far1(u_int *addr, void *target)
{
  set_jump_target(addr, target);
}

static int can_jump_or_call(const void *target)
{
  (void)target;
  return 1;
}

static void emit_mov(int rs, int rt)
{
  RV32_EMIT_BEGIN(1);
  rv32_imm(&e, RV_ADD, rv32_hr(rt), rv32_hr(rs), 0);
  rv32_emit_done(&e);
}

static void emit_movimm(u_int imm, int rt)
{
  RV32_EMIT_BEGIN(2);
  rv32_li(&e, rv32_hr(rt), imm);
  rv32_emit_done(&e);
}

static void emit_zeroreg(int rt) { emit_movimm(0, rt); }
static void emit_not(int rs, int rt)
{
  RV32_EMIT_BEGIN(1);
  rv32_imm(&e, RV_XOR, rv32_hr(rt), rv32_hr(rs), -1);
  rv32_emit_done(&e);
}
static void emit_neg(int rs, int rt)
{
  RV32_EMIT_BEGIN(1);
  rv32_alu(&e, RV_SUB, rv32_hr(rt), RV_ZERO, rv32_hr(rs));
  rv32_emit_done(&e);
}

#define RV32_BIN(name, op) \
  static void name(int rs1, int rs2, int rt) { \
    RV32_EMIT_BEGIN(1); \
    rv32_alu(&e, op, rv32_hr(rt), rv32_hr(rs1), rv32_hr(rs2)); \
    rv32_emit_done(&e); \
  }
RV32_BIN(emit_add, RV_ADD)
RV32_BIN(emit_sub, RV_SUB)
RV32_BIN(emit_and, RV_AND)
RV32_BIN(emit_or, RV_OR)
RV32_BIN(emit_xor, RV_XOR)
RV32_BIN(emit_shl, RV_SLL)
RV32_BIN(emit_shr, RV_SRL)
RV32_BIN(emit_sar, RV_SRA)
RV32_BIN(emit_set_if_less32, RV_SLT)
RV32_BIN(emit_set_if_carry32, RV_SLTU)
#undef RV32_BIN

static void emit_imm(unsigned op, int rs, int32_t imm, int rt)
{
  RV32_EMIT_BEGIN(3);
  if (imm >= -2048 && imm <= 2047)
    rv32_imm(&e, op, rv32_hr(rt), rv32_hr(rs), imm);
  else {
    rv32_li(&e, NDRC_RV32_AUX_REG, (uint32_t)imm);
    rv32_alu(&e, op, rv32_hr(rt), rv32_hr(rs), NDRC_RV32_AUX_REG);
  }
  rv32_emit_done(&e);
}
static void emit_addimm(int rs, int imm, int rt) { emit_imm(RV_ADD, rs, imm, rt); }
static void emit_addimm_ptr(int rs, uintptr_t imm, int rt)
{ emit_imm(RV_ADD, rs, (int32_t)imm, rt); }
static void emit_andimm(int rs, int imm, int rt) { emit_imm(RV_AND, rs, imm, rt); }
static void emit_orimm(int rs, int imm, int rt) { emit_imm(RV_OR, rs, imm, rt); }
static void emit_xorimm(int rs, int imm, int rt) { emit_imm(RV_XOR, rs, imm, rt); }
static void emit_slti32(int rs, int imm, int rt) { emit_imm(RV_SLT, rs, imm, rt); }
static void emit_sltiu32(int rs, int imm, int rt) { emit_imm(RV_SLTU, rs, imm, rt); }

static void emit_addimm_and_set_flags3(int rs, int imm, int rt)
{
  RV32_EMIT_BEGIN(14);
  rv32_li(&e, NDRC_RV32_AUX_REG, (uint32_t)imm);
  rv32_flag_arithmetic(&e, RV_ADD, rv32_hr(rt), rv32_hr(rs),
    NDRC_RV32_AUX_REG);
  rv32_emit_done(&e);
}

static void emit_addimm_and_set_flags(int imm, int rt)
{
  emit_addimm_and_set_flags3(rt, imm, rt);
}

#define RV32_SHIFT_IMM(name, op) \
  static void name(int rs, u_int shift, int rt) { \
    RV32_EMIT_BEGIN(1); \
    rv32_shift(&e, op, rv32_hr(rt), rv32_hr(rs), shift); \
    rv32_emit_done(&e); \
  }
RV32_SHIFT_IMM(emit_shlimm, RV_SLL)
RV32_SHIFT_IMM(emit_shrimm, RV_SRL)
RV32_SHIFT_IMM(emit_sarimm, RV_SRA)
#undef RV32_SHIFT_IMM

static void emit_rorimm(int rs, u_int shift, int rt)
{
  shift &= 31;
  if (!shift) {
    if (rs != rt) emit_mov(rs, rt);
    return;
  }
  RV32_EMIT_BEGIN(3);
  rv32_shift(&e, RV_SRL, NDRC_RV32_AUX_REG, rv32_hr(rs), shift);
  rv32_shift(&e, RV_SLL, rv32_hr(rt), rv32_hr(rs), 32 - shift);
  rv32_alu(&e, RV_OR, rv32_hr(rt), rv32_hr(rt), NDRC_RV32_AUX_REG);
  rv32_emit_done(&e);
}

static void emit_orrshr_imm(int rs, u_int shift, int rt)
{
  RV32_EMIT_BEGIN(2);
  rv32_shift(&e, RV_SRL, NDRC_RV32_AUX_REG, rv32_hr(rs), shift & 31);
  rv32_alu(&e, RV_OR, rv32_hr(rt), rv32_hr(rt), NDRC_RV32_AUX_REG);
  rv32_emit_done(&e);
}

static void emit_signextend16(int rs, int rt)
{
  emit_shlimm(rs, 16, rt);
  emit_sarimm(rt, 16, rt);
}

static void emit_signextend8(int rs, int rt)
{
  emit_shlimm(rs, 24, rt);
  emit_sarimm(rt, 24, rt);
}

static void emit_xorsar_imm(int rs1, int rs2, u_int shift, int rt)
{
  RV32_EMIT_BEGIN(2);
  rv32_shift(&e, RV_SRA, NDRC_RV32_AUX_REG, rv32_hr(rs2), shift & 31);
  rv32_alu(&e, RV_XOR, rv32_hr(rt), rv32_hr(rs1), NDRC_RV32_AUX_REG);
  rv32_emit_done(&e);
}

/* RV32IM has no CLZ. This binary-search lowering is only used for GTE LZCR;
 * it keeps the hot integer paths free of helper calls. */
static void emit_clz(int rs, int rt)
{
  RV32_EMIT_BEGIN(24);
  rv32_imm(&e, RV_ADD, NDRC_RV32_AUX_REG, rv32_hr(rs), 0);
  rv32_imm(&e, RV_ADD, rv32_hr(rt), RV_ZERO, 0);
  rv32_branch(&e, RV_NE, NDRC_RV32_AUX_REG, RV_ZERO, 12);
  rv32_imm(&e, RV_ADD, rv32_hr(rt), RV_ZERO, 32);
  rv32_jal(&e, RV_ZERO, 80);
  rv32_shift(&e, RV_SRL, RV_RA, NDRC_RV32_AUX_REG, 16);
  rv32_branch(&e, RV_NE, RV_RA, RV_ZERO, 12);
  rv32_imm(&e, RV_ADD, rv32_hr(rt), rv32_hr(rt), 16);
  rv32_shift(&e, RV_SLL, NDRC_RV32_AUX_REG, NDRC_RV32_AUX_REG, 16);
  rv32_shift(&e, RV_SRL, RV_RA, NDRC_RV32_AUX_REG, 24);
  rv32_branch(&e, RV_NE, RV_RA, RV_ZERO, 12);
  rv32_imm(&e, RV_ADD, rv32_hr(rt), rv32_hr(rt), 8);
  rv32_shift(&e, RV_SLL, NDRC_RV32_AUX_REG, NDRC_RV32_AUX_REG, 8);
  rv32_shift(&e, RV_SRL, RV_RA, NDRC_RV32_AUX_REG, 28);
  rv32_branch(&e, RV_NE, RV_RA, RV_ZERO, 12);
  rv32_imm(&e, RV_ADD, rv32_hr(rt), rv32_hr(rt), 4);
  rv32_shift(&e, RV_SLL, NDRC_RV32_AUX_REG, NDRC_RV32_AUX_REG, 4);
  rv32_shift(&e, RV_SRL, RV_RA, NDRC_RV32_AUX_REG, 30);
  rv32_branch(&e, RV_NE, RV_RA, RV_ZERO, 12);
  rv32_imm(&e, RV_ADD, rv32_hr(rt), rv32_hr(rt), 2);
  rv32_shift(&e, RV_SLL, NDRC_RV32_AUX_REG, NDRC_RV32_AUX_REG, 2);
  rv32_shift(&e, RV_SRL, RV_RA, NDRC_RV32_AUX_REG, 31);
  rv32_branch(&e, RV_NE, RV_RA, RV_ZERO, 8);
  rv32_imm(&e, RV_ADD, rv32_hr(rt), rv32_hr(rt), 1);
  rv32_emit_done(&e);
}

static void emit_flag_arithmetic(unsigned op, int rs1, int rs2, int rt)
{
  RV32_EMIT_BEGIN(12);
  rv32_flag_arithmetic(&e, op, rv32_hr(rt), rv32_hr(rs1), rv32_hr(rs2));
  rv32_emit_done(&e);
}
static void emit_adds(int rs1, int rs2, int rt)
{ emit_flag_arithmetic(RV_ADD, rs1, rs2, rt); }
static void emit_subs(int rs1, int rs2, int rt)
{ emit_flag_arithmetic(RV_SUB, rs1, rs2, rt); }
static void emit_negs(int rs, int rt)
{
  RV32_EMIT_BEGIN(12);
  rv32_flag_arithmetic(&e, RV_SUB, rv32_hr(rt), RV_ZERO, rv32_hr(rs));
  rv32_emit_done(&e);
}

static void emit_cmp(int rs, int rt)
{
  RV32_EMIT_BEGIN(12);
  rv32_flag_arithmetic(&e, RV_SUB, RV_ZERO, rv32_hr(rs), rv32_hr(rt));
  rv32_emit_done(&e);
}
static void emit_cmpimm(int rs, int imm)
{
  RV32_EMIT_BEGIN(14);
  rv32_li(&e, NDRC_RV32_AUX_REG, (uint32_t)imm);
  rv32_flag_arithmetic(&e, RV_SUB, RV_ZERO, rv32_hr(rs),
    NDRC_RV32_AUX_REG);
  rv32_emit_done(&e);
}
static void emit_cmpcs(int rs, int rt) { emit_cmp(rs, rt); }
static void emit_test(int rs, int rt)
{
  RV32_EMIT_BEGIN(3);
  rv32_flag_test(&e, rv32_hr(rs), rv32_hr(rt));
  rv32_emit_done(&e);
}
static void emit_testimm(int rs, int imm)
{
  RV32_EMIT_BEGIN(5);
  rv32_li(&e, NDRC_RV32_AUX_REG, (uint32_t)imm);
  rv32_flag_test(&e, rv32_hr(rs), NDRC_RV32_AUX_REG);
  rv32_emit_done(&e);
}

static void emit_set_gz32(int rs, int rt)
{
  RV32_EMIT_BEGIN(1);
  rv32_alu(&e, RV_SLT, rv32_hr(rt), RV_ZERO, rv32_hr(rs));
  rv32_emit_done(&e);
}
static void emit_set_nz32(int rs, int rt)
{
  RV32_EMIT_BEGIN(1);
  rv32_alu(&e, RV_SLTU, rv32_hr(rt), RV_ZERO, rv32_hr(rs));
  rv32_emit_done(&e);
}

static void emit_abs_jump(const void *target, unsigned link)
{
  RV32_EMIT_BEGIN(2);
  rv32_abs_jump(&e, link, NDRC_RV32_AUX_REG,
    (uint32_t)(uintptr_t)target);
  rv32_emit_done(&e);
}
static void emit_call(const void *target) { emit_abs_jump(target, RV_RA); }
static void emit_jmp(const void *target) { emit_abs_jump(target, RV_ZERO); }
static void emit_jmpreg(u_int r)
{
  RV32_EMIT_BEGIN(1);
  rv32_jalr(&e, RV_ZERO, rv32_hr(r), 0);
  rv32_emit_done(&e);
}
static void emit_ret(void)
{
  RV32_EMIT_BEGIN(1);
  rv32_jalr(&e, RV_ZERO, RV_RA, 0);
  rv32_emit_done(&e);
}

static void emit_cond_jump(unsigned predicate, const void *target)
{
  RV32_EMIT_BEGIN(8);
  rv32_flag_predicate(&e, predicate, NDRC_RV32_AUX_REG);
  rv32_branch(&e, RV_EQ, NDRC_RV32_AUX_REG, RV_ZERO, 12);
  /* DJT_1/DJT_2 are Ari64 label sentinels, not executable addresses. Emit a
   * valid placeholder pair so set_jump_target() can patch the far leg while
   * preserving this inverted local skip branch. Without the pair, release
   * builds silently retained and repatched the inverse condition. */
  rv32_abs_jump(&e, RV_ZERO, NDRC_RV32_AUX_REG,
    (uintptr_t)target < 3 ? 0 : (uint32_t)(uintptr_t)target);
  rv32_emit_done(&e);
}
#define RV32_CJ(name, pred) \
  static void name(const void *target) { emit_cond_jump(pred, target); }
RV32_CJ(emit_jeq, RV_P_EQ)
RV32_CJ(emit_jne, RV_P_NE)
RV32_CJ(emit_js, RV_P_MI)
RV32_CJ(emit_jns, RV_P_PL)
RV32_CJ(emit_jl, RV_P_LT)
RV32_CJ(emit_jge, RV_P_GE)
RV32_CJ(emit_jo, RV_P_VS)
RV32_CJ(emit_jno, RV_P_VC)
RV32_CJ(emit_jc, RV_P_CS)
#undef RV32_CJ

static void *emit_cbz(int r, const void *target)
{
  void *ret = out;
  RV32_EMIT_BEGIN(3);
  /* B-type branches only reach 4 KiB, while an invalidate stub may be emitted
   * much farther away. Keep a fixed-size, patchable far form. */
  rv32_branch(&e, RV_NE, rv32_hr(r), RV_ZERO, 12);
  rv32_abs_jump(&e, RV_ZERO, NDRC_RV32_AUX_REG,
    (uintptr_t)target < 3 ? 0 : (uint32_t)(uintptr_t)target);
  rv32_emit_done(&e);
  return ret;
}

static void emit_cmov_reg(unsigned predicate, int rs, int rt)
{
  RV32_EMIT_BEGIN(8);
  rv32_flag_predicate(&e, predicate, NDRC_RV32_AUX_REG);
  rv32_branch(&e, RV_EQ, NDRC_RV32_AUX_REG, RV_ZERO, 8);
  rv32_imm(&e, RV_ADD, rv32_hr(rt), rv32_hr(rs), 0);
  rv32_emit_done(&e);
}

static void emit_cmov_imm(unsigned predicate, int imm, int rt)
{
  assert(imm == 0 || imm == 1);
  RV32_EMIT_BEGIN(8);
  rv32_flag_predicate(&e, predicate, NDRC_RV32_AUX_REG);
  rv32_branch(&e, RV_EQ, NDRC_RV32_AUX_REG, RV_ZERO, 8);
  rv32_imm(&e, RV_ADD, rv32_hr(rt), RV_ZERO, imm);
  rv32_emit_done(&e);
}

static void emit_cmoveq_reg(int rs, int rt) { emit_cmov_reg(RV_P_EQ, rs, rt); }
static void emit_cmovne_reg(int rs, int rt) { emit_cmov_reg(RV_P_NE, rs, rt); }
static void emit_cmovl_reg(int rs, int rt) { emit_cmov_reg(RV_P_LT, rs, rt); }
static void emit_cmovb_reg(int rs, int rt) { emit_cmov_reg(RV_P_CC, rs, rt); }
static void emit_cmovs_reg(int rs, int rt) { emit_cmov_reg(RV_P_MI, rs, rt); }
static void emit_cmovne_imm(int imm, int rt) { emit_cmov_imm(RV_P_NE, imm, rt); }
static void emit_cmovl_imm(int imm, int rt) { emit_cmov_imm(RV_P_LT, imm, rt); }
static void emit_cmovb_imm(int imm, int rt) { emit_cmov_imm(RV_P_CC, imm, rt); }

static void emit_readword(void *addr, int rt)
{
  uintptr_t offset = (u_char *)addr - (u_char *)&dynarec_local;
  RV32_EMIT_BEGIN(1);
  assert(!(offset & 3) && offset <= 2047);
  rv32_load(&e, RV_WORD, rv32_hr(rt), rv32_hr(FP), (int32_t)offset);
  rv32_emit_done(&e);
}
#define emit_readptr emit_readword
static void emit_writeword(int rt, void *addr)
{
  uintptr_t offset = (u_char *)addr - (u_char *)&dynarec_local;
  RV32_EMIT_BEGIN(1);
  assert(!(offset & 3) && offset <= 2047);
  rv32_store(&e, RV_WORD, rv32_hr(rt), rv32_hr(FP), (int32_t)offset);
  rv32_emit_done(&e);
}

static void emit_loadreg(int r, int hr)
{
  void *addr;
  if (!r) { emit_zeroreg(hr); return; }
  switch (r) {
  case CCREG: addr = &cycle_count; break;
  case INVCP: emit_readword_indexed(LO_invc_ptr, FP, hr); return;
  case ROREG: emit_readword_indexed(LO_ram_offset, FP, hr); return;
  default: assert(r < 34); addr = &psxRegs.GPR.r[r]; break;
  }
  emit_readword(addr, hr);
}

static void emit_storereg(int r, int hr)
{
  void *addr;
  switch (r) {
  case CCREG: addr = &cycle_count; break;
  default: assert(r < 34); addr = &psxRegs.GPR.r[r]; break;
  }
  emit_writeword(hr, addr);
}

static void emit_readword_indexed(int offset, int rs, int rt)
{
  RV32_EMIT_BEGIN(4);
  if (offset >= -2048 && offset <= 2047)
    rv32_load(&e, RV_WORD, rv32_hr(rt), rv32_hr(rs), offset);
  else {
    rv32_li(&e, NDRC_RV32_AUX_REG, (uint32_t)offset);
    rv32_alu(&e, RV_ADD, NDRC_RV32_AUX_REG, rv32_hr(rs),
      NDRC_RV32_AUX_REG);
    rv32_load(&e, RV_WORD, rv32_hr(rt), NDRC_RV32_AUX_REG, 0);
  }
  rv32_emit_done(&e);
}

static void emit_mem_indexed(unsigned width, int base, int index, int rt, int store)
{
  RV32_EMIT_BEGIN(2);
  rv32_alu(&e, RV_ADD, NDRC_RV32_AUX_REG, rv32_hr(base), rv32_hr(index));
  if (store)
    rv32_store(&e, width, rv32_hr(rt), NDRC_RV32_AUX_REG, 0);
  else
    rv32_load(&e, width, rv32_hr(rt), NDRC_RV32_AUX_REG, 0);
  rv32_emit_done(&e);
}

static void emit_ldr_dualindexed(int b, int i, int rt)
{ emit_mem_indexed(RV_WORD, b, i, rt, 0); }
static void emit_ldrb_dualindexed(int b, int i, int rt)
{ emit_mem_indexed(RV_BYTE_U, b, i, rt, 0); }
static void emit_ldrsb_dualindexed(int b, int i, int rt)
{ emit_mem_indexed(RV_BYTE, b, i, rt, 0); }
static void emit_ldrh_dualindexed(int b, int i, int rt)
{ emit_mem_indexed(RV_HALF_U, b, i, rt, 0); }
static void emit_ldrsh_dualindexed(int b, int i, int rt)
{ emit_mem_indexed(RV_HALF, b, i, rt, 0); }
static void emit_str_dualindexed(int b, int i, int rt)
{ emit_mem_indexed(RV_WORD, b, i, rt, 1); }
static void emit_strh_dualindexed(int b, int i, int rt)
{ emit_mem_indexed(RV_HALF, b, i, rt, 1); }
static void emit_strb_dualindexed(int b, int i, int rt)
{ emit_mem_indexed(RV_BYTE, b, i, rt, 1); }

static void emit_readptr_dualindexedx_ptrlen(int base, int index, int rt)
{
  RV32_EMIT_BEGIN(3);
  rv32_shift(&e, RV_SLL, NDRC_RV32_AUX_REG, rv32_hr(index), 2);
  rv32_alu(&e, RV_ADD, NDRC_RV32_AUX_REG, rv32_hr(base),
    NDRC_RV32_AUX_REG);
  rv32_load(&e, RV_WORD, rv32_hr(rt), NDRC_RV32_AUX_REG, 0);
  rv32_emit_done(&e);
}
#define emit_adds_ptr emit_adds

static void emit_load_indexed(unsigned width, int offset, int rs, int rt)
{
  RV32_EMIT_BEGIN(4);
  if (offset >= -2048 && offset <= 2047)
    rv32_load(&e, width, rv32_hr(rt), rv32_hr(rs), offset);
  else {
    rv32_li(&e, NDRC_RV32_AUX_REG, (uint32_t)offset);
    rv32_alu(&e, RV_ADD, NDRC_RV32_AUX_REG, rv32_hr(rs),
      NDRC_RV32_AUX_REG);
    rv32_load(&e, width, rv32_hr(rt), NDRC_RV32_AUX_REG, 0);
  }
  rv32_emit_done(&e);
}

static void emit_movsbl_indexed(int o, int rs, int rt)
{ emit_load_indexed(RV_BYTE, o, rs, rt); }
static void emit_movzbl_indexed(int o, int rs, int rt)
{ emit_load_indexed(RV_BYTE_U, o, rs, rt); }
static void emit_movswl_indexed(int o, int rs, int rt)
{ emit_load_indexed(RV_HALF, o, rs, rt); }
static void emit_movzwl_indexed(int o, int rs, int rt)
{ emit_load_indexed(RV_HALF_U, o, rs, rt); }

static void emit_store_indexed(unsigned width, int rt, int offset, int rs)
{
  RV32_EMIT_BEGIN(4);
  if (offset >= -2048 && offset <= 2047)
    rv32_store(&e, width, rv32_hr(rt), rv32_hr(rs), offset);
  else {
    rv32_li(&e, NDRC_RV32_AUX_REG, (uint32_t)offset);
    rv32_alu(&e, RV_ADD, NDRC_RV32_AUX_REG, rv32_hr(rs),
      NDRC_RV32_AUX_REG);
    rv32_store(&e, width, rv32_hr(rt), NDRC_RV32_AUX_REG, 0);
  }
  rv32_emit_done(&e);
}

static void emit_writehword_indexed(int rt, int o, int rs)
{ emit_store_indexed(RV_HALF, rt, o, rs); }
static void emit_writebyte_indexed(int rt, int o, int rs)
{ emit_store_indexed(RV_BYTE, rt, o, rs); }

static void emit_writeword_indexed(int rt, int offset, int rs)
{
  RV32_EMIT_BEGIN(4);
  if (offset >= -2048 && offset <= 2047)
    rv32_store(&e, RV_WORD, rv32_hr(rt), rv32_hr(rs), offset);
  else {
    rv32_li(&e, NDRC_RV32_AUX_REG, (uint32_t)offset);
    rv32_alu(&e, RV_ADD, NDRC_RV32_AUX_REG, rv32_hr(rs),
      NDRC_RV32_AUX_REG);
    rv32_store(&e, RV_WORD, rv32_hr(rt), NDRC_RV32_AUX_REG, 0);
  }
  rv32_emit_done(&e);
}

static void emit_ldrb_indexedsr12_reg(int base, int rs, int rt)
{
  RV32_EMIT_BEGIN(3);
  rv32_shift(&e, RV_SRL, NDRC_RV32_AUX_REG, rv32_hr(rs), 12);
  rv32_alu(&e, RV_ADD, NDRC_RV32_AUX_REG, rv32_hr(base),
    NDRC_RV32_AUX_REG);
  rv32_load(&e, RV_BYTE_U, rv32_hr(rt), NDRC_RV32_AUX_REG, 0);
  rv32_emit_done(&e);
}

static void emit_bic_shift(int rs1, int rs2, int shift, int rt, int left)
{
  RV32_EMIT_BEGIN(3);
  /* `shift` is an allocator slot containing the runtime shift count, not an
   * immediate.  loadlr_assemble uses this to form the preservation mask for
   * LWL/LWR; encoding the slot number as an immediate drops bytes from the
   * first half of a same-register unaligned-load pair. */
  rv32_alu(&e, left ? RV_SLL : RV_SRL, NDRC_RV32_AUX_REG,
    rv32_hr(rs2), rv32_hr(shift));
  rv32_imm(&e, RV_XOR, NDRC_RV32_AUX_REG, NDRC_RV32_AUX_REG, -1);
  rv32_alu(&e, RV_AND, rv32_hr(rt), rv32_hr(rs1), NDRC_RV32_AUX_REG);
  rv32_emit_done(&e);
}

static void emit_bic_lsl(int a, int b, int s, int d)
{ emit_bic_shift(a, b, s, d, 1); }
static void emit_bic_lsr(int a, int b, int s, int d)
{ emit_bic_shift(a, b, s, d, 0); }

static void save_load_regs(int store, u_int reglist)
{
  unsigned off = 0;
  RV32_EMIT_BEGIN(32);
  rv32_imm(&e, RV_ADD, RV_SP, RV_SP, store ? -RV32_CALL_SAVE_FRAME : 0);
  for (unsigned slot = 0; slot < 11; slot++) {
    if (!(reglist & (1u << slot))) continue;
    if (store) rv32_store(&e, RV_WORD, rv32_hr(slot), RV_SP, off);
    else rv32_load(&e, RV_WORD, rv32_hr(slot), RV_SP, off);
    off += 4;
  }
  if (store) {
    rv32_store(&e, RV_WORD, RV_T3, RV_SP, 44);
    rv32_store(&e, RV_WORD, RV_T4, RV_SP, 48);
    rv32_store(&e, RV_WORD, RV_T5, RV_SP, 52);
    rv32_store(&e, RV_WORD, RV_RA, RV_SP, 56);
  }
  else {
    rv32_load(&e, RV_WORD, RV_T3, RV_SP, 44);
    rv32_load(&e, RV_WORD, RV_T4, RV_SP, 48);
    rv32_load(&e, RV_WORD, RV_T5, RV_SP, 52);
    rv32_load(&e, RV_WORD, RV_RA, RV_SP, 56);
    rv32_imm(&e, RV_ADD, RV_SP, RV_SP, RV32_CALL_SAVE_FRAME);
  }
  rv32_emit_done(&e);
}

static void save_regs(u_int reglist)
{ save_load_regs(1, reglist & CALLER_SAVE_REGS); }
static void restore_regs(u_int reglist)
{ save_load_regs(0, reglist & CALLER_SAVE_REGS); }

static void emit_movimm_from(u_int rs_val, int rs, u_int rt_val, int rt)
{
  int32_t diff = (int32_t)(rt_val - rs_val);
  if (diff >= -2048 && diff <= 2047)
    emit_addimm(rs, diff, rt);
  else if (rt_val == ~rs_val)
    emit_not(rs, rt);
  else
    emit_movimm(rt_val, rt);
}

static int is_similar_value(u_int a, u_int b)
{
  int32_t diff = (int32_t)(a - b);
  return (diff >= -2048 && diff <= 2047) || a == ~b;
}

static void loadstore_extend(enum stub_type type, int rs, int rt)
{
  switch (type) {
  case LOADB_STUB: emit_signextend8(rs, rt); break;
  case LOADBU_STUB:
  case STOREB_STUB: emit_andimm(rs, 0xff, rt); break;
  case LOADH_STUB: emit_signextend16(rs, rt); break;
  case LOADHU_STUB:
  case STOREH_STUB: emit_andimm(rs, 0xffff, rt); break;
  case LOADW_STUB:
  case STOREW_STUB: if (rs != rt) emit_mov(rs, rt); break;
  default: assert(0);
  }
}

#include "pcsxmem.h"

static void do_readstub(struct compile_state *st, int n)
{
  set_jump_target(stubs[n].addr, out);
  enum stub_type type = stubs[n].type;
  int i = stubs[n].a;
  int rs = stubs[n].b;
  const struct regstat *i_regs = (const void *)stubs[n].c;
  int adj = (int)stubs[n].d;
  u_int reglist = stubs[n].e;
  const signed char *i_regmap = i_regs->regmap;
  int rt = get_reg(i_regmap,
    dops[i].itype == C2LS || dops[i].itype == LOADLR ? FTEMP : dops[i].rt1);
  int temp = -1, temp2 = HOST_TEMPREG, regs_saved = 0;
  void *restore_jump = NULL, *handler_jump;
  assert(rs >= 0);
  reglist |= 1u << rs;
  /* The effective address is still live during the table lookup, including
   * loads whose destination aliases it. Match ARM: choose scratch before
   * removing the overwritten destination from the call-preservation mask. */
  for (int r = 0; r < HOST_CCREG; r++)
    if (r != EXCLUDE_REG && !(reglist & (1u << r))) { temp = r; break; }
  if (rt >= 0 && dops[i].rt1 != 0) reglist &= ~(1u << rt);
  if (temp < 0) {
    save_regs(reglist);
    regs_saved = 1;
    temp = rs == 0 ? 2 : 0;
  }
  if ((regs_saved || !(reglist & 2)) && temp != 1 && rs != 1) temp2 = 1;
  emit_readword(&mem_rtab, temp);
  emit_shrimm(rs, 12, temp2);
  emit_readptr_dualindexedx_ptrlen(temp, temp2, temp2);
  emit_adds(temp2, temp2, temp2);
  handler_jump = out;
  emit_jc(0);
  if (dops[i].itype == C2LS || (rt >= 0 && dops[i].rt1 != 0)) {
    switch (type) {
    case LOADB_STUB: emit_ldrsb_dualindexed(temp2, rs, rt); break;
    case LOADBU_STUB: emit_ldrb_dualindexed(temp2, rs, rt); break;
    case LOADH_STUB: emit_ldrsh_dualindexed(temp2, rs, rt); break;
    case LOADHU_STUB: emit_ldrh_dualindexed(temp2, rs, rt); break;
    case LOADW_STUB: emit_ldr_dualindexed(temp2, rs, rt); break;
    default: assert(0);
    }
  }
  if (regs_saved) { restore_jump = out; emit_jmp(0); }
  else emit_jmp(stubs[n].retaddr);
  set_jump_target(handler_jump, out);
  if (!regs_saved) save_regs(reglist);
  void *handler = type == LOADW_STUB ? (void *)jump_handler_read32 :
    (type == LOADH_STUB || type == LOADHU_STUB) ? (void *)jump_handler_read16 :
    (void *)jump_handler_read8;
  pass_args(rs, temp2);
  int cc = get_reg(i_regmap, CCREG);
  if (cc < 0) emit_loadreg(CCREG, 2);
  emit_addimm(cc < 0 ? 2 : cc, adj, 2);
  /* temp2 may be t6 and holds the handler table address until pass_args. */
  emit_far_call(handler);
  if (dops[i].itype == C2LS || (rt >= 0 && dops[i].rt1 != 0))
    loadstore_extend(type, 0, rt);
  if (restore_jump) set_jump_target(restore_jump, out);
  restore_regs(reglist);
  emit_jmp(stubs[n].retaddr);
}

static void inline_readstub(enum stub_type type, int i, u_int addr,
  u_int guest_pc,
  const signed char regmap[], int target, int adj, u_int reglist)
{
  (void)guest_pc;
  int ra = cinfo[i].addr;
  int rt = get_reg(regmap, target);
  uintptr_t host_addr = 0;
  void *handler = get_direct_memhandler(mem_rtab, addr, type, &host_addr);
  int cc = get_reg(regmap, CCREG);
  assert(ra >= 0);
  if (handler == NULL) {
    if (rt < 0 || dops[i].rt1 == 0) return;
    if (addr != host_addr) emit_movimm_from(addr, ra, host_addr, ra);
    switch (type) {
    case LOADB_STUB: emit_movsbl_indexed(0, ra, rt); break;
    case LOADBU_STUB: emit_movzbl_indexed(0, ra, rt); break;
    case LOADH_STUB: emit_movswl_indexed(0, ra, rt); break;
    case LOADHU_STUB: emit_movzwl_indexed(0, ra, rt); break;
    case LOADW_STUB: emit_readword_indexed(0, ra, rt); break;
    default: assert(0);
    }
    return;
  }
  int dynamic = pcsxmem_is_handler_dynamic(addr);
  if (dynamic)
    handler = type == LOADW_STUB ? (void *)jump_handler_read32 :
      (type == LOADH_STUB || type == LOADHU_STUB) ? (void *)jump_handler_read16 :
      (void *)jump_handler_read8;
  if (rt >= 0 && dops[i].rt1 != 0) reglist &= ~(1u << rt);
  save_regs(reglist);
  if (target == 0) emit_movimm(addr, 0);
  else if (ra != 0) emit_mov(ra, 0);
  if (cc < 0) emit_loadreg(CCREG, 2);
  if (dynamic) {
    emit_movimm(((u_int *)mem_rtab)[addr >> 12] << 1, 1);
    emit_addimm(cc < 0 ? 2 : cc, adj, 2);
  }
  else {
    emit_readword(&last_count, 3);
    emit_addimm(cc < 0 ? 2 : cc, adj, 2);
    emit_add(2, 3, 2);
    emit_writeword(2, &psxRegs.cycle);
  }
  emit_far_call(handler);
  if (rt >= 0 && dops[i].rt1 != 0) loadstore_extend(type, 0, rt);
  restore_regs(reglist);
}

static void do_writestub(struct compile_state *st, int n)
{
  set_jump_target(stubs[n].addr, out);
  enum stub_type type = stubs[n].type;
  int i = stubs[n].a, rs = stubs[n].b;
  const struct regstat *i_regs = (const void *)stubs[n].c;
  int adj = (int)stubs[n].d;
  u_int reglist = stubs[n].e;
  const signed char *i_regmap = i_regs->regmap;
  int rt = get_reg(i_regmap, dops[i].itype == C2LS ? FTEMP : dops[i].rs2);
  int temp = -1, temp2 = HOST_TEMPREG, regs_saved = 0;
  void *restore_jump = NULL, *handler_jump;
  assert(rs >= 0 && rt >= 0);
  u_int reglist2 = reglist | (1u << rs) | (1u << rt);
  for (int r = 0; r < HOST_CCREG; r++)
    if (r != EXCLUDE_REG && !(reglist2 & (1u << r))) { temp = r; break; }
  if (temp < 0) {
    save_regs(reglist);
    regs_saved = 1;
    for (int r = 0; r <= 3; r++) if (r != rs && r != rt) { temp = r; break; }
  }
  if ((regs_saved || !(reglist2 & 8)) && temp != 3 && rs != 3 && rt != 3) temp2 = 3;
  emit_readword(&mem_wtab, temp);
  emit_shrimm(rs, 12, temp2);
  emit_readptr_dualindexedx_ptrlen(temp, temp2, temp2);
  emit_adds(temp2, temp2, temp2);
  handler_jump = out;
  emit_jc(0);
  switch (type) {
  case STOREB_STUB: emit_strb_dualindexed(temp2, rs, rt); break;
  case STOREH_STUB: emit_strh_dualindexed(temp2, rs, rt); break;
  case STOREW_STUB: emit_str_dualindexed(temp2, rs, rt); break;
  default: assert(0);
  }
  if (regs_saved) { restore_jump = out; emit_jmp(0); }
  else emit_jmp(stubs[n].retaddr);
  set_jump_target(handler_jump, out);
  if (!regs_saved) save_regs(reglist);
  void *handler = type == STOREW_STUB ? (void *)jump_handler_write32 :
    type == STOREH_STUB ? (void *)jump_handler_write16 : (void *)jump_handler_write8;
  pass_args(rs, rt);
  if (temp2 != 3) emit_mov(temp2, 3);
  int cc = get_reg(i_regmap, CCREG);
  if (cc < 0) emit_loadreg(CCREG, 2);
  emit_addimm(cc < 0 ? 2 : cc, adj, 2);
  emit_far_call(handler);
  /* RV32 C slow paths return the possibly rescheduled relative cycle count
   * in a0. The argument copy in a2 is stale after a hardware handler. */
  emit_addimm(0, -adj, cc < 0 ? 2 : cc);
  if (cc < 0) emit_storereg(CCREG, 2);
  if (restore_jump) set_jump_target(restore_jump, out);
  restore_regs(reglist);
  emit_jmp(stubs[n].retaddr);
}

static void inline_writestub(enum stub_type type, int i, u_int addr,
  const signed char regmap[], int target, int adj, u_int reglist)
{
  int ra = cinfo[i].addr, rt = get_reg(regmap, target);
  uintptr_t host_addr = 0;
  void *handler = get_direct_memhandler(mem_wtab, addr, type, &host_addr);
  assert(ra >= 0 && rt >= 0);
  if (handler == NULL) {
    if (addr != host_addr) emit_movimm_from(addr, ra, host_addr, ra);
    if (type == STOREB_STUB) emit_writebyte_indexed(rt, 0, ra);
    else if (type == STOREH_STUB) emit_writehword_indexed(rt, 0, ra);
    else { assert(type == STOREW_STUB); emit_writeword_indexed(rt, 0, ra); }
    return;
  }
  save_regs(reglist);
  pass_args(ra, rt);
  int cc = get_reg(regmap, CCREG);
  if (cc < 0) emit_loadreg(CCREG, 2);
  emit_addimm(cc < 0 ? 2 : cc, adj, 2);
  emit_movimm((u_int)(uintptr_t)handler, 3);
  emit_far_call(jump_handler_write_h);
  emit_addimm(0, -adj, cc < 0 ? 2 : cc);
  if (cc < 0) emit_storereg(CCREG, 2);
  restore_regs(reglist);
}

static void c2op_assemble(struct compile_state *st, int i,
  const struct regstat *i_regs)
{
  u_int op = st->source[i] & 0x3f;
  u_int reglist = get_host_reglist(i_regs->regmap) & CALLER_SAVE_REGS;
  if (gte_handlers[op] == NULL) return;
  int need_flags = !(gte_unneeded[i + 1] >> 63);
  if (HACK_ENABLED(NDHACK_GTE_NO_FLAGS)) need_flags = 0;
  save_regs(reglist);
  cop2_do_stall_check(st, op, i, i_regs, 0);
  emit_addimm(FP, (u_char *)&psxRegs.CP2D.r[0] - (u_char *)&dynarec_local, 0);
  emit_movimm(st->source[i], 1);
  emit_writeword(1, &psxRegs.code);
  emit_far_call(need_flags ? gte_handlers[op] : gte_handlers_nf[op]);
  restore_regs(reglist);
}

static void c2op_ctc2_31_assemble(signed char sl, signed char temp)
{
  emit_andimm(sl, 0x7fffe000, temp);
  emit_testimm(temp, 0xff87ffff);
  emit_andimm(sl, 0x7ffff000, temp);
  host_tempreg_acquire();
  emit_orimm(temp, 0x80000000, HOST_TEMPREG);
  emit_cmovne_reg(HOST_TEMPREG, temp);
  host_tempreg_release();
}

static void do_mfc2_31_one(u_int copr, signed char temp)
{
  emit_readword(&reg_cop2d[copr], temp);
  RV32_EMIT_BEGIN(3);
  rv32_shift(&e, RV_SRA, NDRC_RV32_AUX_REG, rv32_hr(temp), 31);
  rv32_imm(&e, RV_XOR, NDRC_RV32_AUX_REG, NDRC_RV32_AUX_REG, -1);
  rv32_alu(&e, RV_AND, rv32_hr(temp), rv32_hr(temp), NDRC_RV32_AUX_REG);
  rv32_emit_done(&e);
  emit_cmpimm(temp, 0xf80);
  void *lt = out, *eq;
  emit_jl(0);
  eq = out;
  emit_jeq(0);
  emit_movimm(~0u, temp);
  set_jump_target(lt, out);
  set_jump_target(eq, out);
  emit_andimm(temp, 0xf80, temp);
}

static void c2op_mfc2_29_assemble(signed char tl, signed char temp)
{
  if (temp < 0) { host_tempreg_acquire(); temp = HOST_TEMPREG; }
  do_mfc2_31_one(9, temp);
  emit_shrimm(temp, 7, tl);
  do_mfc2_31_one(10, temp);
  emit_orrshr_imm(temp, 2, tl);
  do_mfc2_31_one(11, temp);
  RV32_EMIT_BEGIN(2);
  rv32_shift(&e, RV_SLL, NDRC_RV32_AUX_REG, rv32_hr(temp), 3);
  rv32_alu(&e, RV_OR, rv32_hr(tl), rv32_hr(tl), NDRC_RV32_AUX_REG);
  rv32_emit_done(&e);
  emit_writeword(tl, &reg_cop2d[29]);
  if (temp == HOST_TEMPREG) host_tempreg_release();
}

static void emit_li_fixed(unsigned rd, uint32_t value)
{
  int32_t lo = rv32_low12(value);
  RV32_EMIT_BEGIN(2);
  rv32_upper(&e, rd, value - (uint32_t)lo, 0);
  rv32_imm(&e, RV_ADD, rd, rd, lo);
  rv32_emit_done(&e);
}

/* Keep this record fixed-size: the linker must recover the source jump even
 * when either address has a zero upper/lower half. */
static void emit_extjump_stub(u_char *addr, u_int target)
{
  u_int stub = (u_int)(uintptr_t)out;
  emit_li_fixed(RV_A0, target);
  emit_li_fixed(RV_A1, (uint32_t)(uintptr_t)addr);
  emit_li_fixed(RV_A2, stub);
  emit_far_jump(dyna_linker);
}

static uint32_t decode_li_fixed(const uint32_t *p, unsigned rd)
{
  int32_t lo;
  assert((p[0] & 0x7f) == 0x37 && ((p[0] >> 7) & 31) == rd);
  assert((p[1] & 0x707f) == 0x13 && ((p[1] >> 7) & 31) == rd &&
    ((p[1] >> 15) & 31) == rd);
  lo = (int32_t)p[1] >> 20;
  return (p[0] & 0xfffff000u) + (uint32_t)lo;
}

static void *find_extjump_insn(void *stub)
{
  const uint32_t *p = stub;
  return (void *)(uintptr_t)decode_li_fixed(p + 2, RV_A1);
}

static void check_extjump2(void *stub)
{
  const uint32_t *p = stub;
  (void)decode_li_fixed(p, RV_A0);
}

static void do_jump_vaddr(int rs)
{
  if (rs != 0) emit_mov(rs, 0);
  emit_readword_indexed(LO_hash_table_ptr, FP, 1);
  emit_far_call(ndrc_get_addr_ht);
  emit_jmpreg(0);
}

static void do_preload_rhash(int r) { (void)r; }
static void do_preload_rhtbl(int ht)
{ emit_addimm(FP, LO_mini_ht, ht); }
static void do_rhash(int rs, int rh)
{ emit_andimm(rs, 0xf8, rh); }
static void do_miniht_load(int ht, int rh)
{
  emit_add(ht, rh, ht);
  emit_readword_indexed(0, ht, rh);
}
static void do_miniht_jump(int rs, int rh, int ht)
{
  emit_cmp(rh, rs);
  void *miss = out;
  emit_jne(0);
  emit_readword_indexed(4, ht, ht);
  emit_jmpreg(ht);
  set_jump_target(miss, out);
  if (rs != 0) emit_mov(rs, 0);
  emit_readword_indexed(LO_hash_table_ptr, FP, 1);
  emit_far_call(ndrc_get_addr_ht_mini);
  emit_jmpreg(0);
}
static void do_miniht_insert(struct compile_state *st, u_int return_address,
  int rt, int temp)
{
  /* The established ARM path stores a pointer to this caller's internal
   * continuation. Independently allocated RV32 register maps make that
   * unsafe. RV32 populates its mini hash on the first return miss with a
   * normal destination entry veneer instead. */
  (void)st;
  (void)temp;
  emit_movimm(return_address, rt);
}

static void multdiv_assemble_rv32(int i, const struct regstat *i_regs)
{
  signed char hi = get_reg(i_regs->regmap, HIREG);
  signed char lo = get_reg(i_regs->regmap, LOREG);

  /* RV32M directly supplies both halves of multiply and quotient/remainder.
   * RISC-V and the R3000A agree on unsigned divide-by-zero and signed
   * INT_MIN/-1. For signed divide-by-zero only, MIPS returns +1 for a
   * negative dividend while RISC-V always returns -1, so fix that case. */
  if (dops[i].rs1 && dops[i].rs2) {
    signed char lhs = get_reg(i_regs->regmap, dops[i].rs1);
    signed char rhs = get_reg(i_regs->regmap, dops[i].rs2);
    assert(lhs >= 0 && rhs >= 0 && hi >= 0 && lo >= 0);

    if (dops[i].opcode2 == 0x18 || dops[i].opcode2 == 0x19) {
      RV32_EMIT_BEGIN(2);
      rv32_alu(&e, RV_MUL, rv32_hr(lo), rv32_hr(lhs), rv32_hr(rhs));
      rv32_alu(&e, dops[i].opcode2 == 0x18 ? RV_MULH : RV_MULHU,
        rv32_hr(hi), rv32_hr(lhs), rv32_hr(rhs));
      rv32_emit_done(&e);
    }
    else if (dops[i].opcode2 == 0x1b) {
      RV32_EMIT_BEGIN(2);
      rv32_alu(&e, RV_DIVU, rv32_hr(lo), rv32_hr(lhs), rv32_hr(rhs));
      rv32_alu(&e, RV_REMU, rv32_hr(hi), rv32_hr(lhs), rv32_hr(rhs));
      rv32_emit_done(&e);
    }
    else {
      assert(dops[i].opcode2 == 0x1a);
      RV32_EMIT_BEGIN(5);
      rv32_alu(&e, RV_DIV, rv32_hr(lo), rv32_hr(lhs), rv32_hr(rhs));
      rv32_alu(&e, RV_REM, rv32_hr(hi), rv32_hr(lhs), rv32_hr(rhs));
      rv32_branch(&e, RV_NE, rv32_hr(rhs), RV_ZERO, 12);
      rv32_branch(&e, RV_GE, rv32_hr(lhs), RV_ZERO, 8);
      rv32_imm(&e, RV_ADD, rv32_hr(lo), RV_ZERO, 1);
      rv32_emit_done(&e);
    }
    return;
  }

  if ((dops[i].opcode2 == 0x1a || dops[i].opcode2 == 0x1b) &&
      dops[i].rs2 == 0) {
    /* Known zero divisor: HI gets the dividend. LO is -1 except that signed
     * division of a negative value yields +1 on the R3000A. */
    if (dops[i].rs1) {
      signed char lhs = get_reg(i_regs->regmap, dops[i].rs1);
      assert(lhs >= 0);
      if (hi >= 0) emit_mov(lhs, hi);
      if (lo >= 0) {
        if (dops[i].opcode2 == 0x1a) {
          RV32_EMIT_BEGIN(3);
          rv32_alu(&e, RV_SLT, rv32_hr(lo), rv32_hr(lhs), RV_ZERO);
          rv32_shift(&e, RV_SLL, rv32_hr(lo), rv32_hr(lo), 1);
          rv32_imm(&e, RV_ADD, rv32_hr(lo), rv32_hr(lo), -1);
          rv32_emit_done(&e);
        }
        else
          emit_movimm(~0u, lo);
      }
    }
    else {
      if (hi >= 0) emit_zeroreg(hi);
      if (lo >= 0) emit_movimm(~0u, lo);
    }
  }
  else if ((dops[i].opcode2 == 0x1a || dops[i].opcode2 == 0x1b) &&
           dops[i].rs1 == 0) {
    /* RISC-V division already produces 0 for 0/nonzero and -1 for 0/0. */
    signed char rhs = get_reg(i_regs->regmap, dops[i].rs2);
    assert(rhs >= 0);
    if (hi >= 0) emit_zeroreg(hi);
    if (lo >= 0) {
      RV32_EMIT_BEGIN(1);
      rv32_alu(&e, dops[i].opcode2 == 0x1a ? RV_DIV : RV_DIVU,
        rv32_hr(lo), RV_ZERO, rv32_hr(rhs));
      rv32_emit_done(&e);
    }
  }
  else {
    /* MULT/MULTU with either operand wired to the MIPS zero register. */
    if (hi >= 0) emit_zeroreg(hi);
    if (lo >= 0) emit_zeroreg(lo);
  }
}
#define multdiv_assemble multdiv_assemble_rv32

static void arch_init(void) {}

static void literal_pool(int n) { (void)n; }
static void literal_pool_jumpover(int n) { (void)n; }
