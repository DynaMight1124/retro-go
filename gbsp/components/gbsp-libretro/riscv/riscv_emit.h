/* gameplaySP - RISC-V (ESP32-P4) dynamic recompiler emit header
 *
 * Based on mips_emit.h by Exophase <exophase@gmail.com>
 * RISC-V port for ESP32-P4 (rv32imafc)
 *
 * Key differences from MIPS:
 *   - No delay slots (simplifies all branch/call patterns)
 *   - No movz/movn (use branch-over patterns instead)
 *   - No hi:lo multiply (use mul/mulh directly)
 *   - No nor (use xori -1 for NOT)
 *   - 12-bit signed immediates (vs 16-bit on MIPS)
 *   - Function calls via lui+jalr (no absolute jal)
 */

#ifndef RISCV_EMIT_H
#define RISCV_EMIT_H

#include "riscv/riscv_codegen.h"
#ifdef ESP_PLATFORM
#include "esp_cache.h"
#endif

/* Forward declarations for stub functions (defined in riscv_stub.S) */
u32 riscv_update_gba(u32 pc);
void riscv_indirect_branch_arm(u32 address);
void riscv_indirect_branch_thumb(u32 address);
void riscv_indirect_branch_dual(u32 address);
u32 execute_read_cpsr(void);
u32 execute_read_spsr(void);
void execute_swi(u32 pc);
void riscv_cheat_hook(void);
u32 execute_spsr_restore(u32 address);
void execute_store_cpsr(u32 new_cpsr, u32 store_mask);
void execute_store_spsr(u32 new_spsr, u32 store_mask);
void smc_write(void);
void write_io_epilogue(u32 alert_flags);

/* Memory handler pointer tables (filled by stub init) */
extern u32 tmemld[11][16];
extern u32 tmemst[4][16];
extern u32 thnjal[15 * 16];

/* Default handlers - IWRAM assumed, aligned by default */
#define execute_load_u8         tmemld[0][3]
#define execute_load_s8         tmemld[1][3]
#define execute_load_u16        tmemld[2][3]
#define execute_load_s16        tmemld[4][3]
#define execute_load_u32        tmemld[6][3]
#define execute_aligned_load32  tmemld[10][3]
#define execute_store_u8        tmemst[0][3]
#define execute_store_u16       tmemst[1][3]
#define execute_store_u32       tmemst[2][3]
#define execute_aligned_store32 tmemst[3][3]

/* ==================================================================
 * Register Mapping
 * ================================================================== */

/* System registers (callee-saved) */
#define reg_base    rv_s0    /* GBA state array base pointer */
#define reg_cycles  rv_s1    /* Cycle counter (counts down) */
#define reg_pc      rv_s2    /* Stored PC value */
#define reg_n_cache rv_s3    /* N flag */
#define reg_z_cache rv_s4    /* Z flag */
#define reg_c_cache rv_s5    /* C flag */
#define reg_v_cache rv_s6    /* V flag */

/* Temporaries and function args */
#define reg_a0      rv_a0    /* fn arg0 / return value / r15 target */
#define reg_a1      rv_a1    /* fn arg1 */
#define reg_a2      rv_a2    /* fn arg2 */
#define reg_rv      rv_t6    /* secondary scratch / return value copy */
#define reg_temp    rv_t5    /* primary scratch */
#define reg_zero    rv_zero  /* constant zero */

/* GBA r0-r14 mapped to host registers */
#define reg_r0  rv_t0
#define reg_r1  rv_t1
#define reg_r2  rv_t2
#define reg_r3  rv_a3
#define reg_r4  rv_a4
#define reg_r5  rv_a5
#define reg_r6  rv_a6
#define reg_r7  rv_a7
#define reg_r8  rv_s7
#define reg_r9  rv_s8
#define reg_r10 rv_s9
#define reg_r11 rv_s10
#define reg_r12 rv_s11
#define reg_r13 rv_t3
#define reg_r14 rv_t4

/* GBA register index -> host register lookup */
u32 arm_to_mips_reg[] =
{
  reg_r0,  reg_r1,  reg_r2,  reg_r3,
  reg_r4,  reg_r5,  reg_r6,  reg_r7,
  reg_r8,  reg_r9,  reg_r10, reg_r11,
  reg_r12, reg_r13, reg_r14,
  reg_a0,  /* r15: PC writes go to a0 for branch dispatch */
  reg_a1,  /* arm_reg_a1: scratch */
  reg_a2   /* arm_reg_a2: scratch */
};

#define arm_reg_a0  15
#define arm_reg_a1  16
#define arm_reg_a2  17

/* ==================================================================
 * Immediate range check
 * ================================================================== */
#define rv_imm12_fits(v) (((s32)(v) >= -2048) && ((s32)(v) <= 2047))

/* ==================================================================
 * Core code generation macros
 * ================================================================== */

#define generate_load_reg(ireg, reg_index)                                    \
  rv_emit_mv(ireg, arm_to_mips_reg[reg_index])

#define generate_store_reg(ireg, reg_index)                                   \
  rv_emit_mv(arm_to_mips_reg[reg_index], ireg)

#define generate_load_imm(ireg, imm)                                          \
{                                                                             \
  u32 _v = (u32)(imm);                                                        \
  if(rv_imm12_fits((s32)_v)) {                                                \
    rv_emit_addi(ireg, rv_zero, (s32)_v);                                     \
  } else {                                                                    \
    rv_emit_lui(ireg, (_v + 0x800) >> 12);                                    \
    if((_v & 0xFFF) != 0) {                                                   \
      rv_emit_addi(ireg, ireg, _v & 0xFFF);                                  \
    }                                                                         \
  }                                                                           \
}

#define generate_load_pc(ireg, new_pc)                                        \
{                                                                             \
  s32 _delta = (s32)((new_pc) - (stored_pc));                                 \
  if(rv_imm12_fits(_delta)) {                                                 \
    rv_emit_addi(ireg, reg_pc, _delta);                                       \
  } else {                                                                    \
    generate_load_imm(ireg, (new_pc));                                        \
  }                                                                           \
}

#define generate_mov(ireg_dest, ireg_src)                                     \
  rv_emit_mv(ireg_dest, ireg_src)

/* ==================================================================
 * ALU immediate helpers
 *
 * RISC-V addi/andi/ori/xori have 12-bit signed immediates.
 * If the immediate doesn't fit, load into reg_temp first.
 * ================================================================== */

/* For signed immediates (addi) */
#define generate_alu_imm(imm_type, reg_type, ireg_dest, ireg_src, imm)        \
  if(rv_imm12_fits((s32)(imm)))                                               \
  {                                                                           \
    rv_emit_##imm_type(ireg_dest, ireg_src, (s32)(imm));                      \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_load_imm(reg_temp, (imm));                                       \
    rv_emit_##reg_type(ireg_dest, ireg_src, reg_temp);                        \
  }

/* For unsigned immediates (andi/ori/xori) - same check on RISC-V */
#define generate_alu_immu(imm_type, reg_type, ireg_dest, ireg_src, imm)       \
  if(rv_imm12_fits((s32)(imm)))                                               \
  {                                                                           \
    rv_emit_##imm_type(ireg_dest, ireg_src, (s32)(imm));                      \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_load_imm(reg_temp, (imm));                                       \
    rv_emit_##reg_type(ireg_dest, ireg_src, reg_temp);                        \
  }

/* Safe addi that handles out-of-range immediates */
#define rv_addi_safe(rd, rs, imm)                                             \
  if(rv_imm12_fits((s32)(imm))) {                                             \
    rv_emit_addi(rd, rs, (s32)(imm));                                         \
  } else {                                                                    \
    generate_load_imm(reg_temp, (imm));                                       \
    rv_emit_add(rd, rs, reg_temp);                                            \
  }

/* ==================================================================
 * Multiply
 * ================================================================== */

/* For multiply_long: compute 64-bit product directly using mul/mulh */
#define generate_multiply_s64()                                               \
  rv_emit_mul(reg_a0, arm_to_mips_reg[rm], arm_to_mips_reg[rs]);             \
  rv_emit_mulh(reg_a1, arm_to_mips_reg[rm], arm_to_mips_reg[rs])

#define generate_multiply_u64()                                               \
  rv_emit_mul(reg_a0, arm_to_mips_reg[rm], arm_to_mips_reg[rs]);             \
  rv_emit_mulhu(reg_a1, arm_to_mips_reg[rm], arm_to_mips_reg[rs])

/* For multiply-accumulate, we add to rdlo:rdhi */
#define generate_multiply_s64_add()                                           \
  rv_emit_mul(reg_temp, arm_to_mips_reg[rm], arm_to_mips_reg[rs]);           \
  rv_emit_mulh(reg_rv, arm_to_mips_reg[rm], arm_to_mips_reg[rs]);            \
  rv_emit_add(arm_to_mips_reg[rdlo], arm_to_mips_reg[rdlo], reg_temp);       \
  rv_emit_sltu(reg_temp, arm_to_mips_reg[rdlo], reg_temp);                   \
  rv_emit_add(arm_to_mips_reg[rdhi], arm_to_mips_reg[rdhi], reg_rv);         \
  rv_emit_add(arm_to_mips_reg[rdhi], arm_to_mips_reg[rdhi], reg_temp)

#define generate_multiply_u64_add()                                           \
  rv_emit_mul(reg_temp, arm_to_mips_reg[rm], arm_to_mips_reg[rs]);           \
  rv_emit_mulhu(reg_rv, arm_to_mips_reg[rm], arm_to_mips_reg[rs]);           \
  rv_emit_add(arm_to_mips_reg[rdlo], arm_to_mips_reg[rdlo], reg_temp);       \
  rv_emit_sltu(reg_temp, arm_to_mips_reg[rdlo], reg_temp);                   \
  rv_emit_add(arm_to_mips_reg[rdhi], arm_to_mips_reg[rdhi], reg_rv);         \
  rv_emit_add(arm_to_mips_reg[rdhi], arm_to_mips_reg[rdhi], reg_temp)

/* ==================================================================
 * Function calls
 *
 * RISC-V has no absolute jump; use lui+jalr for all far calls.
 * No delay slots — arguments set up BEFORE the call.
 * ================================================================== */

#define genccall(fnptr)                                                       \
{                                                                             \
  u32 _addr = (u32)(fnptr);                                                   \
  rv_emit_lui(reg_temp, (_addr + 0x800) >> 12);                               \
  rv_emit_jalr(rv_ra, reg_temp, _addr & 0xFFF);                              \
}

#define generate_function_call(function_location)                             \
  genccall(function_location)

/* On MIPS, swap_delay moves the prior instruction into the delay slot.
 * On RISC-V, there are no delay slots — just call normally. */
#define generate_function_call_swap_delay(function_location)                  \
  generate_function_call(function_location)

#define generate_function_return_swap_delay()                                 \
  rv_emit_ret()

/* No-op on RISC-V (MIPS delay slot reordering) */
#define generate_swap_delay()

#define generate_raw_u32(value)                                               \
  *((u32 *)translation_ptr) = (u32)(value);                                   \
  translation_ptr += 4

/* ==================================================================
 * Cycle counter updates
 * ================================================================== */

#define generate_cycle_update()                                               \
  if(cycle_count != 0)                                                        \
  {                                                                           \
    rv_addi_safe(reg_cycles, reg_cycles, -(s32)cycle_count);                  \
    cycle_count = 0;                                                          \
  }

#define generate_cycle_update_force()                                         \
  rv_addi_safe(reg_cycles, reg_cycles, -(s32)cycle_count);                    \
  cycle_count = 0

/* ==================================================================
 * Branch patching
 *
 * Conditional: B-type branch (±4KB), patch entire instruction
 * Unconditional: lui+jalr pair (full 32-bit range), patch both
 * ================================================================== */

#define generate_branch_patch_conditional(dest, offset)                       \
  rv_patch_branch(dest, offset)

#define generate_branch_patch_unconditional(dest, offset)                     \
{                                                                             \
  u32 _tgt = (u32)(offset);                                                   \
  ((u32*)(dest))[0] = rv_enc_u((_tgt + 0x800) >> 12, reg_temp, RV_OP_LUI);   \
  ((u32*)(dest))[1] = rv_enc_i(_tgt & 0xFFF, reg_temp, 0, rv_zero,           \
                                RV_OP_JALR);                                  \
}

/* Emit an unconditional jump filler (lui+jalr, 8 bytes) for block exits */
#define rv_emit_j_filler(loc)                                                 \
  (loc) = translation_ptr;                                                    \
  rv_emit_lui(reg_temp, 0);                                                   \
  rv_emit_jalr(rv_zero, reg_temp, 0)

/* ==================================================================
 * Branch generation
 * ================================================================== */

#define generate_branch_no_cycle_update(writeback_location, new_pc)           \
  if(pc == idle_loop_target_pc)                                               \
  {                                                                           \
    generate_load_pc(reg_a0, new_pc);                                         \
    rv_emit_addi(reg_cycles, rv_zero, 0);                                     \
    generate_function_call(riscv_update_gba);                                 \
    rv_emit_j_filler(writeback_location);                                     \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_load_pc(reg_a0, new_pc);                                         \
    /* If cycles < 0, call update trampoline */                               \
    rv_emit_bge(reg_cycles, rv_zero, 8);                                      \
    {                                                                         \
      s32 _toff = (s32)(update_trampoline - translation_ptr);                 \
      rv_emit_jal(rv_ra, _toff);                                              \
    }                                                                         \
    rv_emit_j_filler(writeback_location);                                     \
  }

