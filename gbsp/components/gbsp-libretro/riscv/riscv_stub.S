# gameplaySP
#
# Copyright (C) 2006 Exophase <exophase@gmail.com>
# RISC-V port for ESP32-P4 (rv32imafc)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.

#include "../gpsp_config.h"

#define defsymbl(symbol)  \
.type symbol, @function ; \
.global symbol ;          \
symbol:

#define defobj(symbol)  \
.type symbol, @object ; \
.global symbol ;        \
symbol:

# ═══════════════════════════════════════════════════════════════
# RISC-V register mapping (matches riscv_emit.h)
#
# s0  = reg_base     (GBA register array base pointer)
# s1  = reg_cycles   (cycle counter, counts down)
# s2  = reg_pc       (stored PC / block start PC)
# s3  = reg_n_cache  (N flag)
# s4  = reg_z_cache  (Z flag)
# s5  = reg_c_cache  (C flag)
# s6  = reg_v_cache  (V flag)
#
# GBA r0-r14 -> host regs:
# t0=r0, t1=r1, t2=r2, a3=r3, a4=r4, a5=r5, a6=r6, a7=r7,
# s7=r8, s8=r9, s9=r10, s10=r11, s11=r12, t3=r13, t4=r14
#
# a0=r15 (PC target / fn arg0 / return value)
# a1=fn arg1, a2=fn arg2
# t5=reg_temp (scratch), t6=reg_rv (secondary scratch)
# ═══════════════════════════════════════════════════════════════

# Register aliases
#define reg_base    s0
#define reg_cycles  s1
#define reg_pc      s2
#define reg_n_cache s3
#define reg_z_cache s4
#define reg_c_cache s5
#define reg_v_cache s6

#define reg_r0  t0
#define reg_r1  t1
#define reg_r2  t2
#define reg_r3  a3
#define reg_r4  a4
#define reg_r5  a5
#define reg_r6  a6
#define reg_r7  a7
#define reg_r8  s7
#define reg_r9  s8
#define reg_r10 s9
#define reg_r11 s10
#define reg_r12 s11
#define reg_r13 t3
#define reg_r14 t4

#define reg_temp t5
#define reg_rv   t6

# GBA register file offsets (from reg_base = &reg[0])
.equ REG_R0,              (0 * 4)
.equ REG_R1,              (1 * 4)
.equ REG_R2,              (2 * 4)
.equ REG_R3,              (3 * 4)
.equ REG_R4,              (4 * 4)
.equ REG_R5,              (5 * 4)
.equ REG_R6,              (6 * 4)
.equ REG_R7,              (7 * 4)
.equ REG_R8,              (8 * 4)
.equ REG_R9,              (9 * 4)
.equ REG_R10,             (10 * 4)
.equ REG_R11,             (11 * 4)
.equ REG_R12,             (12 * 4)
.equ REG_R13,             (13 * 4)
.equ REG_R14,             (14 * 4)
.equ REG_PC,              (15 * 4)
.equ REG_CPSR,            (16 * 4)
.equ CPU_MODE,            (17 * 4)
.equ CPU_HALT_STATE,      (18 * 4)

.equ REG_BUS_VALUE,       (19 * 4)
.equ REG_N_FLAG,          (20 * 4)
.equ REG_Z_FLAG,          (21 * 4)
.equ REG_C_FLAG,          (22 * 4)
.equ REG_V_FLAG,          (23 * 4)
.equ REG_SLEEP_CYCLES,    (24 * 4)
.equ OAM_UPDATED,         (25 * 4)
.equ REG_SAVE,            (26 * 4)
.equ REG_SAVE2,           (27 * 4)
.equ REG_SAVE3,           (28 * 4)
.equ REG_SAVE4,           (29 * 4)

.equ CPU_ALERT_HALT,      (1 << 0)
.equ CPU_ALERT_SMC,       (1 << 1)
.equ CPU_ALERT_IRQ,       (1 << 2)

