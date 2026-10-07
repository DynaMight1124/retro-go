/* SPDX-License-Identifier: GPL-2.0-or-later
 * Runtime linkage for the Ari64 RV32 new_dynarec backend.
 */
#include "linkage_offsets.h"

	.section .bss
	.balign 16
	.globl dynarec_local
	.type dynarec_local, @object
	.size dynarec_local, LO_dynarec_local_size
dynarec_local:
	.space LO_dynarec_local_size

#define DRC_VAR(name, offset, size_) \
	.globl name; \
	.type name, @object; \
	.set name, dynarec_local + offset; \
	.size name, size_

DRC_VAR(cycle_count, LO_cycle_count, 4)
DRC_VAR(last_count, LO_last_count, 4)
DRC_VAR(address, LO_address, 4)
DRC_VAR(hack_addr, LO_hack_addr, 4)
DRC_VAR(psxRegs, LO_psxRegs, LO_psxRegs_end - LO_psxRegs)
DRC_VAR(reg_cop2d, LO_reg_cop2d, 128)
DRC_VAR(reg_cop2c, LO_reg_cop2c, 128)
DRC_VAR(rcnts, LO_rcnts, 7 * 4 * 4)
DRC_VAR(inv_code_start, LO_inv_code_start, 4)
DRC_VAR(inv_code_end, LO_inv_code_end, 4)
DRC_VAR(mem_rtab, LO_mem_rtab, 4)
DRC_VAR(mem_wtab, LO_mem_wtab, 4)
DRC_VAR(psxH_ptr, LO_psxH_ptr, 4)
DRC_VAR(zeromem_ptr, LO_zeromem_ptr, 4)
DRC_VAR(invc_ptr, LO_invc_ptr, 4)
DRC_VAR(scratch_buf_ptr, LO_scratch_buf_ptr, 4)
DRC_VAR(ram_offset, LO_ram_offset, 4)
DRC_VAR(hash_table_ptr, LO_hash_table_ptr, 4)
DRC_VAR(mini_ht, LO_mini_ht, 32 * 2 * 4)

	.section .text
	.balign 4
	.option push
	.option norvc

.macro FUNCTION name
	.globl \name
	.type \name, @function
\name:
.endm

/* The fixed external-jump stub supplies:
 *   a0 = guest target, a1 = source jump, a2 = stub address.
 * Preserve every other allocator-visible caller register while C performs
 * the lookup and optional patch. s0/s1 and s2-s11 are protected by the ABI.
 */
FUNCTION dyna_linker
	addi sp, sp, -64
	sw ra, 0(sp)
	sw a3, 4(sp)
	sw a4, 8(sp)
	sw a5, 12(sp)
	sw a6, 16(sp)
	sw a7, 20(sp)
	sw t0, 24(sp)
	sw t1, 28(sp)
	sw t2, 32(sp)
	sw t3, 36(sp)
	sw t4, 40(sp)
	sw t5, 44(sp)
	sw a0, 48(sp)
	sw a1, 52(sp)
	sw a2, 56(sp)

	mv a1, a0
	lw a0, LO_hash_table_ptr(s0)
	li a2, 0
	call ndrc_get_addr_ht_param
	beqz a0, .Ldyna_compile

	sw a0, 60(sp)
	mv a3, a0
	lw a0, 48(sp)
	lw a1, 52(sp)
	lw a2, 56(sp)
	call ndrc_patch_link
	lw t6, 60(sp)
	j .Ldyna_restore_jump

.Ldyna_compile:
	lw a0, 48(sp)
	lw a1, LO_hash_table_ptr(s0)
	call ndrc_get_addr_ht
	mv t6, a0

.Ldyna_restore_jump:
	lw ra, 0(sp)
	lw a3, 4(sp)
	lw a4, 8(sp)
	lw a5, 12(sp)
	lw a6, 16(sp)
	lw a7, 20(sp)
	lw t0, 24(sp)
	lw t1, 28(sp)
	lw t2, 32(sp)
	lw t3, 36(sp)
	lw t4, 40(sp)
	lw t5, 44(sp)
	addi sp, sp, 64
	jr t6
	.size dyna_linker, . - dyna_linker

/* Enter generated code with s0 as the local-state base and s1 as Ari64's
 * relative cycle counter. The complete integer callee-save set is retained
 * because every one of s2-s11 is allocator-visible. */
.macro save_entry_frame
	addi sp, sp, -64
	sw ra, 0(sp)
	sw s0, 4(sp)
	sw s1, 8(sp)
	sw s2, 12(sp)
	sw s3, 16(sp)
	sw s4, 20(sp)
	sw s5, 24(sp)
	sw s6, 28(sp)
	sw s7, 32(sp)
	sw s8, 36(sp)
	sw s9, 40(sp)
	sw s10, 44(sp)
	sw s11, 48(sp)
.endm

