/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef NDRC_RV32_ABI_H
#define NDRC_RV32_ABI_H

/* Private RV32 ABI used by the shared new_dynarec compiler.
 * Keep x0, ra, sp, gp, tp out of the allocator. s0 is the local-state base,
 * s1 the live cycle counter, t3..t5 the private NZCV predicate state, t6 is
 * Ari64's exposed HOST_TEMPREG, and s11 is private emitter scratch. The last
 * two must be distinct: shared code frequently passes HOST_TEMPREG as an
 * operand to a helper which itself needs scratch storage.
 */
#define NDRC_RV32_HOST_REGS 21
#define NDRC_RV32_CC_SLOT 20
#define NDRC_RV32_CALLER_MASK 0x7ff
#define NDRC_RV32_FIRST_SAVED_SLOT 11
#define NDRC_RV32_LAST_SAVED_SLOT 19
#define NDRC_RV32_MEM_ADDR_SLOT 11
#define NDRC_RV32_MEM_VALUE_SLOT 12
#define NDRC_RV32_ENTRY_FRAME 64

#ifndef __ASSEMBLER__
#include "emit.h"

#define NDRC_RV32_AUX_REG RV_S11

static inline unsigned rv32_host_reg(unsigned slot)
{
  static const unsigned char regs[NDRC_RV32_HOST_REGS] = {
    RV_A0, RV_A1, RV_A2, RV_A3, RV_A4, RV_A5, RV_A6, RV_A7,
    RV_T0, RV_T1, RV_T2,
    RV_S2, RV_S3, RV_S4, RV_S5, RV_S6, RV_S7, RV_S8, RV_S9, RV_S10,
    RV_S1
  };
  return slot < NDRC_RV32_HOST_REGS ? regs[slot] : 32;
}

/* Full-address memory lowering may use these only after its boundary adapter
 * has written back and cleared both allocator slots. They are callee-saved, so
 * a slow-path C helper preserves the captured guest address and store value. */
static inline unsigned rv32_mem_addr_reg(void)
{
  return rv32_host_reg(NDRC_RV32_MEM_ADDR_SLOT);
}

static inline unsigned rv32_mem_value_reg(void)
{
  return rv32_host_reg(NDRC_RV32_MEM_VALUE_SLOT);
}

/* code enters with s0=local and s1=*cycles. It must tail-jump to leave,
 * with the entry stack restored. a0 is an optional return/status value.
 * No guest state, events, or C helper synchronization is implicit here.
 */
uint32_t ndrc_rv32_enter(void *code, void *local, uint32_t *cycles);
void ndrc_rv32_leave(void); /* generated-code target, never a C-callable API */

/* Save live caller slots plus reserved predicate registers and ra. C preserves
 * the allocator's s registers, including local and cycles. If slot 0 is live,
 * its old a0 is restored and the helper result is discarded; exclude slot 0 to
 * consume a returned a0. The compiler must marshal arguments BEFORE this call,
 * without destroying still-live guest registers (this is not an argument mover).
 */
static inline void rv32_call_preserved(struct rv32_emitter *e, uint32_t target,
                                      uint32_t live_slots)
{
  unsigned saved[15], count = 0, i, frame;
  if (!rv32_check(e, !(live_slots >> NDRC_RV32_HOST_REGS) && !(target & 1))) return;
  for (i = 0; i < 11; i++)
    if (live_slots & (1u << i)) saved[count++] = rv32_host_reg(i);
  saved[count++] = RV_T3; saved[count++] = RV_T4;
  saved[count++] = RV_T5; saved[count++] = RV_RA;
  frame = (count * 4 + 15) & ~15u;
  if (!rv32_reserve(e, 4 + 2 * count)) return;
  rv32_imm(e, RV_ADD, RV_SP, RV_SP, -(int32_t)frame);
  for (i = 0; i < count; i++) rv32_store(e, RV_WORD, saved[i], RV_SP, (int32_t)(i * 4));
  rv32_abs_jump(e, RV_RA, NDRC_RV32_AUX_REG, target);
  for (i = 0; i < count; i++) rv32_load(e, RV_WORD, saved[i], RV_SP, (int32_t)(i * 4));
  rv32_imm(e, RV_ADD, RV_SP, RV_SP, (int32_t)frame);
}
#endif
#endif
