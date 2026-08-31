/* RISC-V instruction encoding helpers for gpSP dynarec
 * Target: RV32IMAC (ESP32-P4)
 *
 * RISC-V instruction formats:
 *   R-type: funct7[31:25] rs2[24:20] rs1[19:15] funct3[14:12] rd[11:7] opcode[6:0]
 *   I-type: imm[31:20] rs1[19:15] funct3[14:12] rd[11:7] opcode[6:0]
 *   S-type: imm[31:25] rs2[24:20] rs1[19:15] funct3[14:12] imm[11:7] opcode[6:0]
 *   B-type: imm[12|10:5] rs2[24:20] rs1[19:15] funct3[14:12] imm[4:1|11] opcode[6:0]
 *   U-type: imm[31:12] rd[11:7] opcode[6:0]
 *   J-type: imm[20|10:1|11|19:12] rd[11:7] opcode[6:0]
 */

#ifndef RISCV_CODEGEN_H
#define RISCV_CODEGEN_H

/* Register names */
#define rv_zero  0
#define rv_ra    1
#define rv_sp    2
#define rv_gp    3
#define rv_tp    4
#define rv_t0    5
#define rv_t1    6
#define rv_t2    7
#define rv_s0    8
#define rv_s1    9
#define rv_a0   10
#define rv_a1   11
#define rv_a2   12
#define rv_a3   13
#define rv_a4   14
#define rv_a5   15
#define rv_a6   16
#define rv_a7   17
#define rv_s2   18
#define rv_s3   19
#define rv_s4   20
#define rv_s5   21
#define rv_s6   22
#define rv_s7   23
#define rv_s8   24
#define rv_s9   25
#define rv_s10  26
#define rv_s11  27
#define rv_t3   28
#define rv_t4   29
#define rv_t5   30
#define rv_t6   31

/* Opcodes */
#define RV_OP_LUI     0x37
#define RV_OP_AUIPC   0x17
#define RV_OP_JAL     0x6F
#define RV_OP_JALR    0x67
#define RV_OP_BRANCH  0x63
#define RV_OP_LOAD    0x03
#define RV_OP_STORE   0x23
#define RV_OP_IMM     0x13
#define RV_OP_REG     0x33
#define RV_OP_FENCE   0x0F

/* funct3 for branches */
#define RV_BEQ   0
#define RV_BNE   1
#define RV_BLT   4
#define RV_BGE   5
#define RV_BLTU  6
#define RV_BGEU  7

/* funct3 for loads */
#define RV_LB    0
#define RV_LH    1
#define RV_LW    2
#define RV_LBU   4
#define RV_LHU   5

/* funct3 for stores */
#define RV_SB    0
#define RV_SH    1
#define RV_SW    2

/* funct3 for ALU immediate */
#define RV_ADDI  0
#define RV_SLTI  2
#define RV_SLTIU 3
#define RV_XORI  4
#define RV_ORI   6
#define RV_ANDI  7
#define RV_SLLI  1
#define RV_SRLI  5  /* funct7=0x00 */
#define RV_SRAI  5  /* funct7=0x20 */

/* funct3/funct7 for ALU register */
#define RV_ADD   0  /* funct7=0x00 */
#define RV_SUB   0  /* funct7=0x20 */
#define RV_SLL   1
#define RV_SLT   2
#define RV_SLTU  3
#define RV_XOR   4
#define RV_SRL   5  /* funct7=0x00 */
#define RV_SRA   5  /* funct7=0x20 */
#define RV_OR    6
#define RV_AND   7

/* M extension funct3 (funct7=0x01) */
#define RV_MUL    0
#define RV_MULH   1
#define RV_MULHSU 2
#define RV_MULHU  3
#define RV_DIV    4
#define RV_DIVU   5
#define RV_REM    6
#define RV_REMU   7

/* ── Encoding helpers ─────────────────────────────────────────────── */