FUNCTION new_dyna_start_at
	save_entry_frame
	mv s0, a0
	mv t0, a1
	j .Lnew_dyna_enter
	.size new_dyna_start_at, . - new_dyna_start_at

FUNCTION new_dyna_start
	save_entry_frame
	mv s0, a0
	lw a0, LO_pcaddr(s0)
	lw a1, LO_hash_table_ptr(s0)
	call ndrc_get_addr_ht
	mv t0, a0

.Lnew_dyna_enter:
	lw t1, LO_next_interupt(s0)
	lw s1, LO_cycle(s0)
	sw t1, LO_last_count(s0)
	sub s1, s1, t1
	jr t0
	.size new_dyna_start, . - new_dyna_start

FUNCTION new_dyna_leave
	lw t0, LO_last_count(s0)
	add s1, s1, t0
	sw s1, LO_cycle(s0)
	lw ra, 0(sp)
	lw s0, 4(sp)
	lw s1, 8(sp)
	lw s2, 12(sp)
	lw s3, 16(sp)
	lw s4, 20(sp)
	lw s5, 24(sp)
	lw s6, 28(sp)
	lw s7, 32(sp)
	lw s8, 36(sp)
	lw s9, 40(sp)
	lw s10, 44(sp)
	lw s11, 48(sp)
	addi sp, sp, 64
	ret
	.size new_dyna_leave, . - new_dyna_leave

/* Second-level memory dispatch, following the established ARM/AArch64
 * linkage contract. Generated code has already resolved the first-level
 * page entry and passes its decoded secondary table in a1 (reads) or a3
 * (writes). Keeping the remaining lookup here avoids a C bridge on every
 * dynamic hardware access.
 *
 * Reads:  a0=address, a1=secondary table, a2=relative cycles; result in a0.
 * Writes: a0=address, a1=data, a2=relative cycles, a3=secondary table;
 *         updated relative cycles returned in a0.
 */
.macro memhandler_pre_rv32
	lw t0, LO_last_count(s0)
	add t0, t0, a2
	sw t0, LO_cycle(s0)
.endm

.macro memhandler_post_rv32
	lw t0, LO_next_interupt(s0)
	lw a0, LO_cycle(s0)
	sw t0, LO_last_count(s0)
	sub a0, a0, t0
.endm

.macro pcsx_read_mem name, loadop, shift, table_offset
FUNCTION \name
	/* Extract (address & 0xfff) >> shift without a large immediate. */
	slli t0, a0, 20
	srli t0, t0, (20 + \shift)
	li t1, \table_offset
	add t1, a1, t1
	slli t2, t0, 2
	add t1, t1, t2
	lw t1, 0(t1)
	bltz t1, 1f
	slli t1, t1, 1
	slli t0, t0, \shift
	add t1, t1, t0
	\loadop a0, 0(t1)
	ret
1:
	slli t1, t1, 1
	addi sp, sp, -16
	sw ra, 0(sp)
	memhandler_pre_rv32
	jalr ra, t1, 0
	lw ra, 0(sp)
	addi sp, sp, 16
	ret
	.size \name, . - \name
.endm

pcsx_read_mem jump_handler_read8,  lbu, 0, 0x3000
pcsx_read_mem jump_handler_read16, lhu, 1, 0x1000
pcsx_read_mem jump_handler_read32, lw,  2, 0

.macro pcsx_write_mem name, storeop, shift, table_offset
FUNCTION \name
	slli t0, a0, 20
	srli t0, t0, (20 + \shift)
	li t1, \table_offset
	add t1, a3, t1
	slli t2, t0, 2
	add t1, t1, t2
	lw t1, 0(t1)
	bltz t1, 1f
	slli t1, t1, 1
	slli t0, t0, \shift
	add t1, t1, t0
	\storeop a1, 0(t1)
	mv a0, a2
	ret
1:
	slli t1, t1, 1
	sw a0, LO_address(s0)
	addi sp, sp, -16
	sw ra, 0(sp)
	memhandler_pre_rv32
	mv a0, a1
	jalr ra, t1, 0
	memhandler_post_rv32
	lw ra, 0(sp)
	addi sp, sp, 16
	ret
	.size \name, . - \name
.endm

pcsx_write_mem jump_handler_write8,  sb, 0, 0x3000
pcsx_write_mem jump_handler_write16, sh, 1, 0x1000
pcsx_write_mem jump_handler_write32, sw, 2, 0

/* Called by a generated block. Preserve every allocator-visible caller-save
 * register and the private software-condition registers when gen_interupt()
 * returns to the same block. The callee-save allocator registers survive by
 * the C ABI. Dispatch/leave paths do not reuse the old register map. */