# On ESP32-P4, reg/spsr/reg_mode are contiguous in PSRAM (gpsp_memory_alloc.c)
# NOT the MIPS layout which has palette_ram etc. between reg and spsr.
.equ SPSR_BASE,           (0x100)              # right after reg[64] (256 bytes)
.equ REGMODE_BASE,        (SPSR_BASE + 24)     # right after spsr[6] (24 bytes)
.equ SUPERVISOR_MODE,     (0x13)
.equ SUPERVISOR_SPSR,     (3 * 4 + SPSR_BASE)
.equ SUPERVISOR_LR,       ((3 * (7 * 4)) + (6 * 4) + REGMODE_BASE)

# Stack frame: 14 words (s0-s11 + ra + padding = 14*4 = 56 bytes)
.equ FRAME_SIZE,          64    # 16-byte aligned

.text
.balign 4

# ═══════════════════════════════════════════════════════════════
# Flag collapse/extract macros
#
# CPSR bits: N=31, Z=30, C=29, V=28
# Flag regs hold 0 or 1 in the LSB
# ═══════════════════════════════════════════════════════════════

.macro collapse_flags scratch
  # Load CPSR, keep lower 28 bits, insert flags into upper 4 bits
  lw    \scratch, REG_CPSR(reg_base)
  li    reg_temp, 0x0FFFFFFF
  and   \scratch, \scratch, reg_temp   # clear top 4 bits
  slli  reg_temp, reg_n_cache, 31
  or    \scratch, \scratch, reg_temp
  slli  reg_temp, reg_z_cache, 30
  or    \scratch, \scratch, reg_temp
  slli  reg_temp, reg_c_cache, 29
  or    \scratch, \scratch, reg_temp
  slli  reg_temp, reg_v_cache, 28
  or    \scratch, \scratch, reg_temp
  sw    \scratch, REG_CPSR(reg_base)
.endm

.macro extract_flags scratch
  # Load CPSR, extract flags into flag registers
  lw    \scratch, REG_CPSR(reg_base)
  srli  reg_n_cache, \scratch, 31
  andi  reg_n_cache, reg_n_cache, 1
  srli  reg_z_cache, \scratch, 30
  andi  reg_z_cache, reg_z_cache, 1
  srli  reg_c_cache, \scratch, 29
  andi  reg_c_cache, reg_c_cache, 1
  srli  reg_v_cache, \scratch, 28
  andi  reg_v_cache, reg_v_cache, 1
.endm

.macro extract_flags_body scratch
  # Extract flags from \scratch (already loaded)
  srli  reg_n_cache, \scratch, 31
  andi  reg_n_cache, reg_n_cache, 1
  srli  reg_z_cache, \scratch, 30
  andi  reg_z_cache, reg_z_cache, 1
  srli  reg_c_cache, \scratch, 29
  andi  reg_c_cache, reg_c_cache, 1
  srli  reg_v_cache, \scratch, 28
  andi  reg_v_cache, reg_v_cache, 1
.endm

# ═══════════════════════════════════════════════════════════════
# Save/restore GBA registers to/from reg[] array
# ═══════════════════════════════════════════════════════════════

.macro save_registers
  sw    reg_r0,  REG_R0(reg_base)
  sw    reg_r1,  REG_R1(reg_base)
  sw    reg_r2,  REG_R2(reg_base)
  sw    reg_r3,  REG_R3(reg_base)
  sw    reg_r4,  REG_R4(reg_base)
  sw    reg_r5,  REG_R5(reg_base)
  sw    reg_r6,  REG_R6(reg_base)
  sw    reg_r7,  REG_R7(reg_base)
  sw    reg_r8,  REG_R8(reg_base)
  sw    reg_r9,  REG_R9(reg_base)
  sw    reg_r10, REG_R10(reg_base)
  sw    reg_r11, REG_R11(reg_base)
  sw    reg_r12, REG_R12(reg_base)
  sw    reg_r13, REG_R13(reg_base)
  sw    reg_r14, REG_R14(reg_base)
.endm

