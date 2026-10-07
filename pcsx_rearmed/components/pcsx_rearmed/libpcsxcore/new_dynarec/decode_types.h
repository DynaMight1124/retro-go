/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2009-2011 Ari64
 * Shared compiler code extracted unchanged from new_dynarec.c.
 */
struct decoded_insn
{
  u_char itype;
  u_char opcode;   // bits 31-26
  u_char opcode2;  // (depends on opcode)
  u_char rs1;
  u_char rs2;
  u_char rt1;
  u_char rt2;
  u_char use_lt1:1;
  u_char bt:1;
  u_char ooo:1;
  u_char is_ds:1;
  u_char is_jump:1;
  u_char is_ujump:1;
  u_char is_load:1;
  u_char is_store:1;
  u_char is_delay_load:1; // is_load + MFC/CFC
  u_char is_exception:1;  // unconditional, also interp. fallback
  u_char may_except:1;    // might generate an exception
  u_char ls_type:2;       // load/store type (ls_width_type LS_*)
};

enum ls_width_type {
  LS_8 = 0, LS_16, LS_32, LS_LR
};

struct compile_info
{
  int imm;
  u_int ba;
  int ccadj;
  signed char min_free_regs;
  signed char addr;
  signed char reserved[2];
};