FUNCTION cc_interrupt
	addi sp, sp, -64
	sw ra, 0(sp)
	sw a0, 4(sp)
	sw a1, 8(sp)
	sw a2, 12(sp)
	sw a3, 16(sp)
	sw a4, 20(sp)
	sw a5, 24(sp)
	sw a6, 28(sp)
	sw a7, 32(sp)
	sw t0, 36(sp)
	sw t1, 40(sp)
	sw t2, 44(sp)
	sw t3, 48(sp)
	sw t4, 52(sp)
	sw t5, 56(sp)
	lw t0, LO_pcaddr(s0)
	sw t0, 60(sp)
	lw t0, LO_last_count(s0)
#ifdef DRC_DBG
	la t1, drc_dbg_irq_rel_before
	sw s1, 0(t1)
	la t1, drc_dbg_irq_last_before
	sw t0, 0(t1)
#endif
	add t0, t0, s1
	sw t0, LO_cycle(s0)
	addi a0, s0, LO_reg_cop0
	call gen_interupt

#ifdef DRC_DBG
	lw t1, LO_cycle(s0)
	la t0, drc_dbg_irq_cycle_after
	sw t1, 0(t0)
	lw t1, LO_next_interupt(s0)
	la t0, drc_dbg_irq_next_after
	sw t1, 0(t0)
#endif
	lw s1, LO_cycle(s0)
	lw t0, LO_next_interupt(s0)
	sw t0, LO_last_count(s0)
	sub s1, s1, t0
	lbu t1, LO_stop(s0)
	bnez t1, .Linterrupt_leave
	lw t1, LO_pcaddr(s0)
	lw t2, 60(sp)
	bne t1, t2, .Linterrupt_dispatch
	lw ra, 0(sp)
	lw a0, 4(sp)
	lw a1, 8(sp)
	lw a2, 12(sp)
	lw a3, 16(sp)
	lw a4, 20(sp)
	lw a5, 24(sp)
	lw a6, 28(sp)
	lw a7, 32(sp)
	lw t0, 36(sp)
	lw t1, 40(sp)
	lw t2, 44(sp)
	lw t3, 48(sp)
	lw t4, 52(sp)
	lw t5, 56(sp)
	addi sp, sp, 64
	ret

.Linterrupt_dispatch:
	mv a0, t1
	lw a1, LO_hash_table_ptr(s0)
	call ndrc_get_addr_ht
	mv t6, a0
	addi sp, sp, 64
	jr t6

.Linterrupt_leave:
	addi sp, sp, 64
	j new_dyna_leave
	.size cc_interrupt, . - cc_interrupt

FUNCTION jump_addrerror_ds
	sw a1, (LO_psxRegs + (34 + 8) * 4)(s0)
	li a1, 1
	j .Lcall_psx_exception
	.size jump_addrerror_ds, . - jump_addrerror_ds

FUNCTION jump_addrerror
	sw a1, (LO_psxRegs + (34 + 8) * 4)(s0)
	li a1, 0
	j .Lcall_psx_exception
	.size jump_addrerror, . - jump_addrerror

FUNCTION jump_overflow_ds
	li a0, (12 << 2)
	li a1, 1
	j .Lcall_psx_exception
	.size jump_overflow_ds, . - jump_overflow_ds

FUNCTION jump_overflow
	li a0, (12 << 2)
	li a1, 0
	j .Lcall_psx_exception
	.size jump_overflow, . - jump_overflow

FUNCTION jump_break_ds
	li a0, (9 << 2)
	li a1, 1
	j .Lcall_psx_exception
	.size jump_break_ds, . - jump_break_ds

FUNCTION jump_break
	li a0, (9 << 2)
	li a1, 0
	j .Lcall_psx_exception
	.size jump_break, . - jump_break

FUNCTION jump_syscall_ds
	li a0, (8 << 2)
	li a1, 2
	j .Lcall_psx_exception
	.size jump_syscall_ds, . - jump_syscall_ds

FUNCTION jump_syscall
	li a0, (8 << 2)
	li a1, 0
	/* fall through */

.Lcall_psx_exception:
	lw t0, LO_last_count(s0)
	sw a2, LO_pcaddr(s0)
	add s1, s1, t0
	sw s1, LO_cycle(s0)
	addi a2, s0, LO_reg_cop0
	call psxException
	/* psxException may recursively enter the recompiler. Reload all runtime
	 * scheduling state before dispatching its selected PC. */
	.size jump_syscall, . - jump_syscall

FUNCTION jump_to_new_pc
	lbu t0, LO_stop(s0)
	lw t1, LO_next_interupt(s0)
	lw s1, LO_cycle(s0)
	lw a0, LO_pcaddr(s0)
	sw t1, LO_last_count(s0)
	sub s1, s1, t1
	bnez t0, .Lnew_pc_leave
	lw a1, LO_hash_table_ptr(s0)
	call ndrc_get_addr_ht
	jr a0
.Lnew_pc_leave:
	j new_dyna_leave
	.size jump_to_new_pc, . - jump_to_new_pc

	.option pop
