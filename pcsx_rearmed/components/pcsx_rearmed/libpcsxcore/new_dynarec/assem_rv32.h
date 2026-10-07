/* SPDX-License-Identifier: GPL-2.0-or-later
 * Host contract for the real Ari64 new_dynarec RV32 backend.
 *
 * This is deliberately separate from assem_rv32_alu.h: that header adapts a
 * small ALU fixture, while this one describes the register and cache contract
 * consumed by the complete nine-pass compiler in new_dynarec.c.
 */
#ifndef PCSX_NDRC_ASSEM_RV32_H
#define PCSX_NDRC_ASSEM_RV32_H

#include "rv32/abi.h"

#define HOST_IMM8 1
#define BASE_ADDR_DYNAMIC 1

#define HOST_REGS NDRC_RV32_HOST_REGS
#define HOST_CCREG NDRC_RV32_CC_SLOT
#define HOST_TEMPREG NDRC_RV32_HOST_REGS
#define HOST_FPREG (NDRC_RV32_HOST_REGS + 1)
#define EXCLUDE_REG -1

/* s0 owns &dynarec_local and s1 owns CCREG while generated code is active.
 * Give s0 a pseudo-slot outside the allocator range. Emitter helpers accept
 * allocator slots, not raw RV register numbers; defining FP as RV_S0 (8)
 * accidentally remapped it to allocator slot 8 (t0). */
#define FP HOST_FPREG
#define CALLER_SAVE_REGS NDRC_RV32_CALLER_MASK
#define PREFERRED_REG_FIRST NDRC_RV32_FIRST_SAVED_SLOT
#define PREFERRED_REG_LAST NDRC_RV32_LAST_SAVED_SLOT
#define DRC_DBG_REGMASK CALLER_SAVE_REGS

/* Keep MIPS link destinations coherent in psxRegs at their creation point.
 * RV32 can otherwise carry a link only in a host mapping across an external
 * dispatch, allowing a later block to reload stale architectural state. This
 * also makes link generation materialize the return PC unconditionally: the
 * pre-branch loaded-constant mask may describe a different slot mapping. */
#define WRITE_LINK_REGISTER_EARLY 1

/* Temporary saves around C helpers use a self-contained 64-byte frame. */
#define RV32_CALL_SAVE_FRAME 64
#define NDRC_PATCH_LINK_SIZE 48

extern char *invc_ptr;

#if defined(ESP_PLATFORM) && defined(CONFIG_IDF_TARGET_ESP32P4)
void *ndrc_rv32_p4_alloc_exec(size_t size, void **backing);
void ndrc_rv32_p4_free_exec(void *mapping, void *backing);
int ndrc_rv32_p4_publish(void *code, size_t size);
#endif

/* ESP32-P4 currently has room for an 8 MiB executable cache alongside PSX
 * RAM/VRAM. This can be tuned after the real backend replaces the prototype. */
#define TARGET_SIZE_2 23

struct tramp_insns {
  uint32_t unused;
};

#endif
