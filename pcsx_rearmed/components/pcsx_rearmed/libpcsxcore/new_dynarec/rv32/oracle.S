/* SPDX-License-Identifier: GPL-2.0-or-later
 * GNU assembler reference for selftest.c: this data is never executed.
 */
.section .rodata.ndrc_rv32_oracle,"a",@progbits
.balign 4
.option push
.option norvc
.option norelax
.global ndrc_rv32_oracle
.global ndrc_rv32_oracle_end
ndrc_rv32_oracle:
  add a0, a1, a2
  sll a0, a1, a2
  slt a0, a1, a2
  sltu a0, a1, a2
  xor a0, a1, a2
  srl a0, a1, a2
  or a0, a1, a2
  and a0, a1, a2
  mul a0, a1, a2
  mulh a0, a1, a2
  mulhsu a0, a1, a2
  mulhu a0, a1, a2
  div a0, a1, a2
  divu a0, a1, a2
  rem a0, a1, a2
  remu a0, a1, a2
  sub a0, a1, a2
  sra a0, a1, a2
  addi a0, a1, -2048
  addi a0, a1, 2047
  slti a0, a1, -1
  sltiu a0, a1, -1
  xori a0, a1, -1
  ori a0, a1, 42
  andi a0, a1, 255
  slli a0, a1, 31
  srli a0, a1, 0
  srai a0, a1, 31
  lui t6, 0x80000
  auipc t5, 0xfffff
  lb a0, -2048(a1)
  lh a0, -2(a1)
  lw a0, 2044(a1)
  lbu a0, 2047(a1)
  lhu a0, 2046(a1)
  sb a0, -2048(a1)
  sh a0, -2(a1)
  sw a0, 2044(a1)
  beq a0, a1, . - 4096
  bne a0, a1, . + 4092
  blt a0, a1, . - 4
  bge a0, a1, . + 4
  bltu a0, a1, . - 2048
  bgeu a0, a1, . + 2048
  jal ra, . - 1048576
  jal zero, . + 1048572
  jalr ra, -2048(t6)
  jalr zero, 0(ra)
  lui t6, 0
  jalr ra, -2048(t6)
ndrc_rv32_oracle_end:
.option pop