.macro restore_registers
  lw    reg_r0,  REG_R0(reg_base)
  lw    reg_r1,  REG_R1(reg_base)
  lw    reg_r2,  REG_R2(reg_base)
  lw    reg_r3,  REG_R3(reg_base)
  lw    reg_r4,  REG_R4(reg_base)
  lw    reg_r5,  REG_R5(reg_base)
  lw    reg_r6,  REG_R6(reg_base)
  lw    reg_r7,  REG_R7(reg_base)
  lw    reg_r8,  REG_R8(reg_base)
  lw    reg_r9,  REG_R9(reg_base)
  lw    reg_r10, REG_R10(reg_base)
  lw    reg_r11, REG_R11(reg_base)
  lw    reg_r12, REG_R12(reg_base)
  lw    reg_r13, REG_R13(reg_base)
  lw    reg_r14, REG_R14(reg_base)
.endm

# ═══════════════════════════════════════════════════════════════
# riscv_update_gba — called when cycle counter expires
#
# a0: current PC (next address)
# Returns to translated code or exits to main
# ═══════════════════════════════════════════════════════════════

.balign 64
defsymbl(riscv_update_gba)
  sw    a0, REG_PC(reg_base)          # store current PC
  sw    ra, REG_SAVE2(reg_base)       # save return address
  collapse_flags a0                    # fold flags into CPSR (clobbers a0)
  save_registers

  mv    a0, reg_cycles                # remaining cycles as arg0
  call  update_gba                    # returns in a0

  bltz  a0, return_to_main            # bit 31 set = frame complete

  li    reg_temp, 0x7FFF
  and   reg_cycles, a0, reg_temp      # update cycle count

  slli  reg_temp, a0, 1               # check bit 30 (PC changed)
  bltz  reg_temp, lookup_pc

  lw    ra, REG_SAVE2(reg_base)       # restore return address
  restore_registers
  ret                                  # return to translated code


# ═══════════════════════════════════════════════════════════════
# riscv_cheat_hook — called at master PC for cheat processing
# ═══════════════════════════════════════════════════════════════

defsymbl(riscv_cheat_hook)
  sw    ra, REG_SAVE2(reg_base)
  save_registers
  call  process_cheats
  lw    ra, REG_SAVE2(reg_base)
  restore_registers
  ret


# ═══════════════════════════════════════════════════════════════
# return_to_main — exit dynarec, restore callee-saved, return to C
# ARM regs must already be saved before reaching here
# ═══════════════════════════════════════════════════════════════

return_to_main:
  lw    s0,  0*4(sp)
  lw    s1,  1*4(sp)
  lw    s2,  2*4(sp)
  lw    s3,  3*4(sp)
  lw    s4,  4*4(sp)
  lw    s5,  5*4(sp)
  lw    s6,  6*4(sp)
  lw    s7,  7*4(sp)
  lw    s8,  8*4(sp)
  lw    s9,  9*4(sp)
  lw    s10, 10*4(sp)
  lw    s11, 11*4(sp)
  lw    ra,  12*4(sp)
  addi  sp, sp, FRAME_SIZE
  ret


# ═══════════════════════════════════════════════════════════════
# Indirect branch handlers
#
# a0: GBA address to branch to
# Saves regs, calls block_lookup_address_*, restores, jumps there
# ═══════════════════════════════════════════════════════════════

defsymbl(riscv_indirect_branch_arm)
  save_registers
  sw    a0, REG_PC(reg_base)           # store target in reg[PC]
  call  block_lookup_address_arm       # returns native addr in a0
  beqz  a0, lookup_pc                  # NULL = failed, fallback
  mv    reg_temp, a0                   # save result
  restore_registers
  jr    reg_temp                       # jump to translated block

defsymbl(riscv_indirect_branch_thumb)
  sw    reg_pc, REG_SAVE3(reg_base)    # debug: stored_pc of the branching block
  save_registers
  sw    a0, REG_PC(reg_base)           # store target in reg[PC]
  call  block_lookup_address_thumb
  beqz  a0, lookup_pc                  # NULL = failed, fallback
  mv    reg_temp, a0
  restore_registers
  jr    reg_temp