#define generate_branch_cycle_update(writeback_location, new_pc)              \
  generate_cycle_update();                                                    \
  generate_branch_no_cycle_update(writeback_location, new_pc)

/* Indirect branches: a0 holds the GBA destination address */
#define generate_indirect_branch_cycle_update(type)                           \
  generate_cycle_update_force();                                              \
  generate_function_call(riscv_indirect_branch_##type)

#define generate_indirect_branch_no_cycle_update(type)                        \
  generate_function_call(riscv_indirect_branch_##type)

/* ==================================================================
 * Block prologue
 *
 * Layout (8 bytes):
 *   [0]: lui  reg_temp, hi(riscv_update_gba)
 *   [4]: jalr zero, reg_temp, lo(riscv_update_gba)
 * Then generate_load_imm for stored_pc (not counted in prologue size)
 * ================================================================== */

#define block_prologue_size 8

#define generate_block_prologue()                                             \
  update_trampoline = translation_ptr;                                        \
  {                                                                           \
    u32 _addr = (u32)riscv_update_gba;                                        \
    rv_emit_lui(reg_temp, (_addr + 0x800) >> 12);                             \
    rv_emit_jalr(rv_zero, reg_temp, _addr & 0xFFF);                           \
  }                                                                           \
  /* Always emit 2-instruction LUI+ADDI for stored_pc to avoid              */\
  /* single-ADDI encoding corruption observed on ESP32-P4 PSRAM             */\
  {                                                                           \
    u32 _spc = (u32)stored_pc;                                                \
    rv_emit_lui(reg_pc, (_spc + 0x800) >> 12);                                \
    rv_emit_addi(reg_pc, reg_pc, _spc & 0xFFF);                              \
  }

#define generate_block_extra_vars()                                           \
  u32 stored_pc = pc;                                                         \
  u8 *update_trampoline

#define generate_block_extra_vars_arm()                                       \
  generate_block_extra_vars()

#define generate_block_extra_vars_thumb()                                     \
  generate_block_extra_vars()

/* ==================================================================
 * Flag check macros (compile-time, from flag_status)
 * ================================================================== */

#define check_generate_n_flag (flag_status & 0x08)
#define check_generate_z_flag (flag_status & 0x04)
#define check_generate_c_flag (flag_status & 0x02)
#define check_generate_v_flag (flag_status & 0x01)

/* ==================================================================
 * Register load/store with PC handling
 * ================================================================== */

#define generate_load_reg_pc(ireg, reg_index, pc_offset)                      \
  if(reg_index == REG_PC)                                                     \
  {                                                                           \
    generate_load_pc(ireg, (pc + pc_offset));                                 \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_load_reg(ireg, reg_index);                                       \
  }

#define check_load_reg_pc(arm_reg, reg_index, pc_offset)                      \
  if(reg_index == REG_PC)                                                     \
  {                                                                           \
    reg_index = arm_reg;                                                      \
    generate_load_pc(arm_to_mips_reg[arm_reg], (pc + pc_offset));             \
  }

#define check_store_reg_pc_no_flags(reg_index)                                \
  if(reg_index == REG_PC)                                                     \
  {                                                                           \
    generate_indirect_branch_arm();                                           \
  }

#define check_store_reg_pc_flags(reg_index)                                   \
  if(reg_index == REG_PC)                                                     \
  {                                                                           \
    generate_function_call(execute_spsr_restore);                             \
    generate_indirect_branch_dual();                                          \
  }

/* ==================================================================
 * Bit manipulation helpers (no ins/ext on RISC-V)
 * ================================================================== */

#define extract_bits(rt, rs, pos, size)                                       \
  rv_emit_slli(rt, rs, 32 - ((pos) + (size)));                                \
  rv_emit_srli(rt, rt, 32 - (size))

#define insert_bits(rdest, rsrc, rtemp, pos, size)                            \
  rv_emit_slli(rtemp, rsrc, 32 - (size));                                     \
  rv_emit_srli(rtemp, rtemp, 32 - (size) - (pos));                            \
  rv_emit_or(rdest, rdest, rtemp)

#define emit_align_reg(regv, numbits)                                         \
  rv_emit_srli(regv, regv, numbits);                                          \
  rv_emit_slli(regv, regv, numbits)

#define double_byte(regv, rtmp)                                               \
  rv_emit_slli(rtmp, regv, 8);                                                \
  rv_emit_andi(regv, regv, 0xFF);                                             \
  rv_emit_or(regv, regv, rtmp)

#define extend_byte_signed(rd, rs)                                            \
  rv_emit_slli(rd, rs, 24);                                                   \
  rv_emit_srai(rd, rd, 24)

#define rotate_right(rdest, rsrc, rtemp, amount)                              \
  rv_emit_slli(rtemp, rsrc, 32 - (amount));                                   \
  rv_emit_srli(rdest, rsrc, (amount));                                        \
  rv_emit_or(rdest, rdest, rtemp)

#define rotate_right_var(rdest, rsrc, rtemp, ramount)                         \
  rv_emit_andi(rtemp, ramount, 0x1F);                                         \
  rv_emit_srl(rdest, rsrc, rtemp);                                            \
  rv_emit_sub(rtemp, rv_zero, rtemp);                                         \
  rv_emit_addi(rtemp, rtemp, 32);                                             \
  rv_emit_sll(rtemp, rsrc, rtemp);                                            \
  rv_emit_or(rdest, rdest, rtemp)

/* ==================================================================
 * Conditional move emulation (RISC-V has no movz/movn)
 * ================================================================== */

/* movz: if cond_reg == 0, dest = src */
#define rv_movz(dest, src, cond_reg)                                          \
  rv_emit_bne(cond_reg, rv_zero, 8);                                          \
  rv_emit_mv(dest, src)

/* movn: if cond_reg != 0, dest = src */
#define rv_movn(dest, src, cond_reg)                                          \
  rv_emit_beq(cond_reg, rv_zero, 8);                                          \
  rv_emit_mv(dest, src)

/* ==================================================================
 * Shift macros — immediate, no flags
 * ================================================================== */

#define generate_shift_imm_lsl_no_flags(arm_reg, _rm, _shift)                 \
  check_load_reg_pc(arm_reg, _rm, 8);                                         \
  if(_shift != 0)                                                             \
  {                                                                           \
    rv_emit_slli(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], _shift);     \
    _rm = arm_reg;                                                            \
  }

#define generate_shift_imm_lsr_no_flags(arm_reg, _rm, _shift)                 \
  if(_shift != 0)                                                             \
  {                                                                           \
    check_load_reg_pc(arm_reg, _rm, 8);                                       \
    rv_emit_srli(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], _shift);     \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    rv_emit_addi(arm_to_mips_reg[arm_reg], rv_zero, 0);                       \
  }                                                                           \
  _rm = arm_reg

#define generate_shift_imm_asr_no_flags(arm_reg, _rm, _shift)                 \
  check_load_reg_pc(arm_reg, _rm, 8);                                         \
  if(_shift != 0)                                                             \
  {                                                                           \
    rv_emit_srai(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], _shift);     \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    rv_emit_srai(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], 31);         \
  }                                                                           \
  _rm = arm_reg

#define generate_shift_imm_ror_no_flags(arm_reg, _rm, _shift)                 \
  check_load_reg_pc(arm_reg, _rm, 8);                                         \
  if(_shift != 0)                                                             \
  {                                                                           \
    rotate_right(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm],              \
                 reg_temp, _shift);                                           \
  }                                                                           \
  else                                                                        \
  { /* RRX: rotate right through carry */                                     \
    rv_emit_srli(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], 1);          \
    insert_bits(arm_to_mips_reg[arm_reg], reg_c_cache, reg_temp, 31, 1);      \
  }                                                                           \
  _rm = arm_reg

/* ==================================================================
 * Shift macros — immediate, with flags (update C flag)
 * ================================================================== */

#define generate_shift_imm_lsl_flags(arm_reg, _rm, _shift)                    \
  check_load_reg_pc(arm_reg, _rm, 8);                                         \
  if(_shift != 0)                                                             \
  {                                                                           \
    extract_bits(reg_c_cache, arm_to_mips_reg[_rm], (32 - _shift), 1);        \
    rv_emit_slli(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], _shift);     \
    _rm = arm_reg;                                                            \
  }

#define generate_shift_imm_lsr_flags(arm_reg, _rm, _shift)                    \
  check_load_reg_pc(arm_reg, _rm, 8);                                         \
  if(_shift != 0)                                                             \
  {                                                                           \
    extract_bits(reg_c_cache, arm_to_mips_reg[_rm], (_shift - 1), 1);         \
    rv_emit_srli(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], _shift);     \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    rv_emit_srli(reg_c_cache, arm_to_mips_reg[_rm], 31);                      \
    rv_emit_addi(arm_to_mips_reg[arm_reg], rv_zero, 0);                       \
  }                                                                           \
  _rm = arm_reg

#define generate_shift_imm_asr_flags(arm_reg, _rm, _shift)                    \
  check_load_reg_pc(arm_reg, _rm, 8);                                         \
  if(_shift != 0)                                                             \
  {                                                                           \
    extract_bits(reg_c_cache, arm_to_mips_reg[_rm], (_shift - 1), 1);         \
    rv_emit_srai(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], _shift);     \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    rv_emit_srai(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], 31);         \
    rv_emit_andi(reg_c_cache, arm_to_mips_reg[arm_reg], 1);                   \
  }                                                                           \
  _rm = arm_reg

#define generate_shift_imm_ror_flags(arm_reg, _rm, _shift)                    \
  check_load_reg_pc(arm_reg, _rm, 8);                                         \
  if(_shift != 0)                                                             \
  {                                                                           \
    extract_bits(reg_c_cache, arm_to_mips_reg[_rm], (_shift - 1), 1);         \
    rotate_right(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm],              \
                 reg_temp, _shift);                                           \
  }                                                                           \
  else                                                                        \
  { /* RRX with carry update */                                               \
    rv_emit_slli(reg_temp, reg_c_cache, 31);                                   \
    rv_emit_andi(reg_c_cache, arm_to_mips_reg[_rm], 1);                       \
    rv_emit_srli(arm_to_mips_reg[arm_reg], arm_to_mips_reg[_rm], 1);          \
    rv_emit_or(arm_to_mips_reg[arm_reg], arm_to_mips_reg[arm_reg], reg_temp); \
  }                                                                           \
  _rm = arm_reg

/* ==================================================================
 * Shift macros — register, no flags
 *
 * ARM shifts by register: if shift >= 32, result depends on type.
 * RISC-V SLL/SRL/SRA only use lower 5 bits (mod 32).
 * Must explicitly handle shift >= 32.
 * ================================================================== */

#define generate_shift_reg_lsl_no_flags(_rm, _rs)                             \
  rv_emit_sll(reg_a0, arm_to_mips_reg[_rm], arm_to_mips_reg[_rs]);            \
  rv_emit_sltiu(reg_temp, arm_to_mips_reg[_rs], 32);                          \
  rv_movz(reg_a0, rv_zero, reg_temp)

#define generate_shift_reg_lsr_no_flags(_rm, _rs)                             \
  rv_emit_srl(reg_a0, arm_to_mips_reg[_rm], arm_to_mips_reg[_rs]);            \
  rv_emit_sltiu(reg_temp, arm_to_mips_reg[_rs], 32);                          \
  rv_movz(reg_a0, rv_zero, reg_temp)

#define generate_shift_reg_asr_no_flags(_rm, _rs)                             \
  rv_emit_sra(reg_a0, arm_to_mips_reg[_rm], arm_to_mips_reg[_rs]);            \
  rv_emit_sltiu(reg_temp, arm_to_mips_reg[_rs], 32);                          \
  /* If shift >= 32, ASR gives sign-fill (bit 31 replicated) */               \
  rv_emit_bne(reg_temp, rv_zero, 8);                                          \
  rv_emit_srai(reg_a0, arm_to_mips_reg[_rm], 31)

#define generate_shift_reg_ror_no_flags(_rm, _rs)                             \
  rotate_right_var(reg_a0, arm_to_mips_reg[_rm],                              \
                   reg_temp, arm_to_mips_reg[_rs])

/* ==================================================================
 * Shift macros — register, with flags
 * ================================================================== */

#define generate_shift_reg_lsl_flags(_rm, _rs)                                \
{                                                                             \
  u32 shift_reg = _rs;                                                        \
  check_load_reg_pc(arm_reg_a1, shift_reg, 8);                                \
  generate_load_reg_pc(reg_a0, _rm, 12);                                      \
  /* If shift == 0, keep result and flags unchanged */                        \
  rv_emit_beq(arm_to_mips_reg[shift_reg], rv_zero, 10 * 4);                  \
  rv_emit_addi(reg_temp, arm_to_mips_reg[shift_reg], -1);                     \
  rv_emit_sll(reg_a0, reg_a0, reg_temp);                                      \
  rv_emit_srli(reg_c_cache, reg_a0, 31);                                      \
  rv_emit_sltiu(reg_temp, arm_to_mips_reg[shift_reg], 33);                    \
  rv_emit_slli(reg_a0, reg_a0, 1);                                            \
  rv_movz(reg_c_cache, rv_zero, reg_temp);                                    \
  rv_movz(reg_a0, rv_zero, reg_temp);                                         \
}

