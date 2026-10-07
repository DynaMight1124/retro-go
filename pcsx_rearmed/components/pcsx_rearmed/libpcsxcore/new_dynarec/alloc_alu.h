/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2009-2011 Ari64
 * Shared compiler code extracted unchanged from new_dynarec.c.
 */
static void shiftimm_alloc(struct compile_state *st, struct regstat *current,int i)
{
  if(dops[i].opcode2<=0x3) // SLL/SRL/SRA
  {
    if(dops[i].rt1) {
      if(dops[i].rs1&&needed_again(st,dops[i].rs1,i)) alloc_reg(st, current,i,dops[i].rs1);
      else dops[i].use_lt1=!!dops[i].rs1;
      alloc_reg(st, current, i, dops[i].rt1);
      dirty_reg(    current, dops[i].rt1);
      if(is_const(current,dops[i].rs1)) {
        int v=get_const(current,dops[i].rs1);
        if(dops[i].opcode2==0x00) set_const(current,dops[i].rt1,v<<cinfo[i].imm);
        if(dops[i].opcode2==0x02) set_const(current,dops[i].rt1,(u_int)v>>cinfo[i].imm);
        if(dops[i].opcode2==0x03) set_const(current,dops[i].rt1,v>>cinfo[i].imm);
      }
      else clear_const(current,dops[i].rt1);
    }
  }
  else
  {
    clear_const(current,dops[i].rs1);
    clear_const(current,dops[i].rt1);
  }

  if(dops[i].opcode2>=0x38&&dops[i].opcode2<=0x3b) // DSLL/DSRL/DSRA
  {
    assert(0);
  }
  if(dops[i].opcode2==0x3c) // DSLL32
  {
    assert(0);
  }
  if(dops[i].opcode2==0x3e) // DSRL32
  {
    assert(0);
  }
  if(dops[i].opcode2==0x3f) // DSRA32
  {
    assert(0);
  }
}

static void shift_alloc(struct compile_state *st, struct regstat *current, int i)
{
  if(dops[i].rt1) {
      if(dops[i].rs1) alloc_reg(st, current, i, dops[i].rs1);
      if(dops[i].rs2) alloc_reg(st, current, i, dops[i].rs2);
      alloc_reg(st, current, i, dops[i].rt1);
      if(dops[i].rt1==dops[i].rs2) {
        alloc_reg_temp(st, current, i, -1);
        cinfo[i].min_free_regs=1;
      }
    clear_const(current,dops[i].rs1);
    clear_const(current,dops[i].rs2);
    clear_const(current,dops[i].rt1);
    dirty_reg(current,dops[i].rt1);
  }
}

static void alu_alloc(struct compile_state *st, struct regstat *current, int i)
{
  if(dops[i].opcode2>=0x20&&dops[i].opcode2<=0x23) { // ADD/ADDU/SUB/SUBU
    if(dops[i].rt1) {
      if(dops[i].rs1&&dops[i].rs2) {
        alloc_reg(st, current, i, dops[i].rs1);
        alloc_reg(st, current, i, dops[i].rs2);
      }
      else {
        if(dops[i].rs1&&needed_again(st,dops[i].rs1,i)) alloc_reg(st,current,i,dops[i].rs1);
        if(dops[i].rs2&&needed_again(st,dops[i].rs2,i)) alloc_reg(st,current,i,dops[i].rs2);
      }
      alloc_reg(st, current, i, dops[i].rt1);
    }
    if (dops[i].may_except) {
      alloc_cc_optional(current, i); // for exceptions
      alloc_reg_temp(st, current, i, -1);
      cinfo[i].min_free_regs = 1;
    }
  }
  else if(dops[i].opcode2==0x2a||dops[i].opcode2==0x2b) { // SLT/SLTU
    if(dops[i].rt1) {
      alloc_reg(st, current, i, dops[i].rs1);
      alloc_reg(st, current, i, dops[i].rs2);
      alloc_reg(st, current, i, dops[i].rt1);
    }
  }
  else if(dops[i].opcode2>=0x24&&dops[i].opcode2<=0x27) { // AND/OR/XOR/NOR
    if(dops[i].rt1) {
      if(dops[i].rs1&&dops[i].rs2) {
        alloc_reg(st, current, i, dops[i].rs1);
        alloc_reg(st, current, i, dops[i].rs2);
      }
      else
      {
        if(dops[i].rs1&&needed_again(st,dops[i].rs1,i)) alloc_reg(st,current,i,dops[i].rs1);
        if(dops[i].rs2&&needed_again(st,dops[i].rs2,i)) alloc_reg(st,current,i,dops[i].rs2);
      }
      alloc_reg(st, current, i, dops[i].rt1);
    }
  }
  clear_const(current,dops[i].rs1);
  clear_const(current,dops[i].rs2);
  clear_const(current,dops[i].rt1);
  dirty_reg(current,dops[i].rt1);
}

static void imm16_alloc(struct compile_state *st, struct regstat *current, int i)
{
  if(dops[i].rs1&&needed_again(st,dops[i].rs1,i)) alloc_reg(st,current,i,dops[i].rs1);
  else dops[i].use_lt1=!!dops[i].rs1;
  if(dops[i].rt1) alloc_reg(st,current,i,dops[i].rt1);
  if(dops[i].opcode==0x0a||dops[i].opcode==0x0b) { // SLTI/SLTIU
    clear_const(current,dops[i].rs1);
    clear_const(current,dops[i].rt1);
  }
  else if(dops[i].opcode>=0x0c&&dops[i].opcode<=0x0e) { // ANDI/ORI/XORI
    if(is_const(current,dops[i].rs1)) {
      int v=get_const(current,dops[i].rs1);
      if(dops[i].opcode==0x0c) set_const(current,dops[i].rt1,v&cinfo[i].imm);
      if(dops[i].opcode==0x0d) set_const(current,dops[i].rt1,v|cinfo[i].imm);
      if(dops[i].opcode==0x0e) set_const(current,dops[i].rt1,v^cinfo[i].imm);
    }
    else clear_const(current,dops[i].rt1);
  }
  else if(dops[i].opcode==0x08||dops[i].opcode==0x09) { // ADDI/ADDIU
    if(is_const(current,dops[i].rs1)) {
      int v=get_const(current,dops[i].rs1);
      set_const(current,dops[i].rt1,v+cinfo[i].imm);
    }
    else clear_const(current,dops[i].rt1);
    if (dops[i].may_except) {
      alloc_cc_optional(current, i); // for exceptions
      alloc_reg_temp(st, current, i, -1);
      cinfo[i].min_free_regs = 1;
    }
  }
  else {
    set_const(current,dops[i].rt1,cinfo[i].imm<<16); // LUI
  }
  dirty_reg(current,dops[i].rt1);
}