defsymbl(riscv_indirect_branch_dual)
  sw    a0, REG_SAVE4(reg_base)        # debug: save a0 before save_registers
  sw    ra, REG_SAVE2(reg_base)        # debug: save return address (JIT caller)
  sw    reg_pc, REG_SAVE3(reg_base)    # debug: stored_pc of the branching block
  save_registers
  # For dual, strip thumb bit before storing PC
  andi  reg_temp, a0, -2
  sw    reg_temp, REG_PC(reg_base)     # store target in reg[PC]
  call  block_lookup_address_dual
  beqz  a0, lookup_pc                  # NULL = failed, fallback
  mv    reg_temp, a0
  restore_registers
  jr    reg_temp


# ═══════════════════════════════════════════════════════════════
# write_io_epilogue — called after I/O writes with side-effects
#
# a0: alert flags (CPU_ALERT_SMC | CPU_ALERT_IRQ | CPU_ALERT_HALT)
# ═══════════════════════════════════════════════════════════════

defsymbl(write_io_epilogue)
  mv    reg_pc, a0                     # use reg_pc as temp (will be reloaded)
  collapse_flags a0                    # update CPSR

  andi  a0, reg_pc, CPU_ALERT_SMC
  beqz  a0, 1f
  call  flush_translation_cache_ram
1:
  andi  a0, reg_pc, CPU_ALERT_IRQ
  beqz  a0, 2f
  call  check_and_raise_interrupts
2:
  andi  a0, reg_pc, CPU_ALERT_HALT
  beqz  a0, lookup_pc                 # not halted, resume
  # Fall through to cpu_sleep_loop

cpu_sleep_loop:
  mv    a0, reg_cycles
  call  update_gba
  bltz  a0, return_to_main            # frame complete, exit

  li    reg_temp, 0x7FFF
  and   reg_cycles, a0, reg_temp      # update cycle count
  # Fall through to lookup_pc

# ═══════════════════════════════════════════════════════════════
# lookup_pc — find translated block for current PC and jump to it
# ═══════════════════════════════════════════════════════════════

lookup_pc:
  extract_flags reg_temp
lookup_pc_noflags:
  lw    reg_temp, REG_CPSR(reg_base)
  andi  reg_temp, reg_temp, 0x20       # T bit
  lw    a0, REG_PC(reg_base)           # load PC as arg

  bnez  reg_temp, 1f
  # ARM mode
  call  block_lookup_address_arm
  j     2f
1:
  # Thumb mode
  call  block_lookup_address_thumb
2:
  beqz  a0, return_to_main            # catastrophic: can't translate PC
  restore_registers
  jr    a0                             # jump to result


# ═══════════════════════════════════════════════════════════════
# smc_write — self-modifying code detected
# ═══════════════════════════════════════════════════════════════

defsymbl(smc_write)
  sw    a2, REG_PC(reg_base)           # save PC (a2 = current PC in emit.h)
  save_registers
  call  flush_translation_cache_ram
  lw    reg_temp, REG_CPSR(reg_base)
  j     lookup_pc_noflags


# ═══════════════════════════════════════════════════════════════
# execute_read_cpsr — collapse flags and return CPSR in a0
# ═══════════════════════════════════════════════════════════════

defsymbl(execute_read_cpsr)
  collapse_flags a0
  ret


# ═══════════════════════════════════════════════════════════════
# execute_read_spsr — return spsr[cpu_mode] in a0
# ═══════════════════════════════════════════════════════════════

defsymbl(execute_read_spsr)
  lw    reg_temp, CPU_MODE(reg_base)
  andi  reg_temp, reg_temp, 0xF        # remove privilege bits
  slli  reg_temp, reg_temp, 2          # word offset
  add   reg_temp, reg_temp, reg_base
  li    reg_rv, SPSR_BASE
  add   reg_temp, reg_temp, reg_rv
  lw    a0, 0(reg_temp)
  ret


# ═══════════════════════════════════════════════════════════════
# execute_swi — switch to SWI (supervisor) mode
#
# a0: current PC (return address for SWI)
# ═══════════════════════════════════════════════════════════════

