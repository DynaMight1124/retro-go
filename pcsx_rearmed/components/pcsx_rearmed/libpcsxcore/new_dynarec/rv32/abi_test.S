/* SPDX-License-Identifier: GPL-2.0-or-later
 * Test-only ABI adversaries. No PSX state is touched.
 */
.text
.balign 4
.global ndrc_rv32_test_clobber
.type ndrc_rv32_test_clobber,@function
ndrc_rv32_test_clobber:
  li a0, 0x600
  li a1, 0x601
  li a2, 0x602
  li a3, 0x603
  li a4, 0x604
  li a5, 0x605
  li a6, 0x606
  li a7, 0x607
  li t0, 0x608
  li t1, 0x609
  li t2, 0x60a
  li t3, 0x60b
  li t4, 0x60c
  li t5, 0x60d
  andi t6, sp, 15
  or a0, a0, t6 /* result differs if helper entered with misaligned sp */
  li t6, 0x60e
  ret
.size ndrc_rv32_test_clobber, .-ndrc_rv32_test_clobber

/* Call enter after seeding ALL callee-saved registers. C-only tests can miss
 * register corruption when the compiler happens not to keep values in them.
 * Return a failure bitmask; preserve our own caller's registers either way.
 */
.balign 4
.global ndrc_rv32_test_enter
.type ndrc_rv32_test_enter,@function
ndrc_rv32_test_enter:
  addi sp, sp, -80
  sw s0, 0(sp)
  sw s1, 4(sp)
  sw s2, 8(sp)
  sw s3, 12(sp)
  sw s4, 16(sp)
  sw s5, 20(sp)
  sw s6, 24(sp)
  sw s7, 28(sp)
  sw s8, 32(sp)
  sw s9, 36(sp)
  sw s10, 40(sp)
  sw s11, 44(sp)
  sw gp, 48(sp)
  sw tp, 52(sp)
  sw ra, 76(sp)
  li s0, 0x100
  li s1, 0x101
  li s2, 0x102
  li s3, 0x103
  li s4, 0x104
  li s5, 0x105
  li s6, 0x106
  li s7, 0x107
  li s8, 0x108
  li s9, 0x109
  li s10, 0x10a
  li s11, 0x10b
  call ndrc_rv32_enter
  /* Generated test code returns 0x321. */
  xori a0, a0, 0x321
  snez a0, a0
.macro check_saved reg, value, bit
  li t0, \value
  xor t0, t0, \reg
  snez t0, t0
  slli t0, t0, \bit
  or a0, a0, t0
.endm
  check_saved s0, 0x100, 1
  check_saved s1, 0x101, 2
  check_saved s2, 0x102, 3
  check_saved s3, 0x103, 4
  check_saved s4, 0x104, 5
  check_saved s5, 0x105, 6
  check_saved s6, 0x106, 7
  check_saved s7, 0x107, 8
  check_saved s8, 0x108, 9
  check_saved s9, 0x109, 10
  check_saved s10, 0x10a, 11
  check_saved s11, 0x10b, 12
  lw t0, 48(sp)
  xor t0, t0, gp
  snez t0, t0
  slli t0, t0, 13
  or a0, a0, t0
  lw t0, 52(sp)
  xor t0, t0, tp
  snez t0, t0
  slli t0, t0, 14
  or a0, a0, t0
  lw s0, 0(sp)
  lw s1, 4(sp)
  lw s2, 8(sp)
  lw s3, 12(sp)
  lw s4, 16(sp)
  lw s5, 20(sp)
  lw s6, 24(sp)
  lw s7, 28(sp)
  lw s8, 32(sp)
  lw s9, 36(sp)
  lw s10, 40(sp)
  lw s11, 44(sp)
  lw gp, 48(sp)
  lw tp, 52(sp)
  lw ra, 76(sp)
  addi sp, sp, 80
  ret
.size ndrc_rv32_test_enter, .-ndrc_rv32_test_enter
