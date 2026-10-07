/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "abi.h"
.text
.balign 4
.global ndrc_rv32_enter
.type ndrc_rv32_enter,@function
ndrc_rv32_enter:
  addi sp, sp, -NDRC_RV32_ENTRY_FRAME
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
  sw a2, 48(sp)
  sw ra, 60(sp)
  mv s0, a1
  lw s1, 0(a2)
  /* No live return address inside generated code; blocks chain with tail jumps.
   * leave recovers the C return address from this entry frame. */
  jr a0
.size ndrc_rv32_enter, .-ndrc_rv32_enter

.balign 4
.global ndrc_rv32_leave
.type ndrc_rv32_leave,@function
ndrc_rv32_leave:
  lw t6, 48(sp)
  sw s1, 0(t6)
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
  lw ra, 60(sp)
  addi sp, sp, NDRC_RV32_ENTRY_FRAME
  ret
.size ndrc_rv32_leave, .-ndrc_rv32_leave