#define generate_shift_reg_lsr_flags(_rm, _rs)                                \
{                                                                             \
  u32 shift_reg = _rs;                                                        \
  check_load_reg_pc(arm_reg_a1, shift_reg, 8);                                \
  generate_load_reg_pc(reg_a0, _rm, 12);                                      \
  rv_emit_beq(arm_to_mips_reg[shift_reg], rv_zero, 10 * 4);                  \
  rv_emit_addi(reg_temp, arm_to_mips_reg[shift_reg], -1);                     \
  rv_emit_srl(reg_a0, reg_a0, reg_temp);                                      \
  rv_emit_andi(reg_c_cache, reg_a0, 1);                                       \
  rv_emit_sltiu(reg_temp, arm_to_mips_reg[shift_reg], 33);                    \
  rv_emit_srli(reg_a0, reg_a0, 1);                                            \
  rv_movz(reg_c_cache, rv_zero, reg_temp);                                    \
  rv_movz(reg_a0, rv_zero, reg_temp);                                         \
}

#define generate_shift_reg_asr_flags(_rm, _rs)                                \
  generate_load_reg_pc(reg_a1, _rs, 8);                                       \
  generate_load_reg_pc(reg_a0, _rm, 12);                                      \
  rv_emit_beq(reg_a1, rv_zero, 9 * 4);                                        \
  /* Cap shift at 32 */                                                       \
  rv_emit_addi(reg_temp, rv_zero, 32);                                        \
  rv_emit_srli(reg_rv, reg_a1, 5);                                            \
  rv_movn(reg_a1, reg_temp, reg_rv);                                          \
  rv_emit_addi(reg_temp, reg_a1, -1);                                         \
  rv_emit_sra(reg_a0, reg_a0, reg_temp);                                      \
  rv_emit_andi(reg_c_cache, reg_a0, 1);                                       \
  rv_emit_srai(reg_a0, reg_a0, 1)

#define generate_shift_reg_ror_flags(_rm, _rs)                                \
  rv_emit_beq(arm_to_mips_reg[_rs], rv_zero, 4 * 4);                         \
  rv_emit_addi(reg_temp, arm_to_mips_reg[_rs], -1);                           \
  rv_emit_srl(reg_temp, arm_to_mips_reg[_rm], reg_temp);                      \
  rv_emit_andi(reg_c_cache, reg_temp, 1);                                     \
  rotate_right_var(reg_a0, arm_to_mips_reg[_rm],                              \
                   reg_temp, arm_to_mips_reg[_rs])

/* ==================================================================
 * Shift dispatchers
 * ================================================================== */

#define generate_shift_imm(arm_reg, name, flags_op)                           \
  u32 shift = (opcode >> 7) & 0x1F;                                           \
  generate_shift_imm_##name##_##flags_op(arm_reg, rm, shift)

#define generate_shift_reg(arm_reg, name, flags_op)                           \
  u32 rs = ((opcode >> 8) & 0x0F);                                            \
  generate_shift_reg_##name##_##flags_op(rm, rs);                             \
  rm = arm_reg

#define generate_load_rm_sh(rm, flags_op)                                     \
{                                                                             \
  switch((opcode >> 4) & 0x07)                                                \
  {                                                                           \
    case 0x0: { generate_shift_imm(arm_reg_a0, lsl, flags_op); break; }       \
    case 0x1: { generate_shift_reg(arm_reg_a0, lsl, flags_op); break; }       \
    case 0x2: { generate_shift_imm(arm_reg_a0, lsr, flags_op); break; }       \
    case 0x3: { generate_shift_reg(arm_reg_a0, lsr, flags_op); break; }       \
    case 0x4: { generate_shift_imm(arm_reg_a0, asr, flags_op); break; }       \
    case 0x5: { generate_shift_reg(arm_reg_a0, asr, flags_op); break; }       \
    case 0x6: { generate_shift_imm(arm_reg_a0, ror, flags_op); break; }       \
    case 0x7: { generate_shift_reg(arm_reg_a0, ror, flags_op); break; }       \
  }                                                                           \
}

#define generate_load_offset_sh(rm)                                           \
{                                                                             \
  switch((opcode >> 5) & 0x03)                                                \
  {                                                                           \
    case 0x0: { generate_shift_imm(arm_reg_a1, lsl, no_flags); break; }       \
    case 0x1: { generate_shift_imm(arm_reg_a1, lsr, no_flags); break; }       \
    case 0x2: { generate_shift_imm(arm_reg_a1, asr, no_flags); break; }       \
    case 0x3: { generate_shift_imm(arm_reg_a1, ror, no_flags); break; }       \
  }                                                                           \
}

/* ==================================================================
 * Indirect branch wrappers
 * ================================================================== */

#define generate_indirect_branch_arm()                                        \
{                                                                             \
  if(condition == 0x0E)                                                       \
  {                                                                           \
    generate_indirect_branch_cycle_update(arm);                               \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_indirect_branch_no_cycle_update(arm);                            \
  }                                                                           \
}

#define generate_indirect_branch_dual()                                       \
{                                                                             \
  if(condition == 0x0E)                                                       \
  {                                                                           \
    generate_indirect_branch_cycle_update(dual);                              \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_indirect_branch_no_cycle_update(dual);                           \
  }                                                                           \
}

/* ==================================================================
 * Condition macros
 *
 * These branch to SKIP the conditional code if the condition is
 * NOT met (opposite sense). The branch filler is patched later.
 *
 * On MIPS, cycle update was in the delay slot (always executed).
 * On RISC-V, put it before the branch (same effect).
 * ================================================================== */

#define generate_condition_eq()                                               \
  generate_cycle_update_force();                                              \
  rv_emit_b_filler(beq, reg_z_cache, rv_zero, backpatch_address)

#define generate_condition_ne()                                               \
  generate_cycle_update_force();                                              \
  rv_emit_b_filler(bne, reg_z_cache, rv_zero, backpatch_address)

#define generate_condition_cs()                                               \
  generate_cycle_update_force();                                              \
  rv_emit_b_filler(beq, reg_c_cache, rv_zero, backpatch_address)

#define generate_condition_cc()                                               \
  generate_cycle_update_force();                                              \
  rv_emit_b_filler(bne, reg_c_cache, rv_zero, backpatch_address)

#define generate_condition_mi()                                               \
  generate_cycle_update_force();                                              \
  rv_emit_b_filler(beq, reg_n_cache, rv_zero, backpatch_address)

#define generate_condition_pl()                                               \
  generate_cycle_update_force();                                              \
  rv_emit_b_filler(bne, reg_n_cache, rv_zero, backpatch_address)

#define generate_condition_vs()                                               \
  generate_cycle_update_force();                                              \
  rv_emit_b_filler(beq, reg_v_cache, rv_zero, backpatch_address)

#define generate_condition_vc()                                               \
  generate_cycle_update_force();                                              \
  rv_emit_b_filler(bne, reg_v_cache, rv_zero, backpatch_address)

#define generate_condition_hi()                                               \
  generate_cycle_update_force();                                              \
  rv_emit_xori(reg_temp, reg_c_cache, 1);                                     \
  rv_emit_or(reg_temp, reg_temp, reg_z_cache);                                \
  rv_emit_b_filler(bne, reg_temp, rv_zero, backpatch_address)

#define generate_condition_ls()                                               \
  generate_cycle_update_force();                                              \
  rv_emit_xori(reg_temp, reg_c_cache, 1);                                     \
  rv_emit_or(reg_temp, reg_temp, reg_z_cache);                                \
  rv_emit_b_filler(beq, reg_temp, rv_zero, backpatch_address)

#define generate_condition_ge()                                               \
  generate_cycle_update_force();                                              \
  rv_emit_b_filler(bne, reg_n_cache, reg_v_cache, backpatch_address)

#define generate_condition_lt()                                               \
  generate_cycle_update_force();                                              \
  rv_emit_b_filler(beq, reg_n_cache, reg_v_cache, backpatch_address)

#define generate_condition_gt()                                               \
  generate_cycle_update_force();                                              \
  rv_emit_xor(reg_temp, reg_n_cache, reg_v_cache);                            \
  rv_emit_or(reg_temp, reg_temp, reg_z_cache);                                \
  rv_emit_b_filler(bne, reg_temp, rv_zero, backpatch_address)

#define generate_condition_le()                                               \
  generate_cycle_update_force();                                              \
  rv_emit_xor(reg_temp, reg_n_cache, reg_v_cache);                            \
  rv_emit_or(reg_temp, reg_temp, reg_z_cache);                                \
  rv_emit_b_filler(beq, reg_temp, rv_zero, backpatch_address)

#define generate_condition()                                                  \
  switch(condition)                                                           \
  {                                                                           \
    case 0x0: generate_condition_eq(); break;                                 \
    case 0x1: generate_condition_ne(); break;                                 \
    case 0x2: generate_condition_cs(); break;                                 \
    case 0x3: generate_condition_cc(); break;                                 \
    case 0x4: generate_condition_mi(); break;                                 \
    case 0x5: generate_condition_pl(); break;                                 \
    case 0x6: generate_condition_vs(); break;                                 \
    case 0x7: generate_condition_vc(); break;                                 \
    case 0x8: generate_condition_hi(); break;                                 \
    case 0x9: generate_condition_ls(); break;                                 \
    case 0xA: generate_condition_ge(); break;                                 \
    case 0xB: generate_condition_lt(); break;                                 \
    case 0xC: generate_condition_gt(); break;                                 \
    case 0xD: generate_condition_le(); break;                                 \
    default: break;                                                           \
  }

#define generate_branch()                                                     \
{                                                                             \
  if(condition == 0x0E)                                                       \
  {                                                                           \
    generate_branch_cycle_update(                                             \
     block_exits[block_exit_position].branch_source,                          \
     block_exits[block_exit_position].branch_target);                         \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_branch_no_cycle_update(                                          \
     block_exits[block_exit_position].branch_source,                          \
     block_exits[block_exit_position].branch_target);                         \
  }                                                                           \
  block_exit_position++;                                                      \
}

/* ==================================================================
 * ALU operations — register operand
 * ================================================================== */

#define generate_op_and_reg(_rd, _rn, _rm)                                    \
  rv_emit_and(_rd, _rn, _rm)

#define generate_op_orr_reg(_rd, _rn, _rm)                                    \
  rv_emit_or(_rd, _rn, _rm)

#define generate_op_eor_reg(_rd, _rn, _rm)                                    \
  rv_emit_xor(_rd, _rn, _rm)

#define generate_op_bic_reg(_rd, _rn, _rm)                                    \
  rv_emit_not(reg_temp, _rm);                                                 \
  rv_emit_and(_rd, _rn, reg_temp)

#define generate_op_sub_reg(_rd, _rn, _rm)                                    \
  rv_emit_sub(_rd, _rn, _rm)

#define generate_op_rsb_reg(_rd, _rn, _rm)                                    \
  rv_emit_sub(_rd, _rm, _rn)

#define generate_op_sbc_reg(_rd, _rn, _rm)                                    \
  rv_emit_sub(_rd, _rn, _rm);                                                 \
  rv_emit_xori(reg_temp, reg_c_cache, 1);                                     \
  rv_emit_sub(_rd, _rd, reg_temp)

#define generate_op_rsc_reg(_rd, _rn, _rm)                                    \
  rv_emit_add(reg_temp, _rm, reg_c_cache);                                    \
  rv_emit_addi(reg_temp, reg_temp, -1);                                       \
  rv_emit_sub(_rd, reg_temp, _rn)

#define generate_op_add_reg(_rd, _rn, _rm)                                    \
  rv_emit_add(_rd, _rn, _rm)

#define generate_op_adc_reg(_rd, _rn, _rm)                                    \
  rv_emit_add(reg_temp, _rm, reg_c_cache);                                    \
  rv_emit_add(_rd, _rn, reg_temp)

#define generate_op_mov_reg(_rd, _rn, _rm)                                    \
  rv_emit_mv(_rd, _rm)

#define generate_op_mvn_reg(_rd, _rn, _rm)                                    \
  rv_emit_not(_rd, _rm)

/* ==================================================================
 * ALU operations — immediate operand
 * ================================================================== */

#define generate_op_imm_wrapper(name, _rd, _rn)                               \
  if(imm != 0)                                                                \
  {                                                                           \
    generate_load_imm(reg_a0, imm);                                           \
    generate_op_##name##_reg(_rd, _rn, reg_a0);                               \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_op_##name##_reg(_rd, _rn, rv_zero);                              \
  }

#define generate_op_and_imm(_rd, _rn)                                         \
  generate_alu_immu(andi, and, _rd, _rn, imm)