defsymbl(execute_swi)
  sw    ra, REG_SAVE3(reg_base)
  li    reg_rv, SUPERVISOR_LR
  add   reg_rv, reg_rv, reg_base
  sw    a0, 0(reg_rv)                  # save next PC in supervisor LR

  collapse_flags a0                    # get CPSR in a0
  li    reg_rv, SUPERVISOR_SPSR
  add   reg_rv, reg_rv, reg_base
  sw    a0, 0(reg_rv)                  # save CPSR in supervisor SPSR

  # Clear bottom 6 bits, set mode=supervisor + disable IRQs
  srli  a0, a0, 6
  slli  a0, a0, 6
  ori   a0, a0, (0x13 | 0x80)
  sw    a0, REG_CPSR(reg_base)

  save_registers
  li    a0, SUPERVISOR_MODE
  call  set_cpu_mode

  lw    ra, REG_SAVE3(reg_base)
  restore_registers

  # Update open bus value
  li    a0, 0xe3a02004
  sw    a0, REG_BUS_VALUE(reg_base)
  ret


# ═══════════════════════════════════════════════════════════════
# execute_spsr_restore — restore SPSR on exception return
#
# a0: GBA address to return to
# Returns new address in a0 (may have Thumb bit set)
# ═══════════════════════════════════════════════════════════════

defsymbl(execute_spsr_restore)
  lw    reg_temp, CPU_MODE(reg_base)
  andi  reg_temp, reg_temp, 0xF

  beqz  reg_temp, no_spsr_restore      # user/sys mode: no restore

  slli  a1, reg_temp, 2               # word offset
  add   a1, a1, reg_base
  li    reg_rv, SPSR_BASE
  add   a1, a1, reg_rv
  lw    reg_temp, 0(a1)               # reg_temp = spsr[cpu_mode]
  sw    reg_temp, REG_CPSR(reg_base)   # cpsr = spsr[cpu_mode]
  extract_flags_body reg_temp          # extract flags from reg_temp

  sw    ra, REG_SAVE3(reg_base)
  save_registers
  # a0 already has the address
  call  execute_spsr_restore_body      # returns new address in a0
  restore_registers
  lw    ra, REG_SAVE3(reg_base)
  ret

no_spsr_restore:
  ret


# ═══════════════════════════════════════════════════════════════
# execute_store_cpsr — write to CPSR with flag/mode update
#
# a0: new CPSR value (partial)
# a1: current PC
# After the call instruction in JIT code, two u32 words follow:
#   [ra+0]: store_mask (user mode)
#   [ra+4]: store_mask (privileged mode)
# We read the mask based on current mode, advance ra past them.
# ═══════════════════════════════════════════════════════════════

defsymbl(execute_store_cpsr)
  # Determine which mask to use based on privilege bit
  lw    a2, CPU_MODE(reg_base)
  andi  a2, a2, 0x10                  # privilege bit
  srli  a2, a2, 2                     # 0 or 4
  add   a2, a2, ra                    # point to correct mask
  lw    a2, 0(a2)                     # a2 = store_mask
  addi  ra, ra, 8                     # advance ra past the two masks

  and   reg_temp, a0, a2              # reg_temp = new_cpsr & store_mask
  lw    reg_rv, REG_CPSR(reg_base)    # reg_rv = current CPSR
  not   a0, a2                        # a0 = ~store_mask
  and   reg_rv, reg_rv, a0            # reg_rv = (cpsr & ~store_mask)
  or    reg_temp, reg_temp, reg_rv    # reg_temp = combined new CPSR
  extract_flags_body reg_temp         # extract flags from new CPSR

  andi  a2, a2, 0xFF                  # did we overwrite mode bits?
  sw    reg_temp, REG_CPSR(reg_base)  # store new CPSR
  beqz  a2, 1f                        # if not, just return

  # Mode bits changed — need to call C function
  sw    ra, REG_SAVE3(reg_base)
  save_registers
  mv    a0, reg_temp                   # new CPSR as arg0
  call  execute_store_cpsr_body        # returns new address (0=no change)

  beqz  a0, 2f                        # no PC change

  # PC changed — look up new block
  call  block_lookup_address_arm
  mv    reg_temp, a0
  restore_registers
  jr    reg_temp

