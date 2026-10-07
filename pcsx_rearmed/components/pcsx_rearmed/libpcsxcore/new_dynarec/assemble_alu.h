/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2009-2011 Ari64
 * Shared arithmetic/shift emission, extracted unchanged from new_dynarec.c.
 * Included after backend emitters and compiler state declarations.
 */
static void alu_assemble(int i, const struct regstat *i_regs, int ccadj_)
{
  if(dops[i].opcode2>=0x20&&dops[i].opcode2<=0x23) { // ADD/ADDU/SUB/SUBU
    int do_oflow = dops[i].may_except; // ADD/SUB with exceptions enabled
    if (dops[i].rt1 || do_oflow) {
      int do_exception_check = 0;
      signed char s1, s2, t, tmp;
      t = get_reg_w(i_regs->regmap, dops[i].rt1);
      tmp = get_reg_temp(i_regs->regmap);
      if (do_oflow)
        assert(tmp >= 0);
      if (t < 0 && do_oflow)
        t = tmp;
      if (t >= 0) {
        s1 = get_reg(i_regs->regmap, dops[i].rs1);
        s2 = get_reg(i_regs->regmap, dops[i].rs2);
        if (dops[i].rs1 && dops[i].rs2) {
          assert(s1>=0);
          assert(s2>=0);
          if (dops[i].opcode2 & 2) {
            if (do_oflow) {
              emit_subs(s1, s2, tmp);
              do_exception_check = 1;
            }
            else
              emit_sub(s1,s2,t);
          }
          else {
            if (do_oflow) {
              emit_adds(s1, s2, tmp);
              do_exception_check = 1;
            }
            else
              emit_add(s1,s2,t);
          }
        }
        else if(dops[i].rs1) {
          if(s1>=0) emit_mov(s1,t);
          else emit_loadreg(dops[i].rs1,t);
        }
        else if(dops[i].rs2) {
          if (s2 < 0) {
            emit_loadreg(dops[i].rs2, t);
            s2 = t;
          }
          if (dops[i].opcode2 & 2) {
            if (do_oflow) {
              emit_negs(s2, tmp);
              do_exception_check = 1;
            }
            else
              emit_neg(s2, t);
          }
          else if (s2 != t)
            emit_mov(s2, t);
        }
        else
          emit_zeroreg(t);
      }
      if (do_exception_check) {
        void *jaddr = out;
        emit_jo(0);
        if (t >= 0 && tmp != t)
          emit_mov(tmp, t);
        add_stub_r(OVERFLOW_STUB, jaddr, out, i, 0, i_regs, ccadj_, 0);
      }
    }
  }
  else if(dops[i].opcode2==0x2a||dops[i].opcode2==0x2b) { // SLT/SLTU
    if(dops[i].rt1) {
      signed char s1l,s2l,t;
      {
        t=get_reg_w(i_regs->regmap, dops[i].rt1);
        //assert(t>=0);
        if(t>=0) {
          s1l=get_reg(i_regs->regmap,dops[i].rs1);
          s2l=get_reg(i_regs->regmap,dops[i].rs2);
          if(dops[i].rs2==0) // rx<r0
          {
            if(dops[i].opcode2==0x2a&&dops[i].rs1!=0) { // SLT
              assert(s1l>=0);
              emit_shrimm(s1l,31,t);
            }
            else // SLTU (unsigned can not be less than zero, 0<0)
              emit_zeroreg(t);
          }
          else if(dops[i].rs1==0) // r0<rx
          {
            assert(s2l>=0);
            if(dops[i].opcode2==0x2a) // SLT
              emit_set_gz32(s2l,t);
            else // SLTU (set if not zero)
              emit_set_nz32(s2l,t);
          }
          else{
            assert(s1l>=0);assert(s2l>=0);
            if(dops[i].opcode2==0x2a) // SLT
              emit_set_if_less32(s1l,s2l,t);
            else // SLTU
              emit_set_if_carry32(s1l,s2l,t);
          }
        }
      }
    }
  }
  else if(dops[i].opcode2>=0x24&&dops[i].opcode2<=0x27) { // AND/OR/XOR/NOR
    if(dops[i].rt1) {
      signed char s1l,s2l,tl;
      tl=get_reg_w(i_regs->regmap, dops[i].rt1);
      {
        if(tl>=0) {
          s1l=get_reg(i_regs->regmap,dops[i].rs1);
          s2l=get_reg(i_regs->regmap,dops[i].rs2);
          if(dops[i].rs1&&dops[i].rs2) {
            assert(s1l>=0);
            assert(s2l>=0);
            if(dops[i].opcode2==0x24) { // AND
              emit_and(s1l,s2l,tl);
            } else
            if(dops[i].opcode2==0x25) { // OR
              emit_or(s1l,s2l,tl);
            } else
            if(dops[i].opcode2==0x26) { // XOR
              emit_xor(s1l,s2l,tl);
            } else
            if(dops[i].opcode2==0x27) { // NOR
              emit_or(s1l,s2l,tl);
              emit_not(tl,tl);
            }
          }
          else
          {
            if(dops[i].opcode2==0x24) { // AND
              emit_zeroreg(tl);
            } else
            if(dops[i].opcode2==0x25||dops[i].opcode2==0x26) { // OR/XOR
              if(dops[i].rs1){
                if(s1l>=0) emit_mov(s1l,tl);
                else emit_loadreg(dops[i].rs1,tl); // CHECK: regmap_entry?
              }
              else
              if(dops[i].rs2){
                if(s2l>=0) emit_mov(s2l,tl);
                else emit_loadreg(dops[i].rs2,tl); // CHECK: regmap_entry?
              }
              else emit_zeroreg(tl);
            } else
            if(dops[i].opcode2==0x27) { // NOR
              if(dops[i].rs1){
                if(s1l>=0) emit_not(s1l,tl);
                else {
                  emit_loadreg(dops[i].rs1,tl);
                  emit_not(tl,tl);
                }
              }
              else
              if(dops[i].rs2){
                if(s2l>=0) emit_not(s2l,tl);
                else {
                  emit_loadreg(dops[i].rs2,tl);
                  emit_not(tl,tl);
                }
              }
              else emit_movimm(-1,tl);
            }
          }
        }
      }
    }
  }
}