#define generate_op_orr_imm(_rd, _rn)                                         \
  generate_alu_immu(ori, or, _rd, _rn, imm)

#define generate_op_eor_imm(_rd, _rn)                                         \
  generate_alu_immu(xori, xor, _rd, _rn, imm)

#define generate_op_bic_imm(_rd, _rn)                                         \
  generate_alu_immu(andi, and, _rd, _rn, (~imm))

#define generate_op_sub_imm(_rd, _rn)                                         \
  generate_alu_imm(addi, add, _rd, _rn, (-(s32)imm))

#define generate_op_rsb_imm(_rd, _rn)                                         \
  if(imm != 0)                                                                \
  {                                                                           \
    generate_load_imm(reg_temp, imm);                                         \
    rv_emit_sub(_rd, reg_temp, _rn);                                          \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    rv_emit_sub(_rd, rv_zero, _rn);                                           \
  }

#define generate_op_sbc_imm(_rd, _rn)                                         \
  generate_op_imm_wrapper(sbc, _rd, _rn)

#define generate_op_rsc_imm(_rd, _rn)                                         \
  generate_op_imm_wrapper(rsc, _rd, _rn)

#define generate_op_add_imm(_rd, _rn)                                         \
  generate_alu_imm(addi, add, _rd, _rn, imm)

#define generate_op_adc_imm(_rd, _rn)                                         \
  generate_op_imm_wrapper(adc, _rd, _rn)

#define generate_op_mov_imm(_rd, _rn)                                         \
  generate_load_imm(_rd, imm)

#define generate_op_mvn_imm(_rd, _rn)                                         \
  generate_load_imm(_rd, (~imm))

/* ==================================================================
 * Logic flags: N and Z from result
 * ================================================================== */

#define generate_op_logic_flags(_rd)                                          \
  if(check_generate_n_flag)                                                   \
  {                                                                           \
    rv_emit_srli(reg_n_cache, _rd, 31);                                        \
  }                                                                           \
  if(check_generate_z_flag)                                                   \
  {                                                                           \
    rv_emit_sltiu(reg_z_cache, _rd, 1);                                        \
  }

/* ==================================================================
 * Flag-setting ALU — register operand
 * ================================================================== */

#define generate_op_ands_reg(_rd, _rn, _rm)                                   \
  rv_emit_and(_rd, _rn, _rm);                                                 \
  generate_op_logic_flags(_rd)

#define generate_op_orrs_reg(_rd, _rn, _rm)                                   \
  rv_emit_or(_rd, _rn, _rm);                                                  \
  generate_op_logic_flags(_rd)

#define generate_op_eors_reg(_rd, _rn, _rm)                                   \
  rv_emit_xor(_rd, _rn, _rm);                                                 \
  generate_op_logic_flags(_rd)

#define generate_op_bics_reg(_rd, _rn, _rm)                                   \
  rv_emit_not(reg_temp, _rm);                                                 \
  rv_emit_and(_rd, _rn, reg_temp);                                            \
  generate_op_logic_flags(_rd)

#define generate_op_subs_reg(_rd, _rn, _rm)                                   \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
    rv_emit_sltu(reg_c_cache, _rn, _rm);                                       \
    rv_emit_xori(reg_c_cache, reg_c_cache, 1);                                 \
  }                                                                           \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
    rv_emit_slt(reg_v_cache, _rn, _rm);                                        \
  }                                                                           \
  rv_emit_sub(_rd, _rn, _rm);                                                 \
  generate_op_logic_flags(_rd);                                               \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
    if(!check_generate_n_flag)                                                \
    {                                                                         \
      rv_emit_srli(reg_n_cache, _rd, 31);                                      \
    }                                                                         \
    rv_emit_xor(reg_v_cache, reg_v_cache, reg_n_cache);                        \
  }

#define generate_op_rsbs_reg(_rd, _rn, _rm)                                   \
  generate_op_subs_reg(_rd, _rm, _rn)

#define generate_op_sbcs_reg(_rd, _rn, _rm)                                   \
  rv_emit_xori(reg_temp, reg_c_cache, 1);                                      \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
    rv_emit_sltu(reg_c_cache, _rm, _rn);                                       \
    rv_emit_sltu(reg_rv, _rn, _rm);                                            \
    rv_emit_xori(reg_rv, reg_rv, 1);                                           \
    rv_movz(reg_c_cache, reg_rv, reg_temp);                                    \
  }                                                                           \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
    rv_emit_xor(reg_v_cache, _rn, _rm);                                        \
    rv_emit_not(reg_rv, _rm);                                                  \
  }                                                                           \
  rv_emit_sub(_rd, _rn, _rm);                                                 \
  rv_emit_sub(_rd, _rd, reg_temp);                                            \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
    rv_emit_xor(reg_rv, reg_rv, _rd);                                          \
    rv_emit_and(reg_v_cache, reg_v_cache, reg_rv);                             \
    rv_emit_srli(reg_v_cache, reg_v_cache, 31);                                \
  }                                                                           \
  generate_op_logic_flags(_rd)

#define generate_op_rscs_reg(_rd, _rn, _rm)                                   \
  generate_op_sbcs_reg(_rd, _rm, _rn)

#define generate_op_adds_reg(_rd, _rn, _rm)                                   \
  if(check_generate_c_flag | check_generate_v_flag)                           \
  {                                                                           \
    rv_emit_mv(reg_c_cache, _rn);                                              \
  }                                                                           \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
    rv_emit_slt(reg_v_cache, _rm, rv_zero);                                    \
  }                                                                           \
  rv_emit_add(_rd, _rn, _rm);                                                 \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
    rv_emit_slt(reg_a0, _rd, reg_c_cache);                                     \
    rv_emit_xor(reg_v_cache, reg_v_cache, reg_a0);                             \
  }                                                                           \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
    rv_emit_sltu(reg_c_cache, _rd, reg_c_cache);                               \
  }                                                                           \
  generate_op_logic_flags(_rd)

#define generate_op_adcs_reg(_rd, _rn, _rm)                                   \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
    rv_emit_xor(reg_v_cache, _rn, _rm);                                        \
    rv_emit_not(reg_v_cache, reg_v_cache);                                     \
    rv_emit_mv(reg_rv, _rn);                                                   \
  }                                                                           \
  rv_emit_add(reg_a2, _rn, _rm);                                              \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
    rv_emit_sltu(reg_temp, reg_a2, _rm);                                       \
  }                                                                           \
  rv_emit_add(_rd, reg_a2, reg_c_cache);                                       \
  if(check_generate_v_flag)                                                   \
  {                                                                           \
    rv_emit_xor(reg_rv, reg_rv, _rd);                                          \
    rv_emit_and(reg_v_cache, reg_rv, reg_v_cache);                             \
    rv_emit_srli(reg_v_cache, reg_v_cache, 31);                                \
  }                                                                           \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
    rv_emit_sltu(reg_c_cache, _rd, reg_c_cache);                               \
    rv_emit_or(reg_c_cache, reg_temp, reg_c_cache);                            \
  }                                                                           \
  generate_op_logic_flags(_rd)

#define generate_op_movs_reg(_rd, _rn, _rm)                                   \
  rv_emit_mv(_rd, _rm);                                                        \
  generate_op_logic_flags(_rd)

#define generate_op_mvns_reg(_rd, _rn, _rm)                                   \
  rv_emit_not(_rd, _rm);                                                       \
  generate_op_logic_flags(_rd)

#define generate_op_neg_reg(_rd, _rn, _rm)                                    \
  generate_op_subs_reg(_rd, rv_zero, _rm)

#define generate_op_muls_reg(_rd, _rn, _rm)                                   \
  rv_emit_mul(_rd, _rn, _rm);                                                 \
  generate_op_logic_flags(_rd)

#define generate_op_cmp_reg(_rd, _rn, _rm)                                    \
  generate_op_subs_reg(reg_temp, _rn, _rm)

#define generate_op_cmn_reg(_rd, _rn, _rm)                                    \
  generate_op_adds_reg(reg_temp, _rn, _rm)

#define generate_op_tst_reg(_rd, _rn, _rm)                                    \
  generate_op_ands_reg(reg_temp, _rn, _rm)

#define generate_op_teq_reg(_rd, _rn, _rm)                                    \
  generate_op_eors_reg(reg_temp, _rn, _rm)

/* ==================================================================
 * Flag-setting ALU — immediate operand
 * ================================================================== */

#define generate_op_ands_imm(_rd, _rn)                                        \
  generate_alu_immu(andi, and, _rd, _rn, imm);                                \
  generate_op_logic_flags(_rd)

#define generate_op_orrs_imm(_rd, _rn)                                        \
  generate_alu_immu(ori, or, _rd, _rn, imm);                                  \
  generate_op_logic_flags(_rd)

#define generate_op_eors_imm(_rd, _rn)                                        \
  generate_alu_immu(xori, xor, _rd, _rn, imm);                                \
  generate_op_logic_flags(_rd)

#define generate_op_bics_imm(_rd, _rn)                                        \
  generate_alu_immu(andi, and, _rd, _rn, (~imm));                             \
  generate_op_logic_flags(_rd)

#define generate_op_subs_imm(_rd, _rn)                                        \
  generate_op_imm_wrapper(subs, _rd, _rn)

#define generate_op_rsbs_imm(_rd, _rn)                                        \
  generate_op_imm_wrapper(rsbs, _rd, _rn)

#define generate_op_sbcs_imm(_rd, _rn)                                        \
  generate_op_imm_wrapper(sbcs, _rd, _rn)

#define generate_op_rscs_imm(_rd, _rn)                                        \
  generate_op_imm_wrapper(rscs, _rd, _rn)

#define generate_op_adds_imm(_rd, _rn)                                        \
  generate_op_imm_wrapper(adds, _rd, _rn)

#define generate_op_adcs_imm(_rd, _rn)                                        \
  generate_op_imm_wrapper(adcs, _rd, _rn)

#define generate_op_movs_imm(_rd, _rn)                                        \
  generate_load_imm(_rd, imm);                                                \
  generate_op_logic_flags(_rd)

#define generate_op_mvns_imm(_rd, _rn)                                        \
  generate_load_imm(_rd, (~imm));                                             \
  generate_op_logic_flags(_rd)

#define generate_op_cmp_imm(_rd, _rn)                                         \
  generate_op_imm_wrapper(cmp, _rd, _rn)

#define generate_op_cmn_imm(_rd, _rn)                                         \
  generate_op_imm_wrapper(cmn, _rd, _rn)

#define generate_op_tst_imm(_rd, _rn)                                         \
  generate_op_ands_imm(reg_temp, _rn)

#define generate_op_teq_imm(_rd, _rn)                                         \
  generate_op_eors_imm(reg_temp, _rn)

/* ==================================================================
 * ARM data processing dispatch macros
 * ================================================================== */

#define arm_generate_op_load_yes()                                            \
  generate_load_reg_pc(reg_a1, rn, 8)

#define arm_generate_op_load_no()

#define arm_op_check_yes()                                                    \
  check_load_reg_pc(arm_reg_a1, rn, 8)

#define arm_op_check_no()

#define arm_generate_op_reg_flags(name, load_op)                              \
  arm_decode_data_proc_reg(opcode);                                           \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
    generate_load_rm_sh(rm, flags);                                           \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_load_rm_sh(rm, no_flags);                                        \
  }                                                                           \
  arm_op_check_##load_op();                                                   \
  generate_op_##name##_reg(arm_to_mips_reg[rd], arm_to_mips_reg[rn],          \
   arm_to_mips_reg[rm])

#define arm_generate_op_reg(name, load_op)                                    \
  arm_decode_data_proc_reg(opcode);                                           \
  generate_load_rm_sh(rm, no_flags);                                          \
  arm_op_check_##load_op();                                                   \
  generate_op_##name##_reg(arm_to_mips_reg[rd], arm_to_mips_reg[rn],          \
   arm_to_mips_reg[rm])

#define arm_generate_op_imm(name, load_op)                                    \
  arm_decode_data_proc_imm(opcode);                                           \
  ror(imm, imm, imm_ror);                                                     \
  arm_op_check_##load_op();                                                   \
  generate_op_##name##_imm(arm_to_mips_reg[rd], arm_to_mips_reg[rn])

#define arm_generate_op_imm_flags(name, load_op)                              \
  arm_decode_data_proc_imm(opcode);                                           \
  ror(imm, imm, imm_ror);                                                     \
  if(check_generate_c_flag && (imm_ror != 0))                                 \
  {                                                                           \
    rv_emit_addi(reg_c_cache, rv_zero, ((imm) >> 31));                         \
  }                                                                           \
  arm_op_check_##load_op();                                                   \
  generate_op_##name##_imm(arm_to_mips_reg[rd], arm_to_mips_reg[rn])

#define arm_data_proc(name, type, flags_op)                                   \
{                                                                             \
  arm_generate_op_##type(name, yes);                                          \
  check_store_reg_pc_##flags_op(rd);                                          \
}

#define arm_data_proc_test(name, type)                                        \
{                                                                             \
  arm_generate_op_##type(name, yes);                                          \
}

#define arm_data_proc_unary(name, type, flags_op)                             \
{                                                                             \
  arm_generate_op_##type(name, no);                                           \
  check_store_reg_pc_##flags_op(rd);                                          \
}