2:
  restore_registers
  lw    ra, REG_SAVE3(reg_base)
1:
  ret


# ═══════════════════════════════════════════════════════════════
# execute_store_spsr — write to SPSR
#
# a0: new SPSR value (partial)
# a1: store mask
# ═══════════════════════════════════════════════════════════════

defsymbl(execute_store_spsr)
  lw    reg_temp, CPU_MODE(reg_base)
  andi  reg_temp, reg_temp, 0xF
  slli  reg_temp, reg_temp, 2          # word offset
  add   reg_temp, reg_temp, reg_base
  li    a2, SPSR_BASE
  add   reg_temp, reg_temp, a2
  lw    reg_rv, 0(reg_temp)           # reg_rv = spsr[cpu_mode]
  and   a0, a0, a1                    # a0 = new_spsr & store_mask
  not   a1, a1                        # a1 = ~store_mask
  and   reg_rv, reg_rv, a1            # reg_rv = (spsr & ~store_mask)
  or    a0, a0, reg_rv                # a0 = new spsr combined
  sw    a0, 0(reg_temp)
  ret


# ═══════════════════════════════════════════════════════════════
# execute_arm_translate_internal — entry point from C
#
# a0: cycle count
# a1: pointer to reg[] array
#
# Saves callee-saved regs, sets up dynarec state, enters JIT code
# ═══════════════════════════════════════════════════════════════

.balign 16
defsymbl(execute_arm_translate_internal)
  addi  sp, sp, -FRAME_SIZE
  sw    s0,  0*4(sp)
  sw    s1,  1*4(sp)
  sw    s2,  2*4(sp)
  sw    s3,  3*4(sp)
  sw    s4,  4*4(sp)
  sw    s5,  5*4(sp)
  sw    s6,  6*4(sp)
  sw    s7,  7*4(sp)
  sw    s8,  8*4(sp)
  sw    s9,  9*4(sp)
  sw    s10, 10*4(sp)
  sw    s11, 11*4(sp)
  sw    ra,  12*4(sp)

  mv    reg_base, a1                   # set up register base pointer
  mv    reg_cycles, a0                 # load cycle counter

  extract_flags reg_temp               # load flag variables from CPSR

  # Check if CPU is sleeping
  lw    reg_temp, CPU_HALT_STATE(reg_base)
  bnez  reg_temp, cpu_sleep_loop

  # Check Thumb bit
  lw    reg_temp, REG_CPSR(reg_base)
  andi  reg_temp, reg_temp, 0x20
  lw    a0, REG_PC(reg_base)           # load PC as arg

  bnez  reg_temp, 1f

  # ARM mode entry
  call  block_lookup_address_arm
  beqz  a0, return_to_main
  restore_registers
  jr    a0

1:
  # Thumb mode entry
  call  block_lookup_address_thumb
  beqz  a0, return_to_main
  restore_registers
  jr    a0


# ═══════════════════════════════════════════════════════════════
# Translation cache (executable section)
# On ESP32-P4, we use MMAP_JIT_CACHE (heap-allocated), so these
# are not needed here. If not using MMAP, uncomment below.
# ═══════════════════════════════════════════════════════════════

# ═══════════════════════════════════════════════════════════════
# Register file layout in PSRAM (.ext_ram.bss)
#
# Must match the offsets used by SPSR_BASE, REGMODE_BASE, etc.
# Defined here (not in C) to guarantee contiguous placement.
# ═══════════════════════════════════════════════════════════════

.section .ext_ram.bss.regfile,"aw",@nobits
.balign 64

defobj(reg)
  .space 0x100          # u32 reg[64] = 256 bytes

defobj(spsr)
  .space 24             # u32 spsr[6] = 24 bytes

defobj(reg_mode)
  .space 196            # u32 reg_mode[7][7] = 196 bytes

#if !defined(MMAP_JIT_CACHE)
.section .jit,"awx",@nobits
.balign 4

defsymbl(rom_translation_cache)
  .space ROM_TRANSLATION_CACHE_SIZE
defsymbl(ram_translation_cache)
  .space RAM_TRANSLATION_CACHE_SIZE
#endif