/* R-type: op rd, rs1, rs2 */
#define rv_enc_r(funct7, rs2, rs1, funct3, rd, opcode)  \
  ((u32)(funct7) << 25 | (u32)(rs2) << 20 | (u32)(rs1) << 15 | \
   (u32)(funct3) << 12 | (u32)(rd) << 7 | (u32)(opcode))

/* I-type: op rd, rs1, imm12 */
#define rv_enc_i(imm12, rs1, funct3, rd, opcode)  \
  ((u32)((imm12) & 0xFFF) << 20 | (u32)(rs1) << 15 | \
   (u32)(funct3) << 12 | (u32)(rd) << 7 | (u32)(opcode))

/* S-type: op rs2, imm(rs1) */
#define rv_enc_s(imm12, rs2, rs1, funct3, opcode)  \
  ((u32)(((imm12) >> 5) & 0x7F) << 25 | (u32)(rs2) << 20 | \
   (u32)(rs1) << 15 | (u32)(funct3) << 12 | \
   (u32)((imm12) & 0x1F) << 7 | (u32)(opcode))

/* B-type: beq rs1, rs2, offset */
#define rv_enc_b(imm13, rs2, rs1, funct3, opcode)  \
  ((u32)(((imm13) >> 12) & 1) << 31 | \
   (u32)(((imm13) >> 5) & 0x3F) << 25 | \
   (u32)(rs2) << 20 | (u32)(rs1) << 15 | (u32)(funct3) << 12 | \
   (u32)(((imm13) >> 1) & 0xF) << 8 | \
   (u32)(((imm13) >> 11) & 1) << 7 | (u32)(opcode))

/* U-type: lui rd, imm20 */
#define rv_enc_u(imm20, rd, opcode)  \
  ((u32)((imm20) & 0xFFFFF) << 12 | (u32)(rd) << 7 | (u32)(opcode))

/* J-type: jal rd, offset */
#define rv_enc_j(imm21, rd, opcode)  \
  ((u32)(((imm21) >> 20) & 1) << 31 | \
   (u32)(((imm21) >> 1) & 0x3FF) << 21 | \
   (u32)(((imm21) >> 11) & 1) << 20 | \
   (u32)(((imm21) >> 12) & 0xFF) << 12 | \
   (u32)(rd) << 7 | (u32)(opcode))

/* ── Instruction emission macros ──────────────────────────────────── */
/* All emit into translation_ptr and advance by 4 */

#define rv_emit(inst)  \
  *((u32 *)translation_ptr) = (inst); translation_ptr += 4

/* U-type */
#define rv_emit_lui(rd, imm20)   rv_emit(rv_enc_u(imm20, rd, RV_OP_LUI))
#define rv_emit_auipc(rd, imm20) rv_emit(rv_enc_u(imm20, rd, RV_OP_AUIPC))

/* J-type */
#define rv_emit_jal(rd, off)     rv_emit(rv_enc_j(off, rd, RV_OP_JAL))

/* I-type ALU */
#define rv_emit_jalr(rd, rs1, off) rv_emit(rv_enc_i(off, rs1, 0, rd, RV_OP_JALR))
#define rv_emit_addi(rd, rs1, imm) rv_emit(rv_enc_i(imm, rs1, RV_ADDI, rd, RV_OP_IMM))
#define rv_emit_slti(rd, rs1, imm) rv_emit(rv_enc_i(imm, rs1, RV_SLTI, rd, RV_OP_IMM))
#define rv_emit_sltiu(rd, rs1, imm) rv_emit(rv_enc_i(imm, rs1, RV_SLTIU, rd, RV_OP_IMM))
#define rv_emit_xori(rd, rs1, imm) rv_emit(rv_enc_i(imm, rs1, RV_XORI, rd, RV_OP_IMM))
#define rv_emit_ori(rd, rs1, imm)  rv_emit(rv_enc_i(imm, rs1, RV_ORI, rd, RV_OP_IMM))
#define rv_emit_andi(rd, rs1, imm) rv_emit(rv_enc_i(imm, rs1, RV_ANDI, rd, RV_OP_IMM))