/* ==================================================================
 * ARM multiply
 * ================================================================== */

#define arm_multiply_flags_yes(_rd)                                           \
  generate_op_logic_flags(_rd)

#define arm_multiply_flags_no(_rd)

#define arm_multiply_add_no()                                                 \
  rv_emit_mul(arm_to_mips_reg[rd], arm_to_mips_reg[rm], arm_to_mips_reg[rs])

#define arm_multiply_add_yes()                                                \
  rv_emit_mul(reg_temp, arm_to_mips_reg[rm], arm_to_mips_reg[rs]);            \
  rv_emit_add(arm_to_mips_reg[rd], reg_temp, arm_to_mips_reg[rn])

#define arm_multiply(add_op, flags)                                           \
{                                                                             \
  arm_decode_multiply();                                                      \
  arm_multiply_add_##add_op();                                                \
  arm_multiply_flags_##flags(arm_to_mips_reg[rd]);                            \
}

#define arm_multiply_long_flags_yes(_rdlo, _rdhi)                             \
  rv_emit_sltiu(reg_z_cache, _rdlo, 1);                                        \
  rv_emit_sltiu(reg_a0, _rdhi, 1);                                            \
  rv_emit_and(reg_z_cache, reg_z_cache, reg_a0);                               \
  rv_emit_srli(reg_n_cache, _rdhi, 31)

#define arm_multiply_long_flags_no(_rdlo, _rdhi)

/* Non-accumulate (UMULL/SMULL): generate_multiply_*() computes the product
 * into scratch a0/a1 (so rdlo/rdhi may alias rm/rs safely), then we copy it
 * out to rdlo/rdhi.
 * Accumulate (UMLAL/SMLAL): generate_multiply_*_add() accumulates directly
 * into rdlo/rdhi using temp/rv and leaves a0/a1 untouched, so the copy-out
 * MUST NOT run — otherwise it clobbers the accumulated result with garbage. */
#define arm_multiply_long_add_yes(name)                                       \
  generate_multiply_##name()

#define arm_multiply_long_add_no(name)                                        \
  generate_multiply_##name();                                                 \
  rv_emit_mv(arm_to_mips_reg[rdlo], reg_a0);                                  \
  rv_emit_mv(arm_to_mips_reg[rdhi], reg_a1)

#define arm_multiply_long(name, add_op, flags)                                \
{                                                                             \
  arm_decode_multiply_long();                                                 \
  arm_multiply_long_add_##add_op(name);                                       \
  arm_multiply_long_flags_##flags(arm_to_mips_reg[rdlo],                      \
   arm_to_mips_reg[rdhi]);                                                    \
}

/* ==================================================================
 * PSR read/write
 * ================================================================== */

u32 execute_spsr_restore_body(u32 address)
{
  set_cpu_mode(cpu_modes[reg[REG_CPSR] & 0xF]);
  if((io_registers[REG_IE] & io_registers[REG_IF]) &&
   io_registers[REG_IME] && ((reg[REG_CPSR] & 0x80) == 0))
  {
    REG_MODE(MODE_IRQ)[6] = address + 4;
    REG_SPSR(MODE_IRQ) = reg[REG_CPSR];
    reg[REG_CPSR] = 0xD2;
    address = 0x00000018;
    set_cpu_mode(MODE_IRQ);
  }
  if(reg[REG_CPSR] & 0x20)
    address |= 0x01;
  return address;
}

u32 execute_store_cpsr_body(u32 _cpsr, u32 address)
{
  set_cpu_mode(cpu_modes[_cpsr & 0xF]);
  if((io_registers[REG_IE] & io_registers[REG_IF]) &&
   io_registers[REG_IME] && ((_cpsr & 0x80) == 0))
  {
    REG_MODE(MODE_IRQ)[6] = address + 4;
    REG_SPSR(MODE_IRQ) = _cpsr;
    reg[REG_CPSR] = 0xD2;
    set_cpu_mode(MODE_IRQ);
    return 0x00000018;
  }
  return 0;
}

#define arm_psr_read(op_type, psr_reg)                                        \
  generate_function_call(execute_read_##psr_reg);                             \
  rv_emit_mv(reg_rv, reg_a0);                                                \
  generate_store_reg(reg_rv, rd)

#define arm_psr_load_new_reg()                                                \
  generate_load_reg(reg_a0, rm)

#define arm_psr_load_new_imm()                                                \
  generate_load_imm(reg_a0, imm)

#define arm_psr_store_spsr()                                                  \
  generate_load_imm(reg_a1, spsr_masks[psr_pfield]);                          \
  generate_function_call(execute_store_spsr)

#define arm_psr_store_cpsr()                                                  \
  generate_load_pc(reg_a1, (pc));                                             \
  generate_function_call(execute_store_cpsr);                                 \
  generate_raw_u32(cpsr_masks[psr_pfield][0]);                                \
  generate_raw_u32(cpsr_masks[psr_pfield][1])

#define arm_psr_store(op_type, psr_reg)                                       \
  arm_psr_load_new_##op_type();                                               \
  arm_psr_store_##psr_reg()

#define arm_psr(op_type, transfer_type, psr_reg)                              \
{                                                                             \
  arm_decode_psr_##op_type(opcode);                                           \
  arm_psr_##transfer_type(op_type, psr_reg);                                  \
}

/* ==================================================================
 * Memory access — ARM mode
 *
 * On MIPS, memory handlers are called via jal with arguments
 * in delay slots. On RISC-V, set up args first, then call.
 * Return value is in a0; copy to reg_rv for compatibility.
 * ================================================================== */

#define arm_access_memory_load(mem_type)                                      \
  cycle_count += 2;                                                           \
  generate_load_pc(reg_a1, (pc));   /* exact PC for open-bus/BIOS reads */    \
  generate_function_call(execute_load_##mem_type);                            \
  rv_emit_mv(reg_rv, reg_a0);                                                \
  generate_store_reg(reg_rv, rd);                                             \
  check_store_reg_pc_no_flags(rd)

#define arm_access_memory_store(mem_type)                                     \
  cycle_count++;                                                              \
  generate_load_pc(reg_a2, (pc + 4));                                         \
  generate_load_reg_pc(reg_a1, rd, 12);                                       \
  generate_function_call(execute_store_##mem_type)

/* Address computation: register offset, pre-indexed */
#define arm_access_memory_reg_pre_up()                                        \
  rv_emit_add(reg_a0, arm_to_mips_reg[rn], arm_to_mips_reg[rm])

#define arm_access_memory_reg_pre_down()                                      \
  rv_emit_sub(reg_a0, arm_to_mips_reg[rn], arm_to_mips_reg[rm])

#define arm_access_memory_reg_pre(adjust_dir)                                 \
  check_load_reg_pc(arm_reg_a0, rn, 8);                                       \
  arm_access_memory_reg_pre_##adjust_dir()

#define arm_access_memory_reg_pre_wb(adjust_dir)                              \
  arm_access_memory_reg_pre(adjust_dir);                                      \
  generate_store_reg(reg_a0, rn)

/* Post-indexed register offset */
#define arm_access_memory_reg_post_up()                                       \
  rv_emit_add(arm_to_mips_reg[rn], arm_to_mips_reg[rn], arm_to_mips_reg[rm])

#define arm_access_memory_reg_post_down()                                     \
  rv_emit_sub(arm_to_mips_reg[rn], arm_to_mips_reg[rn], arm_to_mips_reg[rm])

#define arm_access_memory_reg_post(adjust_dir)                                \
  generate_load_reg(reg_a0, rn);                                              \
  arm_access_memory_reg_post_##adjust_dir()

/* Immediate offset */
#define arm_access_memory_imm_pre_up()                                        \
  rv_addi_safe(reg_a0, arm_to_mips_reg[rn], offset)

#define arm_access_memory_imm_pre_down()                                      \
  rv_addi_safe(reg_a0, arm_to_mips_reg[rn], -(s32)offset)

#define arm_access_memory_imm_pre(adjust_dir)                                 \
  check_load_reg_pc(arm_reg_a0, rn, 8);                                       \
  arm_access_memory_imm_pre_##adjust_dir()

#define arm_access_memory_imm_pre_wb(adjust_dir)                              \
  arm_access_memory_imm_pre(adjust_dir);                                      \
  generate_store_reg(reg_a0, rn)

#define arm_access_memory_imm_post_up()                                       \
  rv_addi_safe(arm_to_mips_reg[rn], arm_to_mips_reg[rn], offset)

#define arm_access_memory_imm_post_down()                                     \
  rv_addi_safe(arm_to_mips_reg[rn], arm_to_mips_reg[rn], -(s32)offset)

#define arm_access_memory_imm_post(adjust_dir)                                \
  generate_load_reg(reg_a0, rn);                                              \
  arm_access_memory_imm_post_##adjust_dir()

/* Data transfer decode + address computation */
#define arm_data_trans_reg(adjust_op, adjust_dir)                             \
  arm_decode_data_trans_reg();                                                \
  generate_load_offset_sh(rm);                                                \
  arm_access_memory_reg_##adjust_op(adjust_dir)

#define arm_data_trans_imm(adjust_op, adjust_dir)                             \
  arm_decode_data_trans_imm();                                                \
  arm_access_memory_imm_##adjust_op(adjust_dir)

#define arm_data_trans_half_reg(adjust_op, adjust_dir)                        \
  arm_decode_half_trans_r();                                                  \
  arm_access_memory_reg_##adjust_op(adjust_dir)

#define arm_data_trans_half_imm(adjust_op, adjust_dir)                        \
  arm_decode_half_trans_of();                                                 \
  arm_access_memory_imm_##adjust_op(adjust_dir)

#define arm_access_memory(access_type, direction, adjust_op, mem_type,        \
 offset_type)                                                                 \
{                                                                             \
  arm_data_trans_##offset_type(adjust_op, direction);                         \
  arm_access_memory_##access_type(mem_type);                                  \
}

/* ==================================================================
 * Block memory — ARM mode (generic path, no SP optimization)
 * ================================================================== */

#define word_bit_count(word)                                                  \
  (bit_count[word >> 8] + bit_count[word & 0xFF])

#define arm_block_memory_load()                                               \
  generate_load_pc(reg_a1, (pc));   /* exact PC for open-bus/BIOS reads */    \
  generate_function_call(execute_aligned_load32);                             \
  rv_emit_mv(reg_rv, reg_a0);                                                \
  generate_store_reg(reg_rv, i)

#define arm_block_memory_store()                                              \
  generate_load_reg_pc(reg_a1, i, 8);                                         \
  generate_function_call(execute_aligned_store32)

#define arm_block_memory_final_load(writeback_type)                           \
  arm_block_memory_load()

#define arm_block_memory_final_store(writeback_type)                          \
  generate_load_pc(reg_a2, (pc + 4));                                         \
  generate_load_reg(reg_a1, i);                                               \
  arm_block_memory_writeback_post_store(writeback_type);                      \
  generate_function_call(execute_store_u32)

#define arm_block_memory_adjust_pc_store()

#define arm_block_memory_adjust_pc_load()                                     \
  if(reg_list & 0x8000)                                                       \
  {                                                                           \
    generate_mov(reg_a0, reg_rv);                                             \
    generate_indirect_branch_arm();                                           \
  }

#define arm_block_memory_offset_down_a()                                      \
  rv_addi_safe(reg_a2, base_reg, (-((word_bit_count(reg_list) * 4) - 4)))

#define arm_block_memory_offset_down_b()                                      \
  rv_addi_safe(reg_a2, base_reg, (word_bit_count(reg_list) * -4))

#define arm_block_memory_offset_no()                                          \
  rv_emit_mv(reg_a2, base_reg)

#define arm_block_memory_offset_up()                                          \
  rv_emit_addi(reg_a2, base_reg, 4)

#define arm_block_memory_writeback_down()                                     \
  rv_addi_safe(base_reg, base_reg, (-(word_bit_count(reg_list) * 4)))

#define arm_block_memory_writeback_up()                                       \
  rv_addi_safe(base_reg, base_reg, (word_bit_count(reg_list) * 4))

#define arm_block_memory_writeback_no()

#define arm_block_memory_writeback_post_load(writeback_type)
#define arm_block_memory_writeback_pre_load(writeback_type)                   \
  if(!((reg_list >> rn) & 0x01))                                              \
  {                                                                           \
    arm_block_memory_writeback_##writeback_type();                            \
  }

#define arm_block_memory_writeback_pre_store(writeback_type)
#define arm_block_memory_writeback_post_store(writeback_type)                 \
  arm_block_memory_writeback_##writeback_type()

#define arm_block_memory(access_type, offset_type, writeback_type, s_bit)     \
{                                                                             \
  arm_decode_block_trans();                                                   \
  u32 i;                                                                      \
  u32 offset = 0;                                                             \
  u32 base_reg = arm_to_mips_reg[rn];                                         \
                                                                              \
  arm_block_memory_offset_##offset_type();                                    \
  arm_block_memory_writeback_pre_##access_type(writeback_type);               \
                                                                              \
  /* Always use generic (function-call) path */                               \
  emit_align_reg(reg_a2, 2);                                                  \
  for(i = 0; i < 16; i++)                                                     \
  {                                                                           \
    if((reg_list >> i) & 0x01)                                                \
    {                                                                         \
      cycle_count++;                                                          \
      rv_addi_safe(reg_a0, reg_a2, offset);                                   \
      if(reg_list & ~((2 << i) - 1))                                          \
      {                                                                       \
        arm_block_memory_##access_type();                                     \
        offset += 4;                                                          \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        arm_block_memory_final_##access_type(writeback_type);                 \
        break;                                                                \
      }                                                                       \
    }                                                                         \
  }                                                                           \
  arm_block_memory_adjust_pc_##access_type();                                 \
}

/* ARM swap */
#define arm_swap(type)                                                        \
{                                                                             \
  arm_decode_swap();                                                          \
  cycle_count += 3;                                                           \
  generate_load_reg(reg_a0, rn);                                              \
  generate_load_pc(reg_a1, (pc));   /* exact PC for open-bus/BIOS reads */    \
  generate_function_call(execute_load_##type);                                \
  rv_emit_mv(reg_rv, reg_a0);                                                \
  generate_load_reg(reg_a0, rn);                                              \
  generate_load_reg(reg_a1, rm);                                              \
  generate_function_call(execute_store_##type);                               \
  generate_store_reg(reg_rv, rd);                                             \
}

/* ==================================================================
 * Thumb mode macros
 * ================================================================== */

#define thumb_generate_op_load_yes(_rs)                                       \
  generate_load_reg(reg_a1, _rs)

#define thumb_generate_op_load_no(_rs)

#define thumb_generate_op_reg(name, _rd, _rs, _rn)                            \
  generate_op_##name##_reg(arm_to_mips_reg[_rd],                              \
   arm_to_mips_reg[_rs], arm_to_mips_reg[_rn])

#define thumb_generate_op_imm(name, _rd, _rs, _rn)                            \
  generate_op_##name##_imm(arm_to_mips_reg[_rd], arm_to_mips_reg[_rs])

#define thumb_data_proc(type, name, rn_type, _rd, _rs, _rn)                   \
{                                                                             \
  thumb_decode_##type();                                                      \
  thumb_generate_op_##rn_type(name, _rd, _rs, _rn);                           \
}

#define thumb_data_proc_test(type, name, rn_type, _rs, _rn)                   \
{                                                                             \
  thumb_decode_##type();                                                      \
  thumb_generate_op_##rn_type(name, 0, _rs, _rn);                             \
}

#define thumb_data_proc_unary(type, name, rn_type, _rd, _rn)                  \
{                                                                             \
  thumb_decode_##type();                                                      \
  thumb_generate_op_##rn_type(name, _rd, 0, _rn);                             \
}

#define check_store_reg_pc_thumb(_rd)                                         \
  if(_rd == REG_PC)                                                           \
  {                                                                           \
    generate_indirect_branch_cycle_update(thumb);                             \
  }

#define thumb_data_proc_hi(name)                                              \
{                                                                             \
  thumb_decode_hireg_op();                                                    \
  u32 dest_rd = rd;                                                           \
  check_load_reg_pc(arm_reg_a0, rs, 4);                                       \
  check_load_reg_pc(arm_reg_a1, rd, 4);                                       \
  generate_op_##name##_reg(arm_to_mips_reg[dest_rd], arm_to_mips_reg[rd],     \
   arm_to_mips_reg[rs]);                                                      \
  check_store_reg_pc_thumb(dest_rd);                                          \
}

#define thumb_data_proc_test_hi(name)                                         \
{                                                                             \
  thumb_decode_hireg_op();                                                    \
  check_load_reg_pc(arm_reg_a0, rs, 4);                                       \
  check_load_reg_pc(arm_reg_a1, rd, 4);                                       \
  generate_op_##name##_reg(reg_temp, arm_to_mips_reg[rd],                     \
   arm_to_mips_reg[rs]);                                                      \
}

#define thumb_data_proc_mov_hi()                                              \
{                                                                             \
  thumb_decode_hireg_op();                                                    \
  check_load_reg_pc(arm_reg_a0, rs, 4);                                       \
  rv_emit_mv(arm_to_mips_reg[rd], arm_to_mips_reg[rs]);                       \
  check_store_reg_pc_thumb(rd);                                               \
}

/* Thumb constant/address loading */
#define thumb_load_pc_pool_const(rd, value)                                   \
  generate_load_imm(arm_to_mips_reg[rd], (value))

#define thumb_load_pc(_rd)                                                    \
{                                                                             \
  thumb_decode_imm();                                                         \
  generate_load_pc(arm_to_mips_reg[_rd], (((pc & ~2) + 4) + (imm * 4)));     \
}

#define thumb_load_sp(_rd)                                                    \
{                                                                             \
  thumb_decode_imm();                                                         \
  rv_addi_safe(arm_to_mips_reg[_rd], reg_r13, (imm * 4));                    \
}

#define thumb_adjust_sp_up()                                                  \
  rv_addi_safe(reg_r13, reg_r13, (imm * 4))

#define thumb_adjust_sp_down()                                                \
  rv_addi_safe(reg_r13, reg_r13, -(s32)(imm * 4))

#define thumb_adjust_sp(direction)                                            \
{                                                                             \
  thumb_decode_add_sp();                                                      \
  thumb_adjust_sp_##direction();                                              \
}

/* Thumb shifts */
#define thumb_generate_shift_imm(name)                                        \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
    generate_shift_imm_##name##_flags(rd, rs, imm);                           \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_shift_imm_##name##_no_flags(rd, rs, imm);                        \
  }                                                                           \
  if(rs != rd)                                                                \
  {                                                                           \
    rv_emit_mv(arm_to_mips_reg[rd], arm_to_mips_reg[rs]);                     \
  }

