/* gameplaySP - Xtensa backend, instruction translation (included by
 * xtensa_emit.h). Same structure and semantics as x86/x86_emit.h, section by
 * section; the x86 register names map as eax = a0 (a10), edx = a1 (a11),
 * ecx = a2 (a12), esi = t0 (a4). */

#ifndef XTENSA_EMIT_OPS_H
#define XTENSA_EMIT_OPS_H

/* ---- branches out of the block -------------------------------------------- */

/* x86_update_gba: store the PC, collapse the flags, run update_gba; a frame
   completed -> leave, the PC changed -> jump to its block, else go on */
#define xt_update_call(new_pc_expr)                                           \
  generate_load_pc(a0, (new_pc_expr));                                        \
  XT(s32i, reg_cycles, reg_base, XT_CYC_SLOT * 4);                            \
  xt_call(XT_FN_UPDATE_GBA);                                                  \
  XT(l32i, reg_cycles, reg_base, XT_CYC_SLOT * 4);                            \
  XT(beqz, reg_rv, 2);                                                        \
  XT(jx, reg_rv)

/* Cold code goes to the end of the block (xt_emit_cold, called by
   translate_block after the body): in the hot path a branch is only
   "bgez a3, +2; j cold_update; j exit", and the exit j is later patched
   into a direct jump to the next block. The update_gba call and the exit's
   literal/l32r/jx stay out of the instruction cache lines the loop uses. */
/* "pc" is the branch being translated, "new_pc" where it goes. gba_over.h's
   idle_loop_target_pc is matched against the branch, as upstream does; a
   candidate in the interpreter's convention (the PC the loop branches back
   to, which is what the host harness names) goes in idle_loop_head_pc and is
   matched against the target instead. See cpu.h. */
#ifdef GBAPROF
#define XT_IDLE_LOOP_HIT(new_pc)                                              \
  (pc == idle_loop_target_pc ||                                               \
   (idle_loop_head_pc && (new_pc) == idle_loop_head_pc))
#else
#define XT_IDLE_LOOP_HIT(new_pc) (pc == idle_loop_target_pc)
#endif

#define generate_branch_no_cycle_update(writeback_location, new_pc)           \
  if(XT_IDLE_LOOP_HIT(new_pc))                                                \
  {                                                                           \
    XT(movi, reg_cycles, 0);                                                  \
    xt_update_call(new_pc);                                                   \
    xt_emit_exit_filler(writeback_location);                                  \
  }                                                                           \
  else if (xt_cold_n + 2 <= XT_COLD_MAX)                                      \
  {                                                                           \
    XT(bgez, reg_cycles, 2);                  /* over the j */                \
    xt_cold[xt_cold_n].hot = translation_ptr;                                 \
    xt_cold[xt_cold_n].target_pc = (new_pc);                                     \
    xt_cold[xt_cold_n++].kind = XT_COLD_UPDATE;                               \
    XT(j, 0);                                                                 \
    (writeback_location) = translation_ptr;                                   \
    xt_cold[xt_cold_n].hot = translation_ptr;                                 \
    xt_cold[xt_cold_n++].kind = XT_COLD_EXIT;                                 \
    XT(j, 0);                                                                 \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    u8 *_skip;                                                                \
    XT_FWD_B12(_skip, bgez, reg_cycles);                                      \
    xt_update_call(new_pc);                                                   \
    xt_fwd_b12(_skip, translation_ptr);                                       \
    xt_emit_exit_filler(writeback_location);                                  \
  }                                                                           \

#define generate_branch_cycle_update(writeback_location, new_pc)              \
  generate_cycle_update();                                                    \
  generate_branch_no_cycle_update(writeback_location, new_pc)                 \