/* Shifts immediate */
#define rv_emit_slli(rd, rs1, sh)  rv_emit(rv_enc_i(sh, rs1, RV_SLLI, rd, RV_OP_IMM))
#define rv_emit_srli(rd, rs1, sh)  rv_emit(rv_enc_i(sh, rs1, RV_SRLI, rd, RV_OP_IMM))
#define rv_emit_srai(rd, rs1, sh)  rv_emit(rv_enc_i((sh)|0x400, rs1, RV_SRAI, rd, RV_OP_IMM))

/* R-type ALU */
#define rv_emit_add(rd, rs1, rs2)  rv_emit(rv_enc_r(0x00, rs2, rs1, RV_ADD, rd, RV_OP_REG))
#define rv_emit_sub(rd, rs1, rs2)  rv_emit(rv_enc_r(0x20, rs2, rs1, RV_SUB, rd, RV_OP_REG))
#define rv_emit_sll(rd, rs1, rs2)  rv_emit(rv_enc_r(0x00, rs2, rs1, RV_SLL, rd, RV_OP_REG))
#define rv_emit_slt(rd, rs1, rs2)  rv_emit(rv_enc_r(0x00, rs2, rs1, RV_SLT, rd, RV_OP_REG))
#define rv_emit_sltu(rd, rs1, rs2) rv_emit(rv_enc_r(0x00, rs2, rs1, RV_SLTU, rd, RV_OP_REG))
#define rv_emit_xor(rd, rs1, rs2)  rv_emit(rv_enc_r(0x00, rs2, rs1, RV_XOR, rd, RV_OP_REG))
#define rv_emit_srl(rd, rs1, rs2)  rv_emit(rv_enc_r(0x00, rs2, rs1, RV_SRL, rd, RV_OP_REG))
#define rv_emit_sra(rd, rs1, rs2)  rv_emit(rv_enc_r(0x20, rs2, rs1, RV_SRA, rd, RV_OP_REG))
#define rv_emit_or(rd, rs1, rs2)   rv_emit(rv_enc_r(0x00, rs2, rs1, RV_OR, rd, RV_OP_REG))
#define rv_emit_and(rd, rs1, rs2)  rv_emit(rv_enc_r(0x00, rs2, rs1, RV_AND, rd, RV_OP_REG))

/* M extension */
#define rv_emit_mul(rd, rs1, rs2)    rv_emit(rv_enc_r(0x01, rs2, rs1, RV_MUL, rd, RV_OP_REG))
#define rv_emit_mulh(rd, rs1, rs2)   rv_emit(rv_enc_r(0x01, rs2, rs1, RV_MULH, rd, RV_OP_REG))
#define rv_emit_mulhu(rd, rs1, rs2)  rv_emit(rv_enc_r(0x01, rs2, rs1, RV_MULHU, rd, RV_OP_REG))

/* Loads */
#define rv_emit_lb(rd, rs1, off)   rv_emit(rv_enc_i(off, rs1, RV_LB, rd, RV_OP_LOAD))
#define rv_emit_lh(rd, rs1, off)   rv_emit(rv_enc_i(off, rs1, RV_LH, rd, RV_OP_LOAD))
#define rv_emit_lw(rd, rs1, off)   rv_emit(rv_enc_i(off, rs1, RV_LW, rd, RV_OP_LOAD))
#define rv_emit_lbu(rd, rs1, off)  rv_emit(rv_enc_i(off, rs1, RV_LBU, rd, RV_OP_LOAD))
#define rv_emit_lhu(rd, rs1, off)  rv_emit(rv_enc_i(off, rs1, RV_LHU, rd, RV_OP_LOAD))