#define thumb_generate_shift_reg(name)                                        \
{                                                                             \
  u32 original_rd = rd;                                                       \
  if(check_generate_c_flag)                                                   \
  {                                                                           \
    generate_shift_reg_##name##_flags(rd, rs);                                \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_shift_reg_##name##_no_flags(rd, rs);                             \
  }                                                                           \
  rv_emit_mv(arm_to_mips_reg[original_rd], reg_a0);                           \
}

#define thumb_shift(decode_type, op_type, value_type)                         \
{                                                                             \
  thumb_decode_##decode_type();                                               \
  thumb_generate_shift_##value_type(op_type);                                 \
  generate_op_logic_flags(arm_to_mips_reg[rd]);                               \
}

/* Thumb memory access */
#define thumb_access_memory_load(mem_type, reg_rd)                            \
  cycle_count += 2;                                                           \
  generate_load_pc(reg_a1, (pc));   /* exact PC for open-bus/BIOS reads */    \
  generate_function_call(execute_load_##mem_type);                            \
  rv_emit_mv(reg_rv, reg_a0);                                                \
  generate_store_reg(reg_rv, reg_rd)

#define thumb_access_memory_store(mem_type, reg_rd)                           \
  cycle_count++;                                                              \
  generate_load_pc(reg_a2, (pc + 2));                                         \
  generate_load_reg(reg_a1, reg_rd);                                          \
  generate_function_call(execute_store_##mem_type)

#define thumb_access_memory_generate_address_pc_relative(offset, reg_rb,      \
 reg_ro)                                                                      \
  generate_load_pc(reg_a0, (offset))

#define thumb_access_memory_generate_address_reg_imm(offset, reg_rb, reg_ro)  \
  rv_addi_safe(reg_a0, arm_to_mips_reg[reg_rb], (offset))

#define thumb_access_memory_generate_address_reg_imm_sp(offset, reg_rb, reg_ro) \
  rv_addi_safe(reg_a0, arm_to_mips_reg[reg_rb], (offset * 4))

#define thumb_access_memory_generate_address_reg_reg(offset, reg_rb, reg_ro)  \
  rv_emit_add(reg_a0, arm_to_mips_reg[reg_rb], arm_to_mips_reg[reg_ro])

#define thumb_access_memory(access_type, op_type, reg_rd, reg_rb, reg_ro,     \
 address_type, offset, mem_type)                                              \
{                                                                             \
  thumb_decode_##op_type();                                                   \
  thumb_access_memory_generate_address_##address_type(offset, reg_rb,         \
   reg_ro);                                                                   \
  thumb_access_memory_##access_type(mem_type, reg_rd);                        \
}

/* Thumb block memory (generic path) */
#define thumb_block_address_preadjust_no(base_reg)                            \
  rv_emit_mv(reg_a2, arm_to_mips_reg[base_reg])

#define thumb_block_address_preadjust_down(base_reg)                          \
  rv_addi_safe(reg_a2, arm_to_mips_reg[base_reg],                            \
   -(bit_count[reg_list] * 4));                                               \
  rv_emit_mv(arm_to_mips_reg[base_reg], reg_a2)

#define thumb_block_address_preadjust_push_lr(base_reg)                       \
  rv_addi_safe(reg_a2, arm_to_mips_reg[base_reg],                            \
   -((bit_count[reg_list] + 1) * 4));                                         \
  rv_emit_mv(arm_to_mips_reg[base_reg], reg_a2)

#define thumb_block_address_postadjust_no(base_reg)

#define thumb_block_address_postadjust_up(base_reg)                           \
  rv_addi_safe(arm_to_mips_reg[base_reg], reg_a2,                            \
   (bit_count[reg_list] * 4))

#define thumb_block_address_postadjust_pop_pc(base_reg)                       \
  rv_addi_safe(arm_to_mips_reg[base_reg], reg_a2,                            \
   ((bit_count[reg_list] * 4) + 4))

#define thumb_block_address_postadjust_push_lr(base_reg)

#define thumb_block_memory_load()                                             \
  generate_load_pc(reg_a1, (pc));   /* exact PC for open-bus/BIOS reads */    \
  generate_function_call(execute_aligned_load32);                             \
  rv_emit_mv(reg_rv, reg_a0);                                                \
  generate_store_reg(reg_rv, i)

#define thumb_block_memory_store()                                            \
  generate_load_reg(reg_a1, i);                                               \
  generate_function_call(execute_aligned_store32)

#define thumb_block_memory_final_load()                                       \
  thumb_block_memory_load()

#define thumb_block_memory_final_store()                                      \
  generate_load_pc(reg_a2, (pc + 2));                                         \
  generate_load_reg(reg_a1, i);                                               \
  generate_function_call(execute_store_u32)

#define thumb_block_memory_final_no(access_type)                              \
  thumb_block_memory_final_##access_type()

#define thumb_block_memory_final_up(access_type)                              \
  thumb_block_memory_final_##access_type()

#define thumb_block_memory_final_down(access_type)                            \
  thumb_block_memory_final_##access_type()

#define thumb_block_memory_final_push_lr(access_type)                         \
  thumb_block_memory_##access_type()

#define thumb_block_memory_final_pop_pc(access_type)                          \
  thumb_block_memory_##access_type()

#define thumb_block_memory_extra_no()
#define thumb_block_memory_extra_up()
#define thumb_block_memory_extra_down()

#define thumb_block_memory_extra_push_lr()                                    \
  rv_addi_safe(reg_a0, reg_a2, (bit_count[reg_list] * 4));                   \
  generate_load_reg(reg_a1, REG_LR);                                         \
  generate_function_call(execute_aligned_store32)

#define thumb_block_memory_extra_pop_pc()                                     \
  rv_addi_safe(reg_a0, reg_a2, (bit_count[reg_list] * 4));                   \
  generate_load_pc(reg_a1, (pc));   /* exact PC for open-bus/BIOS reads */    \
  generate_function_call(execute_aligned_load32);                             \
  /* Return value in a0 = loaded PC */                                        \
  generate_indirect_branch_cycle_update(thumb)

#define thumb_block_memory(access_type, pre_op, post_op, base_reg)            \
{                                                                             \
  thumb_decode_rlist();                                                       \
  u32 i;                                                                      \
  u32 offset = 0;                                                             \
                                                                              \
  thumb_block_address_preadjust_##pre_op(base_reg);                           \
  thumb_block_address_postadjust_##post_op(base_reg);                         \
                                                                              \
  /* Always use generic path */                                               \
  emit_align_reg(reg_a2, 2);                                                  \
  for(i = 0; i < 8; i++)                                                      \
  {                                                                           \
    if((reg_list >> i) & 0x01)                                                \
    {                                                                         \
      cycle_count++;                                                          \
      rv_addi_safe(reg_a0, reg_a2, offset);                                   \
      if(reg_list & ~((2 << i) - 1))                                          \
      {                                                                       \
        thumb_block_memory_##access_type();                                   \
        offset += 4;                                                          \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        thumb_block_memory_final_##post_op(access_type);                      \
        break;                                                                \
      }                                                                       \
    }                                                                         \
  }                                                                           \
  thumb_block_memory_extra_##post_op();                                       \
}

/* ==================================================================
 * Conditional branch (Thumb)
 * ================================================================== */

#define thumb_conditional_branch(condition)                                   \
{                                                                             \
  generate_condition_##condition();                                           \
  generate_branch_no_cycle_update(                                            \
   block_exits[block_exit_position].branch_source,                            \
   block_exits[block_exit_position].branch_target);                           \
  generate_branch_patch_conditional(backpatch_address, translation_ptr);      \
  block_exit_position++;                                                      \
}