static void imm16_assemble(int i, const struct regstat *i_regs, int ccadj_)
{
  if (dops[i].opcode==0x0f) { // LUI
    if(dops[i].rt1) {
      signed char t;
      t=get_reg_w(i_regs->regmap, dops[i].rt1);
      //assert(t>=0);
      if(t>=0) {
        if(!((i_regs->isconst>>t)&1))
          emit_movimm(cinfo[i].imm<<16,t);
      }
    }
  }
  if(dops[i].opcode==0x08||dops[i].opcode==0x09) { // ADDI/ADDIU
    int is_addi = dops[i].may_except;
    if (dops[i].rt1 || is_addi) {
      signed char s, t, tmp;
      t=get_reg_w(i_regs->regmap, dops[i].rt1);
      s=get_reg(i_regs->regmap,dops[i].rs1);
      if(dops[i].rs1) {
        tmp = get_reg_temp(i_regs->regmap);
        if (is_addi) {
          assert(tmp >= 0);
          if (t < 0) t = tmp;
        }
        if(t>=0) {
          if(!((i_regs->isconst>>t)&1)) {
            int sum, do_exception_check = 0;
            if (s < 0) {
              if(i_regs->regmap_entry[t]!=dops[i].rs1) emit_loadreg(dops[i].rs1,t);
              if (is_addi) {
                emit_addimm_and_set_flags3(t, cinfo[i].imm, tmp);
                do_exception_check = 1;
              }
              else
                emit_addimm(t, cinfo[i].imm, t);
            } else {
              if (!((i_regs->wasconst >> s) & 1)) {
                if (is_addi) {
                  emit_addimm_and_set_flags3(s, cinfo[i].imm, tmp);
                  do_exception_check = 1;
                }
                else
                  emit_addimm(s, cinfo[i].imm, t);
              }
              else {
                int oflow = add_overflow(constmap[i][s], cinfo[i].imm, sum);
                if (is_addi && oflow)
                  do_exception_check = 2;
                else
                  emit_movimm(sum, t);
              }
            }
            if (do_exception_check) {
              void *jaddr = out;
              if (do_exception_check == 2)
                emit_jmp(0);
              else {
                emit_jo(0);
                if (tmp != t)
                  emit_mov(tmp, t);
              }
              add_stub_r(OVERFLOW_STUB, jaddr, out, i, 0, i_regs, ccadj_, 0);
            }
          }
        }
      } else {
        if(t>=0) {
          if(!((i_regs->isconst>>t)&1))
            emit_movimm(cinfo[i].imm,t);
        }
      }
    }
  }
  else if(dops[i].opcode==0x0a||dops[i].opcode==0x0b) { // SLTI/SLTIU
    if(dops[i].rt1) {
      //assert(dops[i].rs1!=0); // r0 might be valid, but it's probably a bug
      signed char sl,t;
      t=get_reg_w(i_regs->regmap, dops[i].rt1);
      sl=get_reg(i_regs->regmap,dops[i].rs1);
      //assert(t>=0);
      if(t>=0) {
        if(dops[i].rs1>0) {
            if(dops[i].opcode==0x0a) { // SLTI
              if(sl<0) {
                if(i_regs->regmap_entry[t]!=dops[i].rs1) emit_loadreg(dops[i].rs1,t);
                emit_slti32(t,cinfo[i].imm,t);
              }else{
                emit_slti32(sl,cinfo[i].imm,t);
              }
            }
            else { // SLTIU
              if(sl<0) {
                if(i_regs->regmap_entry[t]!=dops[i].rs1) emit_loadreg(dops[i].rs1,t);
                emit_sltiu32(t,cinfo[i].imm,t);
              }else{
                emit_sltiu32(sl,cinfo[i].imm,t);
              }
            }
        }else{
          // SLTI(U) with r0 is just stupid,
          // nonetheless examples can be found
          if(dops[i].opcode==0x0a) // SLTI
            if(0<cinfo[i].imm) emit_movimm(1,t);
            else emit_zeroreg(t);
          else // SLTIU
          {
            if(cinfo[i].imm) emit_movimm(1,t);
            else emit_zeroreg(t);
          }
        }
      }
    }
  }
  else if(dops[i].opcode>=0x0c&&dops[i].opcode<=0x0e) { // ANDI/ORI/XORI
    if(dops[i].rt1) {
      signed char sl,tl;
      tl=get_reg_w(i_regs->regmap, dops[i].rt1);
      sl=get_reg(i_regs->regmap,dops[i].rs1);
      if(tl>=0 && !((i_regs->isconst>>tl)&1)) {
        if(dops[i].opcode==0x0c) //ANDI
        {
          if(dops[i].rs1) {
            if(sl<0) {
              if(i_regs->regmap_entry[tl]!=dops[i].rs1) emit_loadreg(dops[i].rs1,tl);
              emit_andimm(tl,cinfo[i].imm,tl);
            }else{
              if(!((i_regs->wasconst>>sl)&1))
                emit_andimm(sl,cinfo[i].imm,tl);
              else
                emit_movimm(constmap[i][sl]&cinfo[i].imm,tl);
            }
          }
          else
            emit_zeroreg(tl);
        }
        else
        {
          if(dops[i].rs1) {
            if(sl<0) {
              if(i_regs->regmap_entry[tl]!=dops[i].rs1) emit_loadreg(dops[i].rs1,tl);
            }
            if(dops[i].opcode==0x0d) { // ORI
              if(sl<0) {
                emit_orimm(tl,cinfo[i].imm,tl);
              }else{
                if(!((i_regs->wasconst>>sl)&1))
                  emit_orimm(sl,cinfo[i].imm,tl);
                else
                  emit_movimm(constmap[i][sl]|cinfo[i].imm,tl);
              }
            }
            if(dops[i].opcode==0x0e) { // XORI
              if(sl<0) {
                emit_xorimm(tl,cinfo[i].imm,tl);
              }else{
                if(!((i_regs->wasconst>>sl)&1))
                  emit_xorimm(sl,cinfo[i].imm,tl);
                else
                  emit_movimm(constmap[i][sl]^cinfo[i].imm,tl);
              }
            }
          }
          else {
            emit_movimm(cinfo[i].imm,tl);
          }
        }
      }
    }
  }
}