/* a0 holds the destination */
#define xt_indirect(type)                                                     \
  xt_call(XT_FN_INDIRECT_##type);                                             \
  XT(jx, reg_rv)

#define generate_indirect_branch_cycle_update(type)                           \
  generate_cycle_update();                                                    \
  xt_indirect(type)

#define generate_indirect_branch_no_cycle_update(type)                        \
  xt_indirect(type)

#define XT_FN_INDIRECT_arm   XT_FN_INDIRECT_ARM
#define XT_FN_INDIRECT_thumb XT_FN_INDIRECT_THUMB
#define XT_FN_INDIRECT_dual  XT_FN_INDIRECT_DUAL

/* the block's start PC in a5 (generate_load_pc works relative to it) */
#define block_prologue_size 0
#define generate_block_prologue()                                             \
  stored_pc = pc;                                                             \
  generate_load_imm(pcbase, pc)
#define generate_block_extra_vars_arm()   u32 stored_pc = 0; int xt_cold_reset = (xt_cold_n = 0)
#define generate_block_extra_vars_thumb() u32 stored_pc = 0; int xt_cold_reset = (xt_cold_n = 0)
#define generate_block_cold()                                                 \
  (void)xt_cold_reset;                                                        \
  translation_ptr = xt_emit_cold(translation_ptr, stored_pc)

#define generate_indirect_branch_arm()                                        \
  {                                                                           \
    if(condition == 0x0E)                                                     \
    {                                                                         \
      generate_indirect_branch_cycle_update(arm);                             \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      generate_indirect_branch_no_cycle_update(arm);                          \
    }                                                                         \
  }                                                                           \

#define generate_indirect_branch_dual()                                       \
  {                                                                           \
    if(condition == 0x0E)                                                     \
    {                                                                         \
      generate_indirect_branch_cycle_update(dual);                            \
    }                                                                         \
    else                                                                      \
    {                                                                         \
      generate_indirect_branch_no_cycle_update(dual);                         \
    }                                                                         \
  }                                                                           \

#define emit_trace_thumb_instruction(pc)
#define emit_trace_arm_instruction(pc)

/* ---- shifts ------------------------------------------------------------------ */

/* r >>= s (logical), s 1..31 */
static __attribute__((noinline)) u8 *xt_srl_imm(u8 *translation_ptr, int r, int s)
{
  if (s <= 15)
    XT(srli, r, r, s);
  else
    XT(extui, r, r, s, 32 - s);
  return translation_ptr;
}

/* store bit b of r as the C flag */
#define xt_c_from_bit(r, b)                                                   \
  XT(extui, XT_S2, r, (b), 1);                                                \
  xt_store_flag(XT_S2, REG_C_FLAG)

/* rrx: r = (r >> 1) | (C << 31) */
#define generate_rrx(ireg)                                                    \
  generate_load_reg(s0, REG_C_FLAG);                                          \
  XT(slli, XT_S0, XT_S0, 31);                                                 \
  XT(srli, reg_##ireg, reg_##ireg, 1);                                        \
  XT(or, reg_##ireg, reg_##ireg, XT_S0)

#define generate_rrx_flags(ireg)                                              \
  generate_load_reg(s0, REG_C_FLAG);                                          \
  XT(slli, XT_S0, XT_S0, 31);                                                 \
  XT(extui, XT_S2, reg_##ireg, 0, 1);                                         \
  xt_store_flag(XT_S2, REG_C_FLAG);                                           \
  XT(srli, reg_##ireg, reg_##ireg, 1);                                        \
  XT(or, reg_##ireg, reg_##ireg, XT_S0)

#define xt_rotate_right_imm(r, s)                                             \
  XT(ssai, (s));                                                              \
  XT(src, r, r, r)

#define generate_shift_imm_lsl_no_flags(ireg)                                 \
  generate_load_reg_pc(ireg, rm, 8);                                          \
  if(shift != 0)                                                              \
  {                                                                           \
    XT(slli, reg_##ireg, reg_##ireg, shift);                                  \
  }                                                                           \

#define generate_shift_imm_lsr_no_flags(ireg)                                 \
  if(shift != 0)                                                              \
  {                                                                           \
    generate_load_reg_pc(ireg, rm, 8);                                        \
    translation_ptr = xt_srl_imm(translation_ptr, reg_##ireg, shift);         \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    XT(movi, reg_##ireg, 0);                                                  \
  }                                                                           \

#define generate_shift_imm_asr_no_flags(ireg)                                 \
  generate_load_reg_pc(ireg, rm, 8);                                          \
  if(shift != 0)                                                              \
  {                                                                           \
    XT(srai, reg_##ireg, reg_##ireg, shift);                                  \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    XT(srai, reg_##ireg, reg_##ireg, 31);                                     \
  }                                                                           \

#define generate_shift_imm_ror_no_flags(ireg)                                 \
  generate_load_reg_pc(ireg, rm, 8);                                          \
  if(shift != 0)                                                              \
  {                                                                           \
    xt_rotate_right_imm(reg_##ireg, shift);                                   \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_rrx(ireg);                                                       \
  }                                                                           \

#define generate_shift_imm_lsl_flags(ireg)                                    \
  generate_load_reg_pc(ireg, rm, 8);                                          \
  if(shift != 0)                                                              \
  {                                                                           \
    xt_c_from_bit(reg_##ireg, 32 - shift);                                    \
    XT(slli, reg_##ireg, reg_##ireg, shift);                                  \
  }                                                                           \

#define generate_shift_imm_lsr_flags(ireg)                                    \
  generate_load_reg_pc(ireg, rm, 8);                                          \
  if(shift != 0)                                                              \
  {                                                                           \
    xt_c_from_bit(reg_##ireg, shift - 1);                                     \
    translation_ptr = xt_srl_imm(translation_ptr, reg_##ireg, shift);         \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    xt_c_from_bit(reg_##ireg, 31);                                            \
    XT(movi, reg_##ireg, 0);                                                  \
  }                                                                           \

#define generate_shift_imm_asr_flags(ireg)                                    \
  generate_load_reg_pc(ireg, rm, 8);                                          \
  if(shift != 0)                                                              \
  {                                                                           \
    xt_c_from_bit(reg_##ireg, shift - 1);                                     \
    XT(srai, reg_##ireg, reg_##ireg, shift);                                  \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    XT(srai, reg_##ireg, reg_##ireg, 31);                                     \
    xt_c_from_bit(reg_##ireg, 0);                                             \
  }                                                                           \

#define generate_shift_imm_ror_flags(ireg)                                    \
  generate_load_reg_pc(ireg, rm, 8);                                          \
  if(shift != 0)                                                              \
  {                                                                           \
    xt_rotate_right_imm(reg_##ireg, shift);                                   \
    xt_c_from_bit(reg_##ireg, 31);                                            \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_rrx_flags(ireg);                                                 \
  }                                                                           \

/* Shifts by register, x86 semantics: a0 = value, a1 = amount; the machine
   shift uses the amount's low 5 bits, the >= 32 cases are then fixed up
   exactly as the x86 code does (LSL/LSR by 32 give C = 0 there). Result in
   a0, then copied to ireg. */
static __attribute__((noinline)) u8 *xt_shift_reg(u8 *translation_ptr, int kind, bool flags)
{
  u8 *zero = NULL, *small = NULL, *big = NULL, *end = NULL;
  const int v = reg_a0, s = reg_a1;
  if (!flags)
  {
    switch (kind)
    {
    case 0: XT(ssl, s); XT(sll, v, v); break;                 /* lsl */
    case 1: XT(ssr, s); XT(srl, v, v); break;                 /* lsr */
    case 2: XT(ssr, s); XT(sra, v, v); break;                 /* asr */
    case 3: XT(ssr, s); XT(src, v, v, v); return translation_ptr;   /* ror */
    }
    XT_FWD_B8(small, bltui, s, 32);
    if (kind == 2)
      XT(srai, v, v, 31);
    else
      XT(movi, v, 0);
    xt_fwd_b8(small, translation_ptr);
    return translation_ptr;
  }

  XT_FWD_B12(zero, beqz, s);                                  /* 0: unchanged */
  switch (kind)
  {
  case 0: /* lsl: C = bit 31 of v << (s - 1), v <<= s; s >= 32: v = 0, C = 0 */
    XT(addi, XT_S1, s, -1);
    XT(ssl, XT_S1);
    XT(sll, v, v);
    xt_c_from_bit(v, 31);
    XT(slli, v, v, 1);
    XT_FWD_B8(small, bltui, s, 32);
    XT(movi, v, 0);
    xt_store_flag(v, REG_C_FLAG);
    xt_fwd_b8(small, translation_ptr);
    break;
  case 1: /* lsr: C = bit 0 of v >> (s - 1), v >>= s; s >= 32: v = 0, C = 0 */
    XT(addi, XT_S1, s, -1);
    XT(ssr, XT_S1);
    XT(srl, v, v);
    xt_c_from_bit(v, 0);
    XT(srli, v, v, 1);
    XT_FWD_B8(small, bltui, s, 32);
    XT(movi, v, 0);
    xt_store_flag(v, REG_C_FLAG);
    xt_fwd_b8(small, translation_ptr);
    break;
  case 2: /* asr: s < 32: C = bit s - 1, v >>= s; else v = sign, C = sign */
    XT_FWD_B8(big, bgeui, s, 32);
    XT(addi, XT_S1, s, -1);
    XT(ssr, XT_S1);
    XT(srl, XT_S2, v);
    xt_c_from_bit(XT_S2, 0);
    XT(ssr, s);
    XT(sra, v, v);
    XT_FWD_J(end);
    xt_fwd_b8(big, translation_ptr);
    XT(srai, v, v, 31);
    xt_c_from_bit(v, 0);
    xt_fwd_j(end, translation_ptr);
    break;
  case 3: /* ror: v = ror(v, (s - 1) & 31) ; C = bit 0 ; v = ror(v, 1) */
    XT(addi, XT_S1, s, -1);
    XT(ssr, XT_S1);
    XT(src, v, v, v);
    xt_c_from_bit(v, 0);
    XT(ssai, 1);
    XT(src, v, v, v);
    break;
  }
  xt_fwd_b12(zero, translation_ptr);
  return translation_ptr;
}

#define xt_shift_kind_lsl 0
#define xt_shift_kind_lsr 1
#define xt_shift_kind_asr 2
#define xt_shift_kind_ror 3
#define xt_shift_flags_flags true
#define xt_shift_flags_no_flags false

#define generate_shift_reg(ireg, name, flags_op)                              \
  generate_load_reg_pc(a0, rm, 12);                                           \
  generate_load_reg(a1, ((opcode >> 8) & 0x0F));                              \
  XT(extui, reg_a1, reg_a1, 0, 8);                                            \
  translation_ptr = xt_shift_reg(translation_ptr, xt_shift_kind_##name,       \
                                 xt_shift_flags_##flags_op);                  \
  generate_mov(ireg, a0)

#define get_shift_imm()                                                       \
  u32 shift = (opcode >> 7) & 0x1F                                            \

#define generate_shift_imm(ireg, name, flags_op)                              \
  get_shift_imm();                                                            \
  generate_shift_imm_##name##_##flags_op(ireg)                                \

#define generate_load_rm_sh(flags_op)                                         \
  switch((opcode >> 4) & 0x07)                                                \
  {                                                                           \
    case 0x0: { generate_shift_imm(a0, lsl, flags_op); break; }               \
    case 0x1: { generate_shift_reg(a0, lsl, flags_op); break; }               \
    case 0x2: { generate_shift_imm(a0, lsr, flags_op); break; }               \
    case 0x3: { generate_shift_reg(a0, lsr, flags_op); break; }               \
    case 0x4: { generate_shift_imm(a0, asr, flags_op); break; }               \
    case 0x5: { generate_shift_reg(a0, asr, flags_op); break; }               \
    case 0x6: { generate_shift_imm(a0, ror, flags_op); break; }               \
    case 0x7: { generate_shift_reg(a0, ror, flags_op); break; }               \
  }                                                                           \

#define generate_load_offset_sh()                                             \
  switch((opcode >> 5) & 0x03)                                                \
  {                                                                           \
    case 0x0: { generate_shift_imm(a1, lsl, no_flags); break; }               \
    case 0x1: { generate_shift_imm(a1, lsr, no_flags); break; }               \
    case 0x2: { generate_shift_imm(a1, asr, no_flags); break; }               \
    case 0x3: { generate_shift_imm(a1, ror, no_flags); break; }               \
  }                                                                           \

#define generate_load_reg_pc(ireg, reg_index, pc_offset)                      \
  if(reg_index == 15)                                                         \
  {                                                                           \
    generate_load_pc(ireg, pc + pc_offset);                                   \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_load_reg(ireg, reg_index);                                       \
  }                                                                           \

#define generate_store_reg_pc_no_flags(ireg, reg_index)                       \
  generate_store_reg(ireg, reg_index);                                        \
  if(reg_index == 15)                                                         \
  {                                                                           \
    generate_mov(a0, ireg);                                                   \
    generate_indirect_branch_arm();                                           \
  }                                                                           \

#define generate_store_reg_pc_flags(ireg, reg_index)                          \
  generate_store_reg(ireg, reg_index);                                        \
  if(reg_index == 15)                                                         \
  {                                                                           \
    generate_mov(a0, ireg);                                                   \
    xt_call(XT_FN_SPSR_RESTORE);                                              \
    generate_indirect_branch_dual();                                          \
  }                                                                           \

/* ---- conditions ---------------------------------------------------------------- */
/* Skip the instruction when the condition fails: a branch taken when it
   holds jumps over a patched j (backpatch_address) that skips it. */
#define xt_cond_skip(bop, hreg)                                               \
  XT(bop, hreg, 2);                                                           \
  backpatch_address = translation_ptr;                                        \
  XT(j, 0)

#define xt_flag(regnum) XT(l32i, XT_S0, reg_base, (regnum) * 4)

#define generate_condition_eq(ireg) xt_flag(REG_Z_FLAG); xt_cond_skip(bnez, XT_S0)
#define generate_condition_ne(ireg) xt_flag(REG_Z_FLAG); xt_cond_skip(beqz, XT_S0)
#define generate_condition_cs(ireg) xt_flag(REG_C_FLAG); xt_cond_skip(bnez, XT_S0)
#define generate_condition_cc(ireg) xt_flag(REG_C_FLAG); xt_cond_skip(beqz, XT_S0)
#define generate_condition_mi(ireg) xt_flag(REG_N_FLAG); xt_cond_skip(bnez, XT_S0)
#define generate_condition_pl(ireg) xt_flag(REG_N_FLAG); xt_cond_skip(beqz, XT_S0)
#define generate_condition_vs(ireg) xt_flag(REG_V_FLAG); xt_cond_skip(bnez, XT_S0)
#define generate_condition_vc(ireg) xt_flag(REG_V_FLAG); xt_cond_skip(beqz, XT_S0)

/* hi: C && !Z  <=>  ((C ^ 1) | Z) == 0 */
#define xt_c_inv_or_z()                                                       \
  XT(l32i, XT_S0, reg_base, REG_C_FLAG * 4);                                  \
  XT(movi, XT_S1, 1);                                                         \
  XT(xor, XT_S0, XT_S0, XT_S1);                                               \
  XT(l32i, XT_S1, reg_base, REG_Z_FLAG * 4);                                  \
  XT(or, XT_S0, XT_S0, XT_S1)
/* (N ^ V) | Z */
#define xt_n_xor_v(or_z)                                                      \
  XT(l32i, XT_S0, reg_base, REG_N_FLAG * 4);                                  \
  XT(l32i, XT_S1, reg_base, REG_V_FLAG * 4);                                  \
  XT(xor, XT_S0, XT_S0, XT_S1);                                               \
  if (or_z)                                                                   \
  {                                                                           \
    XT(l32i, XT_S1, reg_base, REG_Z_FLAG * 4);                                \
    XT(or, XT_S0, XT_S0, XT_S1);                                              \
  }

#define generate_condition_hi(ireg) xt_c_inv_or_z(); xt_cond_skip(beqz, XT_S0)
#define generate_condition_ls(ireg) xt_c_inv_or_z(); xt_cond_skip(bnez, XT_S0)
#define generate_condition_ge(ireg) xt_n_xor_v(0); xt_cond_skip(beqz, XT_S0)
#define generate_condition_lt(ireg) xt_n_xor_v(0); xt_cond_skip(bnez, XT_S0)
#define generate_condition_gt(ireg) xt_n_xor_v(1); xt_cond_skip(beqz, XT_S0)
#define generate_condition_le(ireg) xt_n_xor_v(1); xt_cond_skip(bnez, XT_S0)

#define generate_condition(ireg)                                              \
  switch(condition)                                                           \
  {                                                                           \
    case 0x0: generate_condition_eq(ireg); break;                             \
    case 0x1: generate_condition_ne(ireg); break;                             \
    case 0x2: generate_condition_cs(ireg); break;                             \
    case 0x3: generate_condition_cc(ireg); break;                             \
    case 0x4: generate_condition_mi(ireg); break;                             \
    case 0x5: generate_condition_pl(ireg); break;                             \
    case 0x6: generate_condition_vs(ireg); break;                             \
    case 0x7: generate_condition_vc(ireg); break;                             \
    case 0x8: generate_condition_hi(ireg); break;                             \
    case 0x9: generate_condition_ls(ireg); break;                             \
    case 0xA: generate_condition_ge(ireg); break;                             \
    case 0xB: generate_condition_lt(ireg); break;                             \
    case 0xC: generate_condition_gt(ireg); break;                             \
    case 0xD: generate_condition_le(ireg); break;                             \
    case 0xE: break;                                                          \
    case 0xF: break;                                                          \
  }                                                                           \

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
}                                                                             \

/* ---- data processing ------------------------------------------------------------ */

#define rm_op_reg rm
#define rm_op_imm imm

#define arm_data_proc_reg_flags()                                             \
  arm_decode_data_proc_reg(opcode);                                           \
  if(flag_status & 0x02)                                                      \
  {                                                                           \
    generate_load_rm_sh(flags)                                                \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    generate_load_rm_sh(no_flags);                                            \
  }                                                                           \

#define arm_data_proc_reg()                                                   \
  arm_decode_data_proc_reg(opcode);                                           \
  generate_load_rm_sh(no_flags)                                               \

#define arm_data_proc_imm()                                                   \
  arm_decode_data_proc_imm(opcode);                                           \
  ror(imm, imm, imm_ror);                                                     \
  generate_load_imm(a0, imm)                                                  \

#define arm_data_proc_imm_flags()                                             \
  arm_decode_data_proc_imm(opcode);                                           \
  if((flag_status & 0x02) && (imm_ror != 0))                                  \
  {                                                                           \
    /* Generate carry flag from integer rotation */                           \
    generate_load_imm(a0, ((imm >> (imm_ror - 1)) & 0x01));                   \
    generate_store_reg(a0, REG_C_FLAG);                                       \
  }                                                                           \
  ror(imm, imm, imm_ror);                                                     \
  generate_load_imm(a0, imm)                                                  \

#define arm_data_proc(name, type, flags_op)                                   \
{                                                                             \
  arm_data_proc_##type();                                                     \
  generate_load_reg_pc(a1, rn, 8);                                            \
  arm_data_proc_##name(rd, generate_store_reg_pc_##flags_op);                 \
}                                                                             \

#define arm_data_proc_test(name, type)                                        \
{                                                                             \
  arm_data_proc_##type();                                                     \
  generate_load_reg_pc(a1, rn, 8);                                            \
  arm_data_proc_test_##name();                                                \
}                                                                             \

#define arm_data_proc_unary(name, type, flags_op)                             \
{                                                                             \
  arm_data_proc_##type();                                                     \
  arm_data_proc_unary_##name(rd, generate_store_reg_pc_##flags_op);           \
}                                                                             \

#define arm_data_proc_mov(type)                                               \
{                                                                             \
  arm_data_proc_##type();                                                     \
  generate_store_reg_pc_no_flags(a0, rd);                                     \
}                                                                             \

#define update_logical_flags() update_logical_flags_of(a0)

/* x86: a0 op= a1 (and, eor, orr, add, adc, mul, bic), a1 = a1 - a0 (sub,
   sbc), a0 = a0 - a1 (rsb, rsc) */
#define xt_not(r) XT(movi, XT_S3, -1); XT(xor, r, r, XT_S3)

#define arm_data_proc_and(rd, storefnc)                                       \
  XT(and, reg_a0, reg_a0, reg_a1); storefnc(a0, rd);
#define arm_data_proc_ands(rd, storefnc)                                      \
  XT(and, reg_a0, reg_a0, reg_a1); update_logical_flags(); storefnc(a0, rd);
#define arm_data_proc_eor(rd, storefnc)                                       \
  XT(xor, reg_a0, reg_a0, reg_a1); storefnc(a0, rd);
#define arm_data_proc_eors(rd, storefnc)                                      \
  XT(xor, reg_a0, reg_a0, reg_a1); update_logical_flags(); storefnc(a0, rd);
#define arm_data_proc_orr(rd, storefnc)                                       \
  XT(or, reg_a0, reg_a0, reg_a1); storefnc(a0, rd);
#define arm_data_proc_orrs(rd, storefnc)                                      \
  XT(or, reg_a0, reg_a0, reg_a1); update_logical_flags(); storefnc(a0, rd);
#define arm_data_proc_bic(rd, storefnc)                                       \
  xt_not(reg_a0); XT(and, reg_a0, reg_a0, reg_a1); storefnc(a0, rd);
#define arm_data_proc_bics(rd, storefnc)                                      \
  xt_not(reg_a0); XT(and, reg_a0, reg_a0, reg_a1); update_logical_flags();    \
  storefnc(a0, rd);
#define arm_data_proc_add(rd, storefnc)                                       \
  XT(add, reg_a0, reg_a0, reg_a1); storefnc(a0, rd);
#define arm_data_proc_adds(rd, storefnc)                                      \
  translation_ptr = xt_add_op(translation_ptr, reg_a0, reg_a0, reg_a1, -1, flag_status); \
  storefnc(a0, rd);
#define arm_data_proc_sub(rd, storefnc)                                       \
  XT(sub, reg_a1, reg_a1, reg_a0); storefnc(a1, rd);
#define arm_data_proc_rsb(rd, storefnc)                                       \
  XT(sub, reg_a0, reg_a0, reg_a1); storefnc(a0, rd);
#define arm_data_proc_subs(rd, storefnc)                                      \
  translation_ptr = xt_sub_op(translation_ptr, reg_a1, reg_a1, reg_a0, -1, flag_status); \
  storefnc(a1, rd);
#define arm_data_proc_rsbs(rd, storefnc)                                      \
  translation_ptr = xt_sub_op(translation_ptr, reg_a0, reg_a0, reg_a1, -1, flag_status); \
  storefnc(a0, rd);
#define arm_data_proc_mul(rd, storefnc)                                       \
  XT(mull, reg_a0, reg_a0, reg_a1); storefnc(a0, rd);
#define arm_data_proc_muls(rd, storefnc)                                      \
  XT(mull, reg_a0, reg_a0, reg_a1); update_logical_flags(); storefnc(a0, rd);

/* carry in (0/1) in a2; the borrow of sbc/rsc is 1 - C */
#define xt_load_c(r)      generate_load_reg(r, REG_C_FLAG)
#define xt_load_inv_c(r)                                                      \
  generate_load_reg(r, REG_C_FLAG);                                           \
  XT(movi, XT_S3, 1);                                                         \
  XT(xor, reg_##r, reg_##r, XT_S3)

#define arm_data_proc_adc(rd, storefnc)                                       \
  xt_load_c(a2); XT(add, reg_a0, reg_a0, reg_a1); XT(add, reg_a0, reg_a0, reg_a2); \
  storefnc(a0, rd);
#define arm_data_proc_adcs(rd, storefnc)                                      \
  xt_load_c(a2);                                                              \
  translation_ptr = xt_add_op(translation_ptr, reg_a0, reg_a0, reg_a1, reg_a2, flag_status); \
  storefnc(a0, rd);
#define arm_data_proc_sbc(rd, storefnc)                                       \
  xt_load_inv_c(a2); XT(sub, reg_a1, reg_a1, reg_a0); XT(sub, reg_a1, reg_a1, reg_a2); \
  storefnc(a1, rd);
#define arm_data_proc_sbcs(rd, storefnc)                                      \
  xt_load_inv_c(a2);                                                          \
  translation_ptr = xt_sub_op(translation_ptr, reg_a1, reg_a1, reg_a0, reg_a2, flag_status); \
  storefnc(a1, rd);
#define arm_data_proc_rsc(rd, storefnc)                                       \
  xt_load_inv_c(a2); XT(sub, reg_a0, reg_a0, reg_a1); XT(sub, reg_a0, reg_a0, reg_a2); \
  storefnc(a0, rd);
#define arm_data_proc_rscs(rd, storefnc)                                      \
  xt_load_inv_c(a2);                                                          \
  translation_ptr = xt_sub_op(translation_ptr, reg_a0, reg_a0, reg_a1, reg_a2, flag_status); \
  storefnc(a0, rd);

#define arm_data_proc_test_cmp()                                              \
  translation_ptr = xt_sub_op(translation_ptr, reg_a1, reg_a1, reg_a0, -1, flag_status)
#define arm_data_proc_test_cmn()                                              \
  translation_ptr = xt_add_op(translation_ptr, reg_a1, reg_a1, reg_a0, -1, flag_status)
#define arm_data_proc_test_tst()                                              \
  XT(and, reg_a0, reg_a0, reg_a1); update_logical_flags()
#define arm_data_proc_test_teq()                                              \
  XT(xor, reg_a0, reg_a0, reg_a1); update_logical_flags()

#define arm_data_proc_unary_mov(rd, storefnc)                                 \
  storefnc(a0, rd);
#define arm_data_proc_unary_movs(rd, storefnc)                                \
  arm_data_proc_unary_mov(rd, storefnc);                                      \
  update_logical_flags()
#define arm_data_proc_unary_mvn(rd, storefnc)                                 \
  xt_not(reg_a0); storefnc(a0, rd);
#define arm_data_proc_unary_mvns(rd, storefnc)                                \
  arm_data_proc_unary_mvn(rd, storefnc);                                      \
  update_logical_flags()
#define arm_data_proc_unary_neg(rd, storefnc)                                 \
  XT(movi, reg_a1, 0);                                                        \
  arm_data_proc_subs(rd, storefnc)

/* ---- multiply ------------------------------------------------------------------ */
#define arm_multiply_flags_yes()                                              \
  {                                                                           \
    u32 flag_status = 0x0C;                                                   \
    update_logical_flags_of(a0);                                              \
  }
#define arm_multiply_flags_no(_dest)
#define arm_multiply_add_no()
#define arm_multiply_add_yes()                                                \
  generate_load_reg(a1, rn);                                                  \
  XT(add, reg_a0, reg_a0, reg_a1)

#define arm_multiply(add_op, flags)                                           \
{                                                                             \
  arm_decode_multiply();                                                      \
  generate_load_reg(a0, rm);                                                  \
  generate_load_reg(a1, rs);                                                  \
  XT(mull, reg_a0, reg_a0, reg_a1);                                           \
  arm_multiply_add_##add_op();                                                \
  arm_multiply_flags_##flags();                                               \
  generate_store_reg(a0, rd);                                                 \
}                                                                             \

/* a0:a1 = rm * rs (64 bits), + rdhi:rdlo */
#define xt_mul64_s64() XT(mulsh, XT_S1, reg_a0, reg_a1); XT(mull, reg_a0, reg_a0, reg_a1); XT(mov, reg_a1, XT_S1)
/* cpu_threaded.c names the accumulating forms u64_add / s64_add */
#define xt_mul64_u64_add xt_mul64_u64
#define xt_mul64_s64_add xt_mul64_s64
#define xt_mul64_u64() XT(muluh, XT_S1, reg_a0, reg_a1); XT(mull, reg_a0, reg_a0, reg_a1); XT(mov, reg_a1, XT_S1)

#define arm_multiply_long_add_no(name)                                        \
  xt_mul64_##name()
#define arm_multiply_long_add_yes(name)                                       \
  xt_mul64_##name();                                                          \
  generate_load_reg(a2, rdlo);                                                \
  generate_load_reg(t0, rdhi);                                                \
  XT(add, reg_a0, reg_a0, reg_a2);                                            \
  XT(saltu, XT_S2, reg_a0, reg_a2);                                           \
  XT(add, reg_a1, reg_a1, reg_t0);                                            \
  XT(add, reg_a1, reg_a1, XT_S2)

#define arm_multiply_long_flags_no(_dest)
#define arm_multiply_long_flags_yes()                                         \
  XT(extui, XT_S0, reg_a1, 31, 1);                                            \
  xt_store_flag(XT_S0, REG_N_FLAG);                                           \
  XT(or, XT_S1, reg_a0, reg_a1);                                              \
  XT(nsau, XT_S0, XT_S1);                                                     \
  XT(extui, XT_S0, XT_S0, 5, 1);                                              \
  xt_store_flag(XT_S0, REG_Z_FLAG)

#define arm_multiply_long(name, add_op, flags)                                \
{                                                                             \
  arm_decode_multiply_long();                                                 \
  generate_load_reg(a0, rm);                                                  \
  generate_load_reg(a1, rs);                                                  \
  arm_multiply_long_add_##add_op(name);                                       \
  generate_store_reg(a0, rdlo);                                               \
  generate_store_reg(a1, rdhi);                                               \
  arm_multiply_long_flags_##flags();                                          \
}                                                                             \

/* ---- PSR ------------------------------------------------------------------------ */
#define xt_psr_read_cpsr() xt_call(XT_FN_READ_CPSR)
#define xt_psr_read_spsr() xt_call(XT_FN_READ_SPSR)
#define arm_psr_read(op_type, psr_reg)                                        \
  xt_psr_read_##psr_reg();                                                    \
  generate_store_reg(rv, rd)

#define arm_psr_load_new_reg()                                                \
  generate_load_reg(a0, rm)

#define arm_psr_load_new_imm()                                                \
  ror(imm, imm, imm_ror);                                                     \
  generate_load_imm(a0, imm)

#define xt_execute_store_cpsr_()                                               \
  generate_load_imm(a1, cpsr_masks[psr_pfield][0]);                           \
  generate_load_imm(a2, cpsr_masks[psr_pfield][1]);                           \
  generate_store_reg_i32(pc, REG_PC);                                         \
  XT(s32i, reg_cycles, reg_base, XT_CYC_SLOT * 4);                            \
  xt_call(XT_FN_STORE_CPSR);                                                  \
  xt_jump_if_redirect()

#define xt_execute_store_spsr_()                                               \
  generate_load_imm(a1, spsr_masks[psr_pfield]);                              \
  xt_call(XT_FN_STORE_SPSR)

#define arm_psr_store(op_type, psr_reg)                                       \
  arm_psr_load_new_##op_type();                                               \
  xt_execute_store_##psr_reg##_()

#define arm_psr(op_type, transfer_type, psr_reg)                              \
{                                                                             \
  arm_decode_psr_##op_type(opcode);                                           \
  arm_psr_##transfer_type(op_type, psr_reg);                                  \
}                                                                             \

/* ---- memory ------------------------------------------------------------------------ */
#define XT_FN_LOAD_u8   XT_FN_LOAD_U8
#define XT_FN_LOAD_s8   XT_FN_LOAD_S8
#define XT_FN_LOAD_u16  XT_FN_LOAD_U16
#define XT_FN_LOAD_s16  XT_FN_LOAD_S16
#define XT_FN_LOAD_u32  XT_FN_LOAD_U32
#define XT_FN_STORE_u8  XT_FN_STORE_U8
#define XT_FN_STORE_u16 XT_FN_STORE_U16
#define XT_FN_STORE_u32 XT_FN_STORE_U32

/* loads: a0 = address, a1 = PC; the value in a0. With xt_fastmem the table
   entry is a call0 routine of xt_fastmem.S (same 6 bytes: l32i + callx0) */
#define xt_load(mem_type)                                                     \
  do {                                                                        \
    XT(l32i, XT_CALLREG, reg_base, (XT_FN_SLOT + XT_FN_LOAD_##mem_type) * 4); \
    if (xt_fastmem)                                                           \
      XT(callx0, XT_CALLREG);                                                 \
    else                                                                      \
      XT(callx8, XT_CALLREG);                                                 \
  } while (0)

/* stores: a0 = address, a1 = value, a2 = the PC to store in reg[REG_PC]
   (0: leave it), a13 = cycles left; the handler writes both to reg[] (the
   x86 code stored them before the call). May redirect: the jump goes
   through the block's cold area. */
#define xt_store_pc(fn, pc_value)                                             \
  if (pc_value)                                                               \
    generate_load_pc(a2, (pc_value));                                         \
  else                                                                        \
    XT(movi, reg_a2, 0);                                                      \
  /* xt_cycfix: the cycles handed over include the block's pending ones     \
     (cycle_count, charged at the next update), so a redirect at this store  \
     does not drop them: the time a redirect leaves no longer depends on     \
     whether it happened (same 3 bytes when they fit addi) */                \
  if (xt_cycfix && cycle_count && cycle_count <= 128)                         \
    XT(addi, XT_S1, reg_cycles, -(s32)cycle_count);                           \
  else                                                                        \
  {                                                                           \
    XT(mov, XT_S1, reg_cycles);                                               \
    if (xt_cycfix)                                                            \
      translation_ptr = xt_add_imm(translation_ptr, XT_S1, (u32)-(s32)cycle_count); \
  }                                                                           \
  XT(l32i, XT_CALLREG, reg_base, (XT_FN_SLOT + (fn)) * 4);                    \
  if (xt_faststore)   /* a call0 routine of xt_fastmem.S: it takes a       \
                         redirect itself (xt_fast_st_redirect) */             \
    XT(callx0, XT_CALLREG);                                                   \
  else                                                                        \
  {                                                                           \
    XT(callx8, XT_CALLREG);                                                   \
    xt_redirect_cold();                                                       \
  }
#define xt_store(fn) xt_store_pc(fn, 0)

#define arm_access_memory_load(mem_type)                                      \
  cycle_count += 2;                                                           \
  generate_load_pc(a1, pc);                                                   \
  xt_load(mem_type);                                                          \
  generate_store_reg_pc_no_flags(rv, rd)                                      \

#define arm_access_memory_store(mem_type)                                     \
  cycle_count++;                                                              \
  generate_load_reg_pc(a1, rd, 12);                                           \
  xt_store_pc(XT_FN_STORE_##mem_type, pc + 4)                                 \

#define no_op                                                                 \

#define arm_access_memory_adjust_op_up      add
#define arm_access_memory_adjust_op_down    sub
#define arm_access_memory_reverse_op_up     sub
#define arm_access_memory_reverse_op_down   add

#define generate_add(ireg_dest, ireg_src)                                     \
  XT(add, reg_##ireg_dest, reg_##ireg_dest, reg_##ireg_src)
#define generate_sub(ireg_dest, ireg_src)                                     \
  XT(sub, reg_##ireg_dest, reg_##ireg_dest, reg_##ireg_src)

#define arm_access_memory_reg_pre(adjust_dir_op, reverse_dir_op)              \
  generate_load_reg_pc(a0, rn, 8);                                            \
  generate_##adjust_dir_op(a0, a1)                                            \

#define arm_access_memory_reg_pre_wb(adjust_dir_op, reverse_dir_op)           \
  arm_access_memory_reg_pre(adjust_dir_op, reverse_dir_op);                   \
  generate_store_reg(a0, rn)                                                  \

#define arm_access_memory_reg_post(adjust_dir_op, reverse_dir_op)             \
  generate_load_reg(a0, rn);                                                  \
  generate_##adjust_dir_op(a0, a1);                                           \
  generate_store_reg(a0, rn);                                                 \
  generate_##reverse_dir_op(a0, a1)                                           \

#define arm_access_memory_imm_pre(adjust_dir_op, reverse_dir_op)              \
  generate_load_reg_pc(a0, rn, 8);                                            \
  generate_##adjust_dir_op##_imm(a0, offset)                                  \

#define arm_access_memory_imm_pre_wb(adjust_dir_op, reverse_dir_op)           \
  arm_access_memory_imm_pre(adjust_dir_op, reverse_dir_op);                   \
  generate_store_reg(a0, rn)                                                  \

#define arm_access_memory_imm_post(adjust_dir_op, reverse_dir_op)             \
  generate_load_reg(a0, rn);                                                  \
  generate_##adjust_dir_op##_imm(a0, offset);                                 \
  generate_store_reg(a0, rn);                                                 \
  generate_##reverse_dir_op##_imm(a0, offset)                                 \

#define arm_data_trans_reg(adjust_op, adjust_dir_op, reverse_dir_op)          \
  arm_decode_data_trans_reg();                                                \
  generate_load_offset_sh();                                                  \
  arm_access_memory_reg_##adjust_op(adjust_dir_op, reverse_dir_op)            \

#define arm_data_trans_imm(adjust_op, adjust_dir_op, reverse_dir_op)          \
  arm_decode_data_trans_imm();                                                \
  arm_access_memory_imm_##adjust_op(adjust_dir_op, reverse_dir_op)            \

#define arm_data_trans_half_reg(adjust_op, adjust_dir_op, reverse_dir_op)     \
  arm_decode_half_trans_r();                                                  \
  generate_load_reg(a1, rm);                                                  \
  arm_access_memory_reg_##adjust_op(adjust_dir_op, reverse_dir_op)            \

#define arm_data_trans_half_imm(adjust_op, adjust_dir_op, reverse_dir_op)     \
  arm_decode_half_trans_of();                                                 \
  arm_access_memory_imm_##adjust_op(adjust_dir_op, reverse_dir_op)            \

#define arm_access_memory(access_type, direction, adjust_op, mem_type,        \
 offset_type)                                                                 \
{                                                                             \
  arm_data_trans_##offset_type(adjust_op,                                     \
   arm_access_memory_adjust_op_##direction,                                   \
   arm_access_memory_reverse_op_##direction);                                 \
                                                                              \
  arm_access_memory_##access_type(mem_type);                                  \
}                                                                             \

#define word_bit_count(word)                                                  \
  (bit_count[word >> 8] + bit_count[word & 0xFF])                             \

#define arm_block_memory_load()                                               \
  generate_load_pc(a1, pc);                                                   \
  xt_load(u32);                                                               \
  generate_store_reg(rv, i)                                                   \

#define arm_block_memory_store()                                              \
  generate_load_reg_pc(a1, i, 8);                                             \
  xt_store(XT_FN_STORE_ALIGNED_U32)                                           \

#define arm_block_memory_final_load(writeback_type)                           \
  arm_block_memory_load()                                                     \

#define arm_block_memory_final_store(writeback_type)                          \
  generate_load_reg_pc(a1, i, 12);                                            \
  arm_block_memory_writeback_post_store(writeback_type);                      \
  xt_store_pc(XT_FN_STORE_U32, pc + 4)                                        \

#define arm_block_memory_adjust_pc_store()                                    \

#define arm_block_memory_adjust_pc_load()                                     \
  if(reg_list & 0x8000)                                                       \
  {                                                                           \
    generate_indirect_branch_arm();                                           \
  }                                                                           \

#define arm_block_memory_offset_down_a()                                      \
  generate_add_imm(a0, -((word_bit_count(reg_list) * 4) - 4))                 \

#define arm_block_memory_offset_down_b()                                      \
  generate_add_imm(a0, -(word_bit_count(reg_list) * 4))                       \

#define arm_block_memory_offset_no()                                          \

#define arm_block_memory_offset_up()                                          \
  generate_add_imm(a0, 4)                                                     \

#define arm_block_memory_writeback_down()                                     \
  generate_load_reg(a2, rn);                                                  \
  generate_add_imm(a2, -(word_bit_count(reg_list) * 4));                      \
  generate_store_reg(a2, rn)                                                  \

#define arm_block_memory_writeback_up()                                       \
  generate_load_reg(a2, rn);                                                  \
  generate_add_imm(a2, (word_bit_count(reg_list) * 4));                       \
  generate_store_reg(a2, rn)                                                  \

#define arm_block_memory_writeback_no()

#define arm_block_memory_writeback_pre_load(writeback_type)                   \
  if(!((reg_list >> rn) & 0x01))                                              \
  {                                                                           \
    arm_block_memory_writeback_##writeback_type();                            \
  }                                                                           \

#define arm_block_memory_writeback_pre_store(writeback_type)                  \

#define arm_block_memory_writeback_post_store(writeback_type)                 \
  arm_block_memory_writeback_##writeback_type()                               \

#define arm_block_memory(access_type, offset_type, writeback_type, s_bit)     \
{                                                                             \
  arm_decode_block_trans();                                                   \
  u32 offset = 0;                                                             \
  u32 i;                                                                      \
                                                                              \
  generate_load_reg(a0, rn);                                                  \
  arm_block_memory_offset_##offset_type();                                    \
  generate_and_imm(a0, ~0x03);                                                \
  generate_store_reg(a0, REG_SAVE3);                                          \
  arm_block_memory_writeback_pre_##access_type(writeback_type);               \
                                                                              \
  for(i = 0; i < 16; i++)                                                     \
  {                                                                           \
    if((reg_list >> i) & 0x01)                                                \
    {                                                                         \
      cycle_count++;                                                          \
      generate_load_reg(a0, REG_SAVE3);                                       \
      generate_add_imm(a0, offset);                                           \
      if(reg_list & ~((2 << i) - 1))                                          \
      {                                                                       \
        arm_block_memory_##access_type();                                     \
        offset += 4;                                                          \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        arm_block_memory_final_##access_type(writeback_type);                 \
      }                                                                       \
    }                                                                         \
  }                                                                           \
                                                                              \
  arm_block_memory_adjust_pc_##access_type();                                 \
}                                                                             \

#define arm_swap(type)                                                        \
{                                                                             \
  arm_decode_swap();                                                          \
  cycle_count += 3;                                                           \
  generate_load_reg(a0, rn);                                                  \
  generate_load_pc(a1, pc);                                                   \
  xt_load(type);                                                              \
  generate_mov(t0, rv);                                                       \
  generate_load_reg(a0, rn);                                                  \
  generate_load_reg(a1, rm);                                                  \
  generate_store_reg(t0, rd);                                                 \
  xt_store(XT_FN_STORE_##type);                                               \
}                                                                             \

/* ---- Thumb -------------------------------------------------------------------------- */
#define thumb_rn_op_reg(_rn)                                                  \
  generate_load_reg(a0, _rn)                                                  \

#define thumb_rn_op_imm(_imm)                                                 \
  generate_load_imm(a0, _imm)                                                 \

/* The common Thumb ALU ops go through xt_thumb_alu, which works on the host
   registers of r0..r2 directly (no mov in and out); the rest keep the x86
   shape (operands in a0/a1). Operand order and flag code are the same as
   arm_data_proc_*: a0 = rn (or the immediate), a1 = rs. */
enum { XOP_NONE = -1, XOP_ADD, XOP_SUB, XOP_AND, XOP_EOR, XOP_ORR, XOP_CMP, XOP_CMN, XOP_TST, XOP_MOV };
#define XOP_adds XOP_ADD
#define XOP_subs XOP_SUB
#define XOP_ands XOP_AND
#define XOP_eors XOP_EOR
#define XOP_orrs XOP_ORR
#define XOP_cmp  XOP_CMP
#define XOP_cmn  XOP_CMN
#define XOP_tst  XOP_TST
#define XOP_movs XOP_MOV
#define XOP_adcs XOP_NONE
#define XOP_sbcs XOP_NONE
#define XOP_muls XOP_NONE
#define XOP_bics XOP_NONE
#define XOP_mvns XOP_NONE
#define XOP_neg  XOP_NONE
#define XOP_teq  XOP_NONE
#define XT_RN_reg 0
#define XT_RN_imm 1

static __attribute__((noinline)) u8 *xt_thumb_alu(u8 *translation_ptr, int op, int rd, u32 rs,
                                                  int rn_imm, u32 rn, u32 flag_status)
{
  int a, b = -1, res, m;
  /* a = rn: its host register, loaded into a0, or the immediate in a0 */
  if (rn_imm)
  {
    if (op == XOP_MOV)
    {
      m = xt_host_of(rd);
      res = m >= 0 ? m : reg_a0;
      translation_ptr = xt_load_imm32(translation_ptr, res, rn);
      translation_ptr = xt_nz_flags(translation_ptr, res, flag_status);
      if (m < 0)
        XT(s32i, res, reg_base, rd * 4);
      return translation_ptr;
    }
    translation_ptr = xt_load_imm32(translation_ptr, reg_a0, rn);
    a = reg_a0;
  }
  else if ((m = xt_host_of(rn)) >= 0)
    a = m;
  else
  {
    XT(l32i, reg_a0, reg_base, rn * 4);
    a = reg_a0;
  }
  if ((m = xt_host_of(rs)) >= 0)
    b = m;
  else
  {
    XT(l32i, reg_a1, reg_base, rs * 4);
    b = reg_a1;
  }
  /* the result: straight into rd's host register, else a0 then reg[rd] */
  m = rd >= 0 ? xt_host_of(rd) : -1;
  res = m >= 0 ? m : reg_a0;
  switch (op)
  {
  case XOP_ADD: translation_ptr = xt_add_op(translation_ptr, res, a, b, -1, flag_status); break;
  case XOP_SUB: translation_ptr = xt_sub_op(translation_ptr, res, b, a, -1, flag_status); break;
  case XOP_AND: XT(and, res, a, b); translation_ptr = xt_nz_flags(translation_ptr, res, flag_status); break;
  case XOP_EOR: XT(xor, res, a, b); translation_ptr = xt_nz_flags(translation_ptr, res, flag_status); break;
  case XOP_ORR: XT(or, res, a, b); translation_ptr = xt_nz_flags(translation_ptr, res, flag_status); break;
  case XOP_CMP: return xt_sub_op(translation_ptr, XT_S3, b, a, -1, flag_status);
  case XOP_CMN: return xt_add_op(translation_ptr, XT_S3, b, a, -1, flag_status);
  case XOP_TST: XT(and, XT_S3, a, b); return xt_nz_flags(translation_ptr, XT_S3, flag_status);
  }
  if (m < 0)
    XT(s32i, res, reg_base, rd * 4);
  return translation_ptr;
}

#define thumb_data_proc(type, name, rn_type, _rd, _rs, _rn)                   \
{                                                                             \
  thumb_decode_##type();                                                      \
  if (XOP_##name != XOP_NONE)                                                 \
    translation_ptr = xt_thumb_alu(translation_ptr, XOP_##name, (_rd), (_rs), \
                                   XT_RN_##rn_type, (_rn), flag_status);      \
  else                                                                        \
  {                                                                           \
    thumb_rn_op_##rn_type(_rn);                                               \
    generate_load_reg(a1, _rs);                                               \
    arm_data_proc_##name(_rd, generate_store_reg);                            \
  }                                                                           \
}                                                                             \

#define thumb_data_proc_test(type, name, rn_type, _rs, _rn)                   \
{                                                                             \
  thumb_decode_##type();                                                      \
  if (XOP_##name != XOP_NONE)                                                 \
    translation_ptr = xt_thumb_alu(translation_ptr, XOP_##name, -1, (_rs),    \
                                   XT_RN_##rn_type, (_rn), flag_status);      \
  else                                                                        \
  {                                                                           \
    thumb_rn_op_##rn_type(_rn);                                               \
    generate_load_reg(a1, _rs);                                               \
    arm_data_proc_test_##name();                                              \
  }                                                                           \
}                                                                             \

#define thumb_data_proc_unary(type, name, rn_type, _rd, _rn)                  \
{                                                                             \
  thumb_decode_##type();                                                      \
  if (XOP_##name == XOP_MOV && XT_RN_##rn_type == XT_RN_imm)                  \
    translation_ptr = xt_thumb_alu(translation_ptr, XOP_MOV, (_rd), 0,        \
                                   XT_RN_imm, (_rn), flag_status);            \
  else                                                                        \
  {                                                                           \
    thumb_rn_op_##rn_type(_rn);                                               \
    arm_data_proc_unary_##name(_rd, generate_store_reg);                      \
  }                                                                           \
}                                                                             \

#define thumb_data_proc_mov(type, rn_type, _rd, _rn)                          \
{                                                                             \
  thumb_decode_##type();                                                      \
  thumb_rn_op_##rn_type(_rn);                                                 \
  generate_store_reg(a0, _rd);                                                \
}                                                                             \

#define generate_store_reg_pc_thumb(ireg, rd)                                 \
  generate_store_reg(ireg, rd);                                               \
  if(rd == 15)                                                                \
  {                                                                           \
    generate_mov(a0, ireg);                                                   \
    generate_indirect_branch_cycle_update(thumb);                             \
  }                                                                           \

#define thumb_data_proc_hi(name)                                              \
{                                                                             \
  thumb_decode_hireg_op();                                                    \
  generate_load_reg_pc(a0, rs, 4);                                            \
  generate_load_reg_pc(a1, rd, 4);                                            \
  arm_data_proc_##name(rd, generate_store_reg_pc_thumb);                      \
}                                                                             \

#define thumb_data_proc_test_hi(name)                                         \
{                                                                             \
  thumb_decode_hireg_op();                                                    \
  generate_load_reg_pc(a0, rs, 4);                                            \
  generate_load_reg_pc(a1, rd, 4);                                            \
  arm_data_proc_test_##name();                                                \
}                                                                             \

#define thumb_data_proc_unary_hi(name)                                        \
{                                                                             \
  thumb_decode_hireg_op();                                                    \
  generate_load_reg_pc(a0, rn, 4);                                            \
  arm_data_proc_unary_##name(rd, generate_store_reg_pc_thumb);                \
}                                                                             \

#define thumb_data_proc_mov_hi()                                              \
{                                                                             \
  thumb_decode_hireg_op();                                                    \
  generate_load_reg_pc(a0, rs, 4);                                            \
  generate_store_reg_pc_thumb(a0, rd);                                        \
}                                                                             \

#define thumb_load_pc(_rd)                                                    \
{                                                                             \
  thumb_decode_imm();                                                         \
  generate_load_pc(a0, (((pc & ~2) + 4) + (imm * 4)));                        \
  generate_store_reg(a0, _rd);                                                \
}                                                                             \

#define thumb_load_sp(_rd)                                                    \
{                                                                             \
  thumb_decode_imm();                                                         \
  generate_load_reg(a0, 13);                                                  \
  generate_add_imm(a0, (imm * 4));                                            \
  generate_store_reg(a0, _rd);                                                \
}                                                                             \

#define thumb_adjust_sp_up()                                                  \
  generate_add_imm(a0, imm * 4)                                               \

#define thumb_adjust_sp_down()                                                \
  generate_sub_imm(a0, imm * 4)                                               \

#define thumb_adjust_sp(direction)                                            \
{                                                                             \
  thumb_decode_add_sp();                                                      \
  generate_load_reg(a0, REG_SP);                                              \
  thumb_adjust_sp_##direction();                                              \
  generate_store_reg(a0, REG_SP);                                             \
}                                                                             \

/* Thumb shifts by immediate, x86 semantics (N/Z always from the result) */
#define thumb_lsl_imm_op()                                                    \
  if (imm) {                                                                  \
    xt_c_from_bit(reg_a0, 32 - imm);                                          \
    XT(slli, reg_a0, reg_a0, imm);                                            \
  }                                                                           \
  update_logical_flags()                                                      \

#define thumb_lsr_imm_op()                                                    \
  if (imm) {                                                                  \
    xt_c_from_bit(reg_a0, imm - 1);                                           \
    translation_ptr = xt_srl_imm(translation_ptr, reg_a0, imm);               \
  } else {                                                                    \
    xt_c_from_bit(reg_a0, 31);                                                \
    XT(movi, reg_a0, 0);                                                      \
  }                                                                           \
  update_logical_flags()                                                      \

#define thumb_asr_imm_op()                                                    \
  if (imm) {                                                                  \
    xt_c_from_bit(reg_a0, imm - 1);                                           \
    XT(srai, reg_a0, reg_a0, imm);                                            \
  } else {                                                                    \
    XT(srai, reg_a0, reg_a0, 31);                                             \
    xt_c_from_bit(reg_a0, 31);                                                \
  }                                                                           \
  update_logical_flags()                                                      \

#define thumb_ror_imm_op()                                                    \
  if (imm) {                                                                  \
    xt_rotate_right_imm(reg_a0, imm);                                         \
    xt_c_from_bit(reg_a0, 31);                                                \
  } else {                                                                    \
    generate_rrx_flags(a0);                                                   \
  }                                                                           \
  update_logical_flags()                                                      \

#define generate_shift_load_operands_reg()                                    \
  generate_load_reg(a0, rd);                                                  \
  generate_load_reg(a1, rs)                                                   \

#define generate_shift_load_operands_imm()                                    \
  generate_load_reg(a0, rs);                                                  \
  generate_load_imm(a1, imm)                                                  \

#define thumb_shift_operation_imm(op_type)                                    \
  thumb_##op_type##_imm_op()

/* Thumb shifts by register: the whole register is the amount (no & 0xFF) */
#define thumb_shift_operation_reg(op_type)                                    \
  translation_ptr = xt_shift_reg(translation_ptr, xt_shift_kind_##op_type, true); \
  update_logical_flags()                                                      \

#define thumb_shift(decode_type, op_type, value_type)                         \
{                                                                             \
  thumb_decode_##decode_type();                                               \
  generate_shift_load_operands_##value_type();                                \
  thumb_shift_operation_##value_type(op_type);                                \
  generate_store_reg(rv, rd);                                                 \
}                                                                             \

#define thumb_load_pc_pool_const(reg_rd, value)                               \
  generate_store_reg_i32(value, reg_rd)                                       \

#define thumb_access_memory_load(mem_type, reg_rd)                            \
  cycle_count += 2;                                                           \
  generate_load_pc(a1, pc);                                                   \
  xt_load(mem_type);                                                          \
  generate_store_reg(rv, reg_rd)                                              \

#define thumb_access_memory_store(mem_type, reg_rd)                           \
  cycle_count++;                                                              \
  generate_load_reg(a1, reg_rd);                                              \
  xt_store_pc(XT_FN_STORE_##mem_type, pc + 2)                                 \

#define thumb_access_memory_generate_address_pc_relative(offset, _rb, _ro)    \
  generate_load_pc(a0, (offset))                                              \

#define thumb_access_memory_generate_address_reg_imm_sp(offset, _rb, _ro)     \
  generate_load_reg(a0, _rb);                                                 \
  generate_add_imm(a0, (offset * 4))                                          \

#define thumb_access_memory_generate_address_reg_imm(offset, _rb, _ro)        \
  generate_load_reg(a0, _rb);                                                 \
  generate_add_imm(a0, (offset))                                              \

#define thumb_access_memory_generate_address_reg_reg(offset, _rb, _ro)        \
  generate_load_reg(a0, _rb);                                                 \
  generate_load_reg(a1, _ro);                                                 \
  generate_add(a0, a1)                                                        \

#define thumb_access_memory(access_type, op_type, _rd, _rb, _ro,              \
 address_type, offset, mem_type)                                              \
{                                                                             \
  thumb_decode_##op_type();                                                   \
  thumb_access_memory_generate_address_##address_type(offset, _rb, _ro);      \
  thumb_access_memory_##access_type(mem_type, _rd);                           \
}                                                                             \

#define thumb_block_address_preadjust_up()                                    \
  generate_add_imm(a0, (bit_count[reg_list] * 4))                             \

#define thumb_block_address_preadjust_down()                                  \
  generate_sub_imm(a0, (bit_count[reg_list] * 4))                             \

#define thumb_block_address_preadjust_push_lr()                               \
  generate_sub_imm(a0, ((bit_count[reg_list] + 1) * 4))                       \

#define thumb_block_address_preadjust_no()                                    \

#define thumb_block_address_postadjust_no(base_reg)                           \
  generate_store_reg(a0, base_reg)                                            \

#define thumb_block_address_postadjust_up(base_reg)                           \
  generate_add_imm(a0, (bit_count[reg_list] * 4));                            \
  generate_store_reg(a0, base_reg)                                            \

#define thumb_block_address_postadjust_down(base_reg)                         \
  generate_sub_imm(a0, (bit_count[reg_list] * 4));                            \
  generate_store_reg(a0, base_reg)                                            \

#define thumb_block_address_postadjust_pop_pc(base_reg)                       \
  generate_add_imm(a0, ((bit_count[reg_list] + 1) * 4));                      \
  generate_store_reg(a0, base_reg)                                            \

#define thumb_block_address_postadjust_push_lr(base_reg)                      \
  generate_store_reg(a0, base_reg)                                            \

#define thumb_block_memory_extra_no()                                         \

#define thumb_block_memory_extra_up()                                         \

#define thumb_block_memory_extra_down()                                       \

#define thumb_block_memory_extra_pop_pc()                                     \
  generate_load_reg(a0, REG_SAVE3);                                           \
  generate_add_imm(a0, (bit_count[reg_list] * 4));                            \
  generate_load_pc(a1, pc);                                                   \
  xt_load(u32);                                                               \
  generate_store_reg(rv, REG_PC);                                             \
  generate_indirect_branch_cycle_update(thumb)                                \

#define thumb_block_memory_extra_push_lr(base_reg)                            \
  generate_load_reg(a0, REG_SAVE3);                                           \
  generate_add_imm(a0, (bit_count[reg_list] * 4));                            \
  generate_load_reg(a1, REG_LR);                                              \
  xt_store(XT_FN_STORE_ALIGNED_U32)                                           \

#define thumb_block_memory_load()                                             \
  generate_load_pc(a1, pc);                                                   \
  xt_load(u32);                                                               \
  generate_store_reg(rv, i)                                                   \

#define thumb_block_memory_store()                                            \
  generate_load_reg(a1, i);                                                   \
  xt_store(XT_FN_STORE_ALIGNED_U32)                                           \

#define thumb_block_memory_final_load()                                       \
  thumb_block_memory_load()                                                   \

#define thumb_block_memory_final_store()                                      \
  generate_load_reg(a1, i);                                                   \
  xt_store_pc(XT_FN_STORE_U32, pc + 2)                                        \

#define thumb_block_memory_final_no(access_type)                              \
  thumb_block_memory_final_##access_type()                                    \

#define thumb_block_memory_final_up(access_type)                              \
  thumb_block_memory_final_##access_type()                                    \

#define thumb_block_memory_final_down(access_type)                            \
  thumb_block_memory_final_##access_type()                                    \

#define thumb_block_memory_final_push_lr(access_type)                         \
  thumb_block_memory_##access_type()                                          \

#define thumb_block_memory_final_pop_pc(access_type)                          \
  thumb_block_memory_##access_type()                                          \

#define thumb_block_memory(access_type, pre_op, post_op, base_reg)            \
{                                                                             \
  thumb_decode_rlist();                                                       \
  u32 i;                                                                      \
  u32 offset = 0;                                                             \
                                                                              \
  generate_load_reg(a0, base_reg);                                            \
  generate_and_imm(a0, ~0x03);                                                \
  thumb_block_address_preadjust_##pre_op();                                   \
  generate_store_reg(a0, REG_SAVE3);                                          \
  thumb_block_address_postadjust_##post_op(base_reg);                         \
                                                                              \
  for(i = 0; i < 8; i++)                                                      \
  {                                                                           \
    if((reg_list >> i) & 0x01)                                                \
    {                                                                         \
      cycle_count++;                                                          \
      generate_load_reg(a0, REG_SAVE3);                                       \
      generate_add_imm(a0, offset);                                           \
      if(reg_list & ~((2 << i) - 1))                                          \
      {                                                                       \
        thumb_block_memory_##access_type();                                   \
        offset += 4;                                                          \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        thumb_block_memory_final_##post_op(access_type);                      \
      }                                                                       \
    }                                                                         \
  }                                                                           \
                                                                              \
  thumb_block_memory_extra_##post_op();                                       \
}                                                                             \

#define thumb_conditional_branch(condition)                                   \
{                                                                             \
  generate_cycle_update();                                                    \
  generate_condition_##condition(a0);                                         \
  generate_branch_no_cycle_update(                                            \
   block_exits[block_exit_position].branch_source,                            \
   block_exits[block_exit_position].branch_target);                           \
  generate_branch_patch_conditional(backpatch_address, translation_ptr);      \
  block_exit_position++;                                                      \
}                                                                             \

/* ---- branches, SWI, HLE ---------------------------------------------------------------- */
#define arm_conditional_block_header()                                        \
  generate_cycle_update();                                                    \
  generate_condition(a0);                                                     \

#define arm_b()                                                               \
  generate_branch()                                                           \

#define arm_bl()                                                              \
  generate_load_pc(a0, (pc + 4));                                             \
  generate_store_reg(a0, REG_LR);                                             \
  generate_branch()                                                           \

#define arm_bx()                                                              \
  arm_decode_branchx(opcode);                                                 \
  generate_load_reg(a0, rn);                                                  \
  generate_indirect_branch_dual();                                            \

/* collapse the flags, then execute_swi(pc) (both in xt_swi) */
#define arm_swi()                                                             \
  generate_load_pc(a0, (pc + 4));                                             \
  xt_call(XT_FN_SWI);                                                         \
  generate_branch()                                                           \

#define thumb_b()                                                             \
  generate_branch_cycle_update(                                               \
   block_exits[block_exit_position].branch_source,                            \
   block_exits[block_exit_position].branch_target);                           \
  block_exit_position++                                                       \

#define thumb_bl()                                                            \
  generate_load_pc(a0, ((pc + 2) | 0x01));                                    \
  generate_store_reg(a0, REG_LR);                                             \
  generate_branch_cycle_update(                                               \
   block_exits[block_exit_position].branch_source,                            \
   block_exits[block_exit_position].branch_target);                           \
  block_exit_position++                                                       \

#define thumb_blh()                                                           \
{                                                                             \
  thumb_decode_branch();                                                      \
  generate_load_pc(a0, ((pc + 2) | 0x01));                                    \
  generate_load_reg(a1, REG_LR);                                              \
  generate_store_reg(a0, REG_LR);                                             \
  generate_mov(a0, a1);                                                       \
  generate_add_imm(a0, (offset * 2));                                         \
  generate_indirect_branch_cycle_update(thumb);                               \
}                                                                             \

#define thumb_bx()                                                            \
{                                                                             \
  thumb_decode_hireg_op();                                                    \
  generate_load_reg_pc(a0, rs, 4);                                            \
  generate_indirect_branch_cycle_update(dual);                                \
}                                                                             \

#define thumb_process_cheats()                                                \
  translation_ptr = xt_sync_to_mem(translation_ptr);                          \
  xt_call(XT_FN_PROCESS_CHEATS);                                              \
  translation_ptr = xt_sync_from_mem(translation_ptr);

#define arm_process_cheats()                                                  \
  translation_ptr = xt_sync_to_mem(translation_ptr);                          \
  xt_call(XT_FN_PROCESS_CHEATS);                                              \
  translation_ptr = xt_sync_from_mem(translation_ptr);

#define thumb_swi()                                                           \
  generate_load_pc(a0, (pc + 2));                                             \
  xt_call(XT_FN_SWI);                                                         \
  generate_branch_cycle_update(                                               \
   block_exits[block_exit_position].branch_source,                            \
   block_exits[block_exit_position].branch_target);                           \
  block_exit_position++                                                       \

#define arm_hle_div(cpu_mode)                                                 \
  translation_ptr = xt_sync_to_mem(translation_ptr);                          \
  xt_call(XT_FN_HLE_DIV);                                                     \
  translation_ptr = xt_sync_from_mem(translation_ptr)
#define arm_hle_div_arm(cpu_mode)                                             \
  translation_ptr = xt_sync_to_mem(translation_ptr);                          \
  xt_call(XT_FN_HLE_DIV_ARM);                                                 \
  translation_ptr = xt_sync_from_mem(translation_ptr)

/* the m4a mixer loop head (cpu.cpp): run it natively, or go on here */
#define xt_m4a_hook(hook_pc)                                                  \
  translation_ptr = xt_sync_to_mem(translation_ptr);                          \
  generate_load_pc(a0, (hook_pc));                                            \
  XT(s32i, reg_cycles, reg_base, XT_CYC_SLOT * 4);                            \
  xt_call(XT_FN_M4A);                                                         \
  translation_ptr = xt_sync_from_mem(translation_ptr);                        \
  xt_jump_if_redirect()

#define generate_translation_gate(type)                                       \
  generate_load_pc(a0, pc);                                                   \
  generate_indirect_branch_no_cycle_update(type)                              \

void init_emitter(bool must_swap);
u32 execute_arm_translate(u32 cycles);

#endif