/* ==================================================================
 * ARM conditional block header
 * ================================================================== */

#define arm_conditional_block_header()                                        \
  generate_condition()

/* ==================================================================
 * Branch / call / SWI macros
 * ================================================================== */

#define arm_b()                                                               \
  generate_branch()

#define arm_bl()                                                              \
  generate_load_pc(reg_r14, (pc + 4));                                        \
  generate_branch()

#define arm_bx()                                                              \
  arm_decode_branchx(opcode);                                                 \
  generate_load_reg_pc(reg_a0, rn, 8);                                        \
  generate_indirect_branch_dual()

#define arm_swi()                                                             \
  generate_load_pc(reg_a0, (pc + 4));                                         \
  generate_function_call(execute_swi);                                        \
  generate_branch()

#define thumb_b()                                                             \
  generate_branch_cycle_update(                                               \
   block_exits[block_exit_position].branch_source,                            \
   block_exits[block_exit_position].branch_target);                           \
  block_exit_position++

#define thumb_bl()                                                            \
  generate_load_pc(reg_r14, ((pc + 2) | 0x01));                               \
  generate_branch_cycle_update(                                               \
   block_exits[block_exit_position].branch_source,                            \
   block_exits[block_exit_position].branch_target);                           \
  block_exit_position++

#define thumb_blh()                                                           \
{                                                                             \
  thumb_decode_branch();                                                      \
  rv_addi_safe(reg_a0, reg_r14, (offset * 2));                                \
  generate_load_pc(reg_r14, ((pc + 2) | 0x01));                               \
  generate_indirect_branch_cycle_update(thumb);                               \
}

#define thumb_bx()                                                            \
{                                                                             \
  thumb_decode_hireg_op();                                                    \
  generate_load_reg_pc(reg_a0, rs, 4);                                        \
  generate_indirect_branch_cycle_update(dual);                                \
}

#define thumb_swi()                                                           \
  generate_load_pc(reg_a0, (pc + 2));                                         \
  generate_function_call(execute_swi);                                        \
  generate_branch_cycle_update(                                               \
   block_exits[block_exit_position].branch_source,                            \
   block_exits[block_exit_position].branch_target);                           \
  block_exit_position++

/* ==================================================================
 * HLE division (software divide optimization)
 * RISC-V has hardware div/rem.
 * ================================================================== */

/* SWI 6 Div: num=r0, den=r1 -> r0=num/den, r1=num%den, r3=|num/den|.
 * The remainder MUST be computed from the ORIGINAL r0: div writes r0 first,
 * so compute rem into a temp before overwriting the dividend. */
#define arm_hle_div(cpu_mode)                                                 \
  rv_emit_rem(reg_temp, reg_r0, reg_r1);                                      \
  rv_emit_div(reg_r0, reg_r0, reg_r1);                                        \
  rv_emit_mv(reg_r1, reg_temp);                                               \
  rv_emit_srai(reg_a0, reg_r0, 31);                                           \
  rv_emit_xor(reg_r3, reg_r0, reg_a0);                                        \
  rv_emit_sub(reg_r3, reg_r3, reg_a0)

/* SWI 7 DivArm: num=r1, den=r0 -> r0=num/den, r1=num%den, r3=|num/den|.
 * Same hazard: den (r0) is overwritten by the quotient, so compute the
 * remainder from the ORIGINAL r0 into a temp first. */
#define arm_hle_div_arm(cpu_mode)                                             \
  rv_emit_rem(reg_temp, reg_r1, reg_r0);                                      \
  rv_emit_div(reg_r0, reg_r1, reg_r0);                                        \
  rv_emit_mv(reg_r1, reg_temp);                                               \
  rv_emit_srai(reg_a0, reg_r0, 31);                                           \
  rv_emit_xor(reg_r3, reg_r0, reg_a0);                                        \
  rv_emit_sub(reg_r3, reg_r3, reg_a0)

/* ==================================================================
 * Miscellaneous
 * ================================================================== */

#define generate_translation_gate(type)                                       \
  generate_load_pc(reg_a0, pc);                                               \
  generate_indirect_branch_no_cycle_update(type)

#define generate_update_pc_reg()                                              \
  generate_load_pc(reg_a0, pc);                                               \
  rv_emit_sw(reg_a0, reg_base, (REG_PC * 4))

#define thumb_process_cheats()                                                \
  generate_function_call(riscv_cheat_hook)

#define arm_process_cheats()                                                  \
  generate_function_call(riscv_cheat_hook)

/* Tracing (disabled by default) */
#ifdef TRACE_INSTRUCTIONS
  void trace_instruction(u32 pc, u32 mode)
  {
    if (mode)
      printf("Executed arm %x\n", pc);
    else
      printf("Executed thumb %x\n", pc);
  }
  #define emit_trace_instruction(pc, mode)                                    \
    emit_save_regs(false);                                                    \
    generate_load_imm(reg_a0, pc);                                            \
    generate_load_imm(reg_a1, mode);                                          \
    genccall(&trace_instruction);                                             \
    emit_restore_regs(false)
  #define emit_trace_thumb_instruction(pc) emit_trace_instruction(pc, 0)
  #define emit_trace_arm_instruction(pc)   emit_trace_instruction(pc, 1)
#else
  #define emit_trace_thumb_instruction(pc)
  #define emit_trace_arm_instruction(pc)
#endif

/* ==================================================================
 * Register save/restore offsets
 * ================================================================== */

#define ReOff_RegPC    (REG_PC    * 4)
#define ReOff_CPSR     (REG_CPSR  * 4)
#define ReOff_SaveR1   (REG_SAVE  * 4)
#define ReOff_SaveR2   (REG_SAVE2 * 4)
#define ReOff_SaveR3   (REG_SAVE3 * 4)
#define ReOff_OamUpd   (OAM_UPDATED*4)
#define ReOff_GP_Save  (REG_SAVE5 * 4)

/* Save all GBA registers from host regs to memory */
#define emit_save_regs(save_a2) {                                             \
  int _i;                                                                     \
  for (_i = 0; _i < 15; _i++) {                                              \
    rv_emit_sw(arm_to_mips_reg[_i], reg_base, 4 * _i);                       \
  }                                                                           \
  if (save_a2) {                                                              \
    rv_emit_sw(reg_a2, reg_base, ReOff_SaveR2);                               \
  }                                                                           \
}

/* Restore all GBA registers from memory to host regs */
#define emit_restore_regs(restore_a2) {                                       \
  int _i;                                                                     \
  if (restore_a2) {                                                           \
    rv_emit_lw(reg_a2, reg_base, ReOff_SaveR2);                               \
  }                                                                           \
  for (_i = 0; _i < 15; _i++) {                                              \
    rv_emit_lw(arm_to_mips_reg[_i], reg_base, 4 * _i);                       \
  }                                                                           \
}

/* Call a C function from within a generated stub */
#define emit_mem_call_ds(fnptr, mask)                                         \
  rv_emit_sw(rv_ra, reg_base, ReOff_SaveR1);                                  \
  emit_save_regs(true);                                                       \
  rv_emit_andi(reg_a0, reg_a0, (mask));                                       \
  genccall(fnptr);                                                            \
  rv_emit_lw(rv_ra, reg_base, ReOff_SaveR1);                                  \
  emit_restore_regs(true)

#define emit_mem_call(fnptr, mask)                                            \
  emit_mem_call_ds(fnptr, mask);                                              \
  rv_emit_ret()

/* Pointer table to stubs */
u32* openld_core_ptrs[11];

const u8 ldopmap[6][2] = { {0, 1}, {1, 2}, {2, 4}, {4, 6}, {6, 10}, {10, 11} };
const u8 ldhldrtbl[11] = {0, 1, 2, 2, 3, 3, 4, 4, 4, 4, 5};

/* These will need adaptation for RISC-V address calculation */
#define ld_phndlr_branch(memop) \
  (((u32*)&rom_translation_cache[ldhldrtbl[(memop)]*16*4]) - ((u32*)translation_ptr + 1))

#define st_phndlr_branch(memop) \
  (((u32*)&rom_translation_cache[((memop) + 6)*16*4]) - ((u32*)translation_ptr + 1))

#define branch_handlerid(phndlrid) \
  (((u32*)&rom_translation_cache[(phndlrid)*16*4]) - ((u32*)translation_ptr + 1))

#define branch_offset(ptr) \
  (((u32*)ptr) - ((u32*)translation_ptr + 1))

/* ==================================================================
 * DIV/REM instruction emission (M extension)
 * ================================================================== */

#define rv_emit_div(rd, rs1, rs2)   rv_emit(rv_enc_r(0x01, rs2, rs1, RV_DIV,  rd, RV_OP_REG))
#define rv_emit_divu(rd, rs1, rs2)  rv_emit(rv_enc_r(0x01, rs2, rs1, RV_DIVU, rd, RV_OP_REG))
#define rv_emit_rem(rd, rs1, rs2)   rv_emit(rv_enc_r(0x01, rs2, rs1, RV_REM,  rd, RV_OP_REG))
#define rv_emit_remu(rd, rs1, rs2)  rv_emit(rv_enc_r(0x01, rs2, rs1, RV_REMU, rd, RV_OP_REG))

/* Memory access stub generation (minimal for initial port)
 *
 * The full MIPS version generates optimized per-region stubs at
 * init time. For the initial RISC-V port, we use generic C function
 * handlers (read_memory/write_memory) and wrap them in small
 * JIT stubs that save/restore GBA regs around the C call.
 *
 * Each load stub:  save regs, call read_memory_X(a0), restore, ret
 * Each store stub: save regs, call write_memory_X(a0,a1), check alert, restore, ret
 *
 * TODO: Generate optimized per-region RISC-V stubs.
 */

/* Spacing between generated stubs in the translation cache. The inline
 * fast path made the load stubs grow past the old 256-byte slot. */
#define STUB_SPACING 512

/* Emit one terminal "reg_a0 = <width/sign> [reg_a0]" load, then ret.
 * ldop is the RISC-V load funct3 (RV_LBU/RV_LB/RV_LHU/RV_LH/RV_LW). */
#define emit_fast_loadop_ret(ldop)                                            \
  rv_emit(rv_enc_i(0, reg_a0, (ldop), reg_a0, RV_OP_LOAD));                   \
  rv_emit_ret()

/* Emit a load stub.
 *
 * INLINE FAST PATH (no register spilling, no C call) for the hot
 * plain-memory regions, computing the SAME address the C read_memory()
 * switch computes for the corresponding case, so behaviour is identical:
 *   region 3   (IWRAM): iwram[(addr & 0x7FFF) + 0x8000]
 *   region 2   (EWRAM): ewram[addr & 0x3FFFF]
 *   region 8-C (ROM)  : memory_map_read[addr >> 15][addr & 0x7FFF]
 *
 * Everything else — BIOS (0), I/O (4, needs &0x3FF mask + is only 0x400
 * bytes), palette (5), VRAM (6, mirrored), OAM (7), EEPROM/flash (D/E/F),
 * unmapped, or a ROM page not yet paged in (map == NULL) — falls through
 * to the original generic C-handler slow path, which is unchanged.
 *
 * ldop selects the load width/sign for the fast path and must match c_fn
 * (read_memory8 -> RV_LBU, read_memory8s -> RV_LB, read_memory16 -> RV_LHU,
 *  read_memory16s -> RV_LH, read_memory32 -> RV_LW).
 *
 * almask is the alignment mask for this width (0 for bytes, 1 for halfwords,
 * 3 for words). UNALIGNED accesses are sent to the slow path, because the C
 * handlers give unaligned loads special semantics the plain fast-path load
 * cannot reproduce: read_memory32 rotates by (addr&3)*8, read_memory16
 * rotates by 8, and read_memory16_signed returns a sign-extended *byte* for
 * an odd address. Aligned accesses (the overwhelming majority, and all
 * LDM/STM block transfers) need no rotation, so the fast path is exact.
 *
 * Scratch usage on the fast path: reg_temp (t5), reg_rv (t6). reg_a0 holds
 * the address (masked in place only after we commit to a region), reg_a1
 * holds the exact instruction PC (untouched, still valid for the slow path). */