static void shiftimm_assemble(int i, const struct regstat *i_regs)
{
  if(dops[i].opcode2<=0x3) // SLL/SRL/SRA
  {
    if(dops[i].rt1) {
      signed char s,t;
      t=get_reg_w(i_regs->regmap, dops[i].rt1);
      s=get_reg(i_regs->regmap,dops[i].rs1);
      //assert(t>=0);
      if(t>=0&&!((i_regs->isconst>>t)&1)){
        if(dops[i].rs1==0)
        {
          emit_zeroreg(t);
        }
        else
        {
          if(s<0&&i_regs->regmap_entry[t]!=dops[i].rs1) emit_loadreg(dops[i].rs1,t);
          if(cinfo[i].imm) {
            if(dops[i].opcode2==0) // SLL
            {
              emit_shlimm(s<0?t:s,cinfo[i].imm,t);
            }
            if(dops[i].opcode2==2) // SRL
            {
              emit_shrimm(s<0?t:s,cinfo[i].imm,t);
            }
            if(dops[i].opcode2==3) // SRA
            {
              emit_sarimm(s<0?t:s,cinfo[i].imm,t);
            }
          }else{
            // Shift by zero
            if(s>=0 && s!=t) emit_mov(s,t);
          }
        }
      }
      //emit_storereg(dops[i].rt1,t); //DEBUG
    }
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

#ifndef shift_assemble
static void shift_assemble(int i, const struct regstat *i_regs)
{
  signed char s,t,shift;
  if (dops[i].rt1 == 0)
    return;
  assert(dops[i].opcode2<=0x07); // SLLV/SRLV/SRAV
  t = get_reg(i_regs->regmap, dops[i].rt1);
  s = get_reg(i_regs->regmap, dops[i].rs1);
  shift = get_reg(i_regs->regmap, dops[i].rs2);
  if (t < 0)
    return;

  if(dops[i].rs1==0)
    emit_zeroreg(t);
  else if(dops[i].rs2==0) {
    assert(s>=0);
    if(s!=t) emit_mov(s,t);
  }
  else {
    host_tempreg_acquire();
    emit_andimm(shift,31,HOST_TEMPREG);
    switch(dops[i].opcode2) {
    case 4: // SLLV
      emit_shl(s,HOST_TEMPREG,t);
      break;
    case 6: // SRLV
      emit_shr(s,HOST_TEMPREG,t);
      break;
    case 7: // SRAV
      emit_sar(s,HOST_TEMPREG,t);
      break;
    default:
      assert(0);
    }
    host_tempreg_release();
  }
}

#endif