/* Stores */
#define rv_emit_sb(rs2, rs1, off)  rv_emit(rv_enc_s(off, rs2, rs1, RV_SB, RV_OP_STORE))
#define rv_emit_sh(rs2, rs1, off)  rv_emit(rv_enc_s(off, rs2, rs1, RV_SH, RV_OP_STORE))
#define rv_emit_sw(rs2, rs1, off)  rv_emit(rv_enc_s(off, rs2, rs1, RV_SW, RV_OP_STORE))

/* Branches */
#define rv_emit_beq(rs1, rs2, off)  rv_emit(rv_enc_b(off, rs2, rs1, RV_BEQ, RV_OP_BRANCH))
#define rv_emit_bne(rs1, rs2, off)  rv_emit(rv_enc_b(off, rs2, rs1, RV_BNE, RV_OP_BRANCH))
#define rv_emit_blt(rs1, rs2, off)  rv_emit(rv_enc_b(off, rs2, rs1, RV_BLT, RV_OP_BRANCH))
#define rv_emit_bge(rs1, rs2, off)  rv_emit(rv_enc_b(off, rs2, rs1, RV_BGE, RV_OP_BRANCH))
#define rv_emit_bltu(rs1, rs2, off) rv_emit(rv_enc_b(off, rs2, rs1, RV_BLTU, RV_OP_BRANCH))
#define rv_emit_bgeu(rs1, rs2, off) rv_emit(rv_enc_b(off, rs2, rs1, RV_BGEU, RV_OP_BRANCH))

/* Pseudo-instructions */
#define rv_emit_nop()              rv_emit_addi(rv_zero, rv_zero, 0)
#define rv_emit_mv(rd, rs1)        rv_emit_addi(rd, rs1, 0)
#define rv_emit_not(rd, rs1)       rv_emit_xori(rd, rs1, -1)
#define rv_emit_neg(rd, rs1)       rv_emit_sub(rd, rv_zero, rs1)
#define rv_emit_li_small(rd, imm)  rv_emit_addi(rd, rv_zero, imm)
#define rv_emit_seqz(rd, rs1)      rv_emit_sltiu(rd, rs1, 1)
#define rv_emit_snez(rd, rs1)      rv_emit_sltu(rd, rv_zero, rs1)
#define rv_emit_ret()              rv_emit_jalr(rv_zero, rv_ra, 0)
#define rv_emit_call(rd, off)      rv_emit_jal(rv_ra, off)

/* Fence */
#define rv_emit_fence_i()          rv_emit(rv_enc_i(0, 0, 1, 0, RV_OP_FENCE))

/* ── Helper: emit branch with filler (to be patched later) ─────── */
/* Returns pointer to the branch instruction for later patching */
#define rv_emit_b_filler(type, rs1, rs2, ptr)  \
  (ptr) = translation_ptr;                     \
  rv_emit_##type(rs1, rs2, 0)

/* ── Helper: patch a branch instruction with actual offset ─────── */
#define rv_patch_branch(loc, target)  \
{                                                                      \
  s32 _off = (u8*)(target) - (u8*)(loc);                               \
  u32 _old = *(u32*)(loc);                                             \
  u32 _funct3 = (_old >> 12) & 7;                                      \
  u32 _rs1 = (_old >> 15) & 0x1F;                                      \
  u32 _rs2 = (_old >> 20) & 0x1F;                                      \
  *(u32*)(loc) = rv_enc_b(_off, _rs2, _rs1, _funct3, RV_OP_BRANCH);   \
}

/* ── Helper: patch a JAL instruction ───────────────────────────── */
#define rv_patch_jal(loc, target)  \
{                                                                      \
  s32 _off = (u8*)(target) - (u8*)(loc);                               \
  u32 _old = *(u32*)(loc);                                             \
  u32 _rd = (_old >> 7) & 0x1F;                                        \
  *(u32*)(loc) = rv_enc_j(_off, _rd, RV_OP_JAL);                      \
}

#endif /* RISCV_CODEGEN_H */