static u8 *emit_load_stub(u8 *translation_ptr, void *c_fn, int ldop, u32 almask)
{
  u8 *entry = translation_ptr;
  u8 *b_not_iwram, *b_not_ewram, *b_not_rom, *b_rom_null, *b_rom_oob;
  u8 *b_unaligned = NULL;

  /* Reject unaligned accesses up front -> slow path (C handles rotation). */
  if (almask) {
    rv_emit_andi(reg_temp, reg_a0, almask);
    rv_emit_b_filler(bne, reg_temp, rv_zero, b_unaligned);
  }

  rv_emit_srli(reg_temp, reg_a0, 24);          /* reg_temp = addr >> 24 (region) */

  /* ---- IWRAM (region == 3): iwram[(addr & 0x7FFF) + 0x8000] ---- */
  rv_emit_addi(reg_rv, rv_zero, 3);
  rv_emit_b_filler(bne, reg_temp, reg_rv, b_not_iwram);
  rv_emit_slli(reg_a0, reg_a0, 17);            /* addr & 0x7FFF */
  rv_emit_srli(reg_a0, reg_a0, 17);
  generate_load_imm(reg_rv, (u32)&iwram[0x8000]);
  rv_emit_add(reg_a0, reg_a0, reg_rv);
  emit_fast_loadop_ret(ldop);
  rv_patch_branch(b_not_iwram, translation_ptr);

  /* ---- EWRAM (region == 2): ewram[addr & 0x3FFFF] ---- */
  rv_emit_addi(reg_rv, rv_zero, 2);
  rv_emit_b_filler(bne, reg_temp, reg_rv, b_not_ewram);
  rv_emit_slli(reg_a0, reg_a0, 14);            /* addr & 0x3FFFF */
  rv_emit_srli(reg_a0, reg_a0, 14);
  generate_load_imm(reg_rv, (u32)ewram);
  rv_emit_add(reg_a0, reg_a0, reg_rv);
  emit_fast_loadop_ret(ldop);
  rv_patch_branch(b_not_ewram, translation_ptr);

  /* ---- ROM (region 8..C) ----
   * C: if((addr & 0x1FFFFFF) >= gamepak_size) open-bus (slow path);
   *    else value = memory_map_read[addr >> 15][addr & 0x7FFF].
   * NULL map entry (block not paged in) also goes slow (load_gamepak_page). */
  rv_emit_addi(reg_rv, reg_temp, -8);          /* reg_rv = region - 8 */
  rv_emit_sltiu(reg_rv, reg_rv, 5);            /* set iff region in 8..12 (0x08-0x0C) */
  rv_emit_b_filler(beq, reg_rv, rv_zero, b_not_rom);
  /* bounds check: (addr & 0x1FFFFFF) >= gamepak_size -> slow */
  rv_emit_slli(reg_temp, reg_a0, 7);           /* addr & 0x1FFFFFF (drop top 7 bits) */
  rv_emit_srli(reg_temp, reg_temp, 7);
  generate_load_imm(reg_rv, (u32)&gamepak_size);
  rv_emit_lw(reg_rv, reg_rv, 0);               /* reg_rv = gamepak_size */
  rv_emit_b_filler(bgeu, reg_temp, reg_rv, b_rom_oob);
  /* map = memory_map_read[addr >> 15] */
  rv_emit_srli(reg_rv, reg_a0, 15);            /* page index = addr >> 15 */
  rv_emit_slli(reg_rv, reg_rv, 2);             /* * sizeof(u8*) */
  generate_load_imm(reg_temp, (u32)memory_map_read);
  rv_emit_add(reg_rv, reg_rv, reg_temp);
  rv_emit_lw(reg_rv, reg_rv, 0);               /* map = memory_map_read[idx] */
  rv_emit_b_filler(beq, reg_rv, rv_zero, b_rom_null); /* NULL -> page fault / slow */
  rv_emit_slli(reg_a0, reg_a0, 17);            /* addr & 0x7FFF */
  rv_emit_srli(reg_a0, reg_a0, 17);
  rv_emit_add(reg_a0, reg_a0, reg_rv);
  emit_fast_loadop_ret(ldop);
  rv_patch_branch(b_not_rom, translation_ptr);
  rv_patch_branch(b_rom_oob, translation_ptr);
  rv_patch_branch(b_rom_null, translation_ptr);
  if (b_unaligned)
    rv_patch_branch(b_unaligned, translation_ptr);

  /* ============================================================
   * SLOW PATH — original generic C handler (unchanged). Reached
   * with reg_a0 = untouched address, reg_a1 = exact instruction PC.
   * ============================================================ */

  /* Save ra and GBA regs */
  rv_emit_sw(rv_ra, reg_base, REG_SAVE * 4);
  emit_save_regs(true);

  /* Store the EXACT instruction PC (passed by the caller in a1) into
   * reg[REG_PC]. This is needed for:
   *   (a) open-bus / unmapped reads, which return read_memory32(reg[PC]+8)
   *       (a prefetched instruction word) — using the block-base PC here
   *       returned the wrong word and could feed a garbage BX target;
   *   (b) BIOS read protection — the exact PC is inside the current block,
   *       so it is < 0x4000 iff we are executing from the BIOS, exactly as
   *       the block-base PC was. */
  rv_emit_sw(rv_a1, reg_base, REG_PC * 4);

  /* Call C function: a0 = address (already set), returns value in a0 */
  genccall(c_fn);

  /* Restore GBA regs and ra */
  emit_restore_regs(true);
  rv_emit_lw(rv_ra, reg_base, REG_SAVE * 4);
  rv_emit_ret();

  return entry;
}

/* Emit a store stub: saves GBA regs, calls C write fn, checks alert, returns */
static u8 *emit_store_stub(u8 *translation_ptr, void *c_fn)
{
  u8 *entry = translation_ptr;

  /* Save ra and GBA regs */
  rv_emit_sw(rv_ra, reg_base, REG_SAVE * 4);
  emit_save_regs(true);

  /* Store current PC (a2 = pc+4 from JIT site) for write_io_epilogue.
   * check_and_raise_interrupts uses reg[REG_PC] for IRQ return address. */
  rv_emit_sw(reg_a2, reg_base, REG_PC * 4);

  /* Call C function: a0=address, a1=value, returns alert flags in a0 */
  genccall(c_fn);

  /* Check for alert (non-zero return = side effects) */
  rv_emit_beq(rv_a0, rv_zero, 3 * 4);  /* skip lui+jalr (2 insns) to normal return */

  /* Alert path: jump to write_io_epilogue (does not return normally) */
  {
    u32 _addr = (u32)write_io_epilogue;
    rv_emit_lui(reg_temp, (_addr + 0x800) >> 12);
    rv_emit_jalr(rv_zero, reg_temp, _addr & 0xFFF);
  }

  /* Normal return */
  emit_restore_regs(true);
  rv_emit_lw(rv_ra, reg_base, REG_SAVE * 4);
  rv_emit_ret();

  return entry;
}

/* Emit an ALIGNED store stub, used exclusively by LDM/STM/PUSH block
 * transfers (execute_aligned_store32).
 *
 * This mirrors the MIPS reference (mips_emit.h: the region store handlers
 * only run the SMC check and the I/O side-effect epilogue when `!aligned`,
 * see the `if (meminfo->check_smc && !aligned)` guard and the comment
 * "aligned (strop==3) accesses do not trigger IRQs").
 *
 * The regular emit_store_stub is WRONG for block transfers because:
 *   - it unconditionally writes reg_a2 into reg[REG_PC], but non-final block
 *     elements leave a2 = transfer BASE address (not a valid PC), and
 *   - on any alert it diverts to write_io_epilogue -> lookup_pc, which would
 *     then resume execution at that base address (data executed as code).
 * That produced a garbage BX target and the crash.
 *
 * So the aligned stub performs the write and returns WITHOUT saving a PC and
 * WITHOUT diverting: side-effects (IRQ/HALT) are picked up by the block's
 * FINAL element (which goes through the regular store stub with a2 = pc+N) or
 * by the periodic update_gba check; SMC on block writes is not tracked, just
 * like MIPS. */
static u8 *emit_aligned_store_stub(u8 *translation_ptr, void *c_fn)
{
  u8 *entry = translation_ptr;

  rv_emit_sw(rv_ra, reg_base, REG_SAVE * 4);
  emit_save_regs(true);

  /* Deliberately NOT storing a2 into reg[REG_PC] here. */

  /* a0=address, a1=value; the alert return value is intentionally ignored. */
  genccall(c_fn);

  emit_restore_regs(true);
  rv_emit_lw(rv_ra, reg_base, REG_SAVE * 4);
  rv_emit_ret();

  return entry;
}

/* Fill all 16 region entries for a tmem row with the same stub address */
static void fill_tmem_row(u32 *row, u32 addr)
{
  int i;
  for (i = 0; i < 16; i++)
    row[i] = addr;
}

void init_emitter(bool must_swap) {
  (void)must_swap;

  rom_cache_watermark = INITIAL_ROM_WATERMARK;
  u8 *translation_ptr = &rom_translation_cache[0];
  u8 *stub;

  /* Generate load stubs and fill tmemld table.
   * ldop (RV_LBU/RV_LB/RV_LHU/RV_LH/RV_LW) selects the fast-path load
   * width/sign and must match the C fallback handler. */
  /* tmemld[0] = load u8 */
  stub = emit_load_stub(translation_ptr, (void*)read_memory8, RV_LBU, 0);
  fill_tmem_row(tmemld[0], (u32)stub);
  translation_ptr = stub + STUB_SPACING;

  /* tmemld[1] = load s8 */
  stub = emit_load_stub(translation_ptr, (void*)read_memory8s, RV_LB, 0);
  fill_tmem_row(tmemld[1], (u32)stub);
  translation_ptr = stub + STUB_SPACING;

  /* tmemld[2] = load u16 */
  stub = emit_load_stub(translation_ptr, (void*)read_memory16, RV_LHU, 1);
  fill_tmem_row(tmemld[2], (u32)stub);
  fill_tmem_row(tmemld[3], (u32)stub);  /* u16 unaligned uses same handler */
  translation_ptr = stub + STUB_SPACING;

  /* tmemld[4] = load s16 */
  stub = emit_load_stub(translation_ptr, (void*)read_memory16s, RV_LH, 1);
  fill_tmem_row(tmemld[4], (u32)stub);
  fill_tmem_row(tmemld[5], (u32)stub);  /* s16 unaligned */
  translation_ptr = stub + STUB_SPACING;

  /* tmemld[6] = load u32 */
  stub = emit_load_stub(translation_ptr, (void*)read_memory32, RV_LW, 3);
  fill_tmem_row(tmemld[6], (u32)stub);
  fill_tmem_row(tmemld[7], (u32)stub);  /* u32 unaligned variants */
  fill_tmem_row(tmemld[8], (u32)stub);
  fill_tmem_row(tmemld[9], (u32)stub);
  translation_ptr = stub + STUB_SPACING;

  /* tmemld[10] = aligned load 32 (same as u32 for C fallback) */
  fill_tmem_row(tmemld[10], (u32)stub);

  /* Generate store stubs and fill tmemst table */
  /* tmemst[0] = store u8 */
  stub = emit_store_stub(translation_ptr, (void*)write_memory8);
  fill_tmem_row(tmemst[0], (u32)stub);
  translation_ptr = stub + STUB_SPACING;

  /* tmemst[1] = store u16 */
  stub = emit_store_stub(translation_ptr, (void*)write_memory16);
  fill_tmem_row(tmemst[1], (u32)stub);
  translation_ptr = stub + STUB_SPACING;

  /* tmemst[2] = store u32 */
  stub = emit_store_stub(translation_ptr, (void*)write_memory32);
  fill_tmem_row(tmemst[2], (u32)stub);
  translation_ptr = stub + STUB_SPACING;

  /* tmemst[3] = aligned store 32 — DEDICATED no-side-effect stub used by
   * LDM/STM/PUSH block transfers. Must NOT reuse the u32 stub: block callers
   * leave a2 = base address, and the u32 stub would write it to reg[REG_PC]
   * and divert to write_io_epilogue on any alert, resuming at that data
   * address. See emit_aligned_store_stub. */
  stub = emit_aligned_store_stub(translation_ptr, (void*)write_memory32);
  fill_tmem_row(tmemst[3], (u32)stub);
  translation_ptr = stub + STUB_SPACING;

  /* thnjal table: unused in RISC-V (MIPS-specific JAL encoding), zero it */
  memset(thnjal, 0, sizeof(thnjal));

  /* Flush data cache to PSRAM then sync i-cache */
  {
    uintptr_t aligned_start = (uintptr_t)&rom_translation_cache[0] & ~63UL;
    uintptr_t aligned_end   = ((uintptr_t)translation_ptr + 63) & ~63UL;
    esp_cache_msync((void *)aligned_start, aligned_end - aligned_start,
                    ESP_CACHE_MSYNC_FLAG_DIR_C2M);
  }
  __asm__ volatile("fence.i" ::: "memory");

  /* Set watermark so ROM cache flushes don't wipe our stubs */
  rom_cache_watermark = (u32)(translation_ptr - rom_translation_cache);

  init_bios_hooks();
}

/* Entry point wrapper
 * ================================================================== */

u32 execute_arm_translate_internal(u32 cycles, void *regptr);
u32 execute_arm_translate(u32 cycles) {
  static int dbg_cnt = 0;
  if (dbg_cnt < 5) {
    printf("eat_enter[%d]: PC=0x%08x CPSR=0x%08x &reg=%p\n",
           dbg_cnt, reg[REG_PC], reg[REG_CPSR], &reg[0]);
  }
  u32 ret = execute_arm_translate_internal(cycles, &reg[0]);
  if (dbg_cnt < 5) {
    printf("eat_exit[%d]: PC=0x%08x ret=%u\n",
           dbg_cnt, reg[REG_PC], ret);
    dbg_cnt++;
  }
  return ret;
}

#endif /* RISCV_EMIT_H */
