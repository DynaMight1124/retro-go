/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2009-2011 Ari64
 * Shared compiler code extracted unchanged from new_dynarec.c.
 */
static void disassemble_one(struct compile_state *st, int i, u_int src)
{
    unsigned int type, op, op2, op3;
    enum ls_width_type ls_type = LS_32;
    memset(&dops[i], 0, sizeof(dops[i]));
    memset(&cinfo[i], 0, sizeof(cinfo[i]));
    cinfo[i].ba = -1;
    cinfo[i].addr = -1;
    dops[i].opcode = op = src >> 26;
    op2 = 0;
    type = INTCALL;
    set_mnemonic(i, "???");
    switch(op)
    {
      case 0x00: set_mnemonic(i, "special");
        op2 = src & 0x3f;
        switch(op2)
        {
          case 0x00: set_mnemonic(i, "SLL"); type=SHIFTIMM; break;
          case 0x02: set_mnemonic(i, "SRL"); type=SHIFTIMM; break;
          case 0x03: set_mnemonic(i, "SRA"); type=SHIFTIMM; break;
          case 0x04: set_mnemonic(i, "SLLV"); type=SHIFT; break;
          case 0x06: set_mnemonic(i, "SRLV"); type=SHIFT; break;
          case 0x07: set_mnemonic(i, "SRAV"); type=SHIFT; break;
          case 0x08: set_mnemonic(i, "JR"); type=RJUMP; break;
          case 0x09: set_mnemonic(i, "JALR"); type=RJUMP; break;
          case 0x0C: set_mnemonic(i, "SYSCALL"); type=SYSCALL; break;
          case 0x0D: set_mnemonic(i, "BREAK"); type=SYSCALL; break;
          case 0x10: set_mnemonic(i, "MFHI"); type=MOV; break;
          case 0x11: set_mnemonic(i, "MTHI"); type=MOV; break;
          case 0x12: set_mnemonic(i, "MFLO"); type=MOV; break;
          case 0x13: set_mnemonic(i, "MTLO"); type=MOV; break;
          case 0x18: set_mnemonic(i, "MULT"); type=MULTDIV; break;
          case 0x19: set_mnemonic(i, "MULTU"); type=MULTDIV; break;
          case 0x1A: set_mnemonic(i, "DIV"); type=MULTDIV; break;
          case 0x1B: set_mnemonic(i, "DIVU"); type=MULTDIV; break;
          case 0x20: set_mnemonic(i, "ADD"); type=ALU; break;
          case 0x21: set_mnemonic(i, "ADDU"); type=ALU; break;
          case 0x22: set_mnemonic(i, "SUB"); type=ALU; break;
          case 0x23: set_mnemonic(i, "SUBU"); type=ALU; break;
          case 0x24: set_mnemonic(i, "AND"); type=ALU; break;
          case 0x25: set_mnemonic(i, "OR"); type=ALU; break;
          case 0x26: set_mnemonic(i, "XOR"); type=ALU; break;
          case 0x27: set_mnemonic(i, "NOR"); type=ALU; break;
          case 0x2A: set_mnemonic(i, "SLT"); type=ALU; break;
          case 0x2B: set_mnemonic(i, "SLTU"); type=ALU; break;
        }
        break;
      case 0x01: set_mnemonic(i, "regimm");
        type = SJUMP;
        op2 = (src >> 16) & 0x1f;
        switch(op2)
        {
          case 0x10: set_mnemonic(i, "BLTZAL"); break;
          case 0x11: set_mnemonic(i, "BGEZAL"); break;
          default:
            if (op2 & 1)
              set_mnemonic(i, "BGEZ");
            else
              set_mnemonic(i, "BLTZ");
        }
        break;
      case 0x02: set_mnemonic(i, "J"); type=UJUMP; break;
      case 0x03: set_mnemonic(i, "JAL"); type=UJUMP; break;
      case 0x04: set_mnemonic(i, "BEQ"); type=CJUMP; break;
      case 0x05: set_mnemonic(i, "BNE"); type=CJUMP; break;
      case 0x06: set_mnemonic(i, "BLEZ"); type=CJUMP; break;
      case 0x07: set_mnemonic(i, "BGTZ"); type=CJUMP; break;
      case 0x08: set_mnemonic(i, "ADDI"); type=IMM16; break;
      case 0x09: set_mnemonic(i, "ADDIU"); type=IMM16; break;
      case 0x0A: set_mnemonic(i, "SLTI"); type=IMM16; break;
      case 0x0B: set_mnemonic(i, "SLTIU"); type=IMM16; break;
      case 0x0C: set_mnemonic(i, "ANDI"); type=IMM16; break;
      case 0x0D: set_mnemonic(i, "ORI"); type=IMM16; break;
      case 0x0E: set_mnemonic(i, "XORI"); type=IMM16; break;
      case 0x0F: set_mnemonic(i, "LUI"); type=IMM16; break;
      case 0x10: set_mnemonic(i, "COP0");
        op2 = (src >> 21) & 0x1f;
	if (op2 & 0x10) {
          op3 = src & 0x1f;
          switch (op3)
          {
            case 0x01: case 0x02: case 0x06: case 0x08: type = INTCALL; break;
            case 0x10: set_mnemonic(i, "RFE"); type=RFE; break;
            default:   type = OTHER; break;
          }
          break;
        }
        switch(op2)
        {
          u32 rd;
          case 0x00:
            set_mnemonic(i, "MFC0");
            rd = (src >> 11) & 0x1F;
            if (!(0x00000417u & (1u << rd)))
              type = COP0;
            break;
          case 0x04: set_mnemonic(i, "MTC0"); type=COP0; break;
          case 0x02:
          case 0x06: type = INTCALL; break;
          default:   type = OTHER; break;
        }
        break;
      case 0x11: set_mnemonic(i, "COP1");
        op2 = (src >> 21) & 0x1f;
        break;
      case 0x12: set_mnemonic(i, "COP2");
        op2 = (src >> 21) & 0x1f;
        if (op2 & 0x10) {
          type = OTHER;
          if (gte_handlers[src & 0x3f] != NULL) {
#ifdef DISASM
            if (gte_regnames[src & 0x3f] != NULL)
              strcpy(insn[i], gte_regnames[src & 0x3f]);
            else
              snprintf(insn[i], sizeof(insn[i]), "COP2 %x", src & 0x3f);
#endif
            type = C2OP;
          }
        }
        else switch(op2)
        {
          case 0x00: set_mnemonic(i, "MFC2"); type=COP2; break;
          case 0x02: set_mnemonic(i, "CFC2"); type=COP2; break;
          case 0x04: set_mnemonic(i, "MTC2"); type=COP2; break;
          case 0x06: set_mnemonic(i, "CTC2"); type=COP2; break;
        }
        break;
      case 0x13: set_mnemonic(i, "COP3");
        op2 = (src >> 21) & 0x1f;
        break;
      case 0x20: set_mnemonic(i, "LB"); type=LOAD; ls_type = LS_8; break;
      case 0x21: set_mnemonic(i, "LH"); type=LOAD; ls_type = LS_16; break;
      case 0x22: set_mnemonic(i, "LWL"); type=LOADLR; ls_type = LS_LR; break;
      case 0x23: set_mnemonic(i, "LW"); type=LOAD; ls_type = LS_32; break;
      case 0x24: set_mnemonic(i, "LBU"); type=LOAD; ls_type = LS_8; break;
      case 0x25: set_mnemonic(i, "LHU"); type=LOAD; ls_type = LS_16; break;
      case 0x26: set_mnemonic(i, "LWR"); type=LOADLR; ls_type = LS_LR; break;
      case 0x28: set_mnemonic(i, "SB"); type=STORE; ls_type = LS_8; break;
      case 0x29: set_mnemonic(i, "SH"); type=STORE; ls_type = LS_16; break;
      case 0x2A: set_mnemonic(i, "SWL"); type=STORELR; ls_type = LS_LR; break;
      case 0x2B: set_mnemonic(i, "SW"); type=STORE; ls_type = LS_32; break;
      case 0x2E: set_mnemonic(i, "SWR"); type=STORELR; ls_type = LS_LR; break;
      case 0x32: set_mnemonic(i, "LWC2"); type=C2LS; ls_type = LS_32; break;
      case 0x3A: set_mnemonic(i, "SWC2"); type=C2LS; ls_type = LS_32; break;
      case 0x3B:
        if (Config.HLE && (src & 0x03ffffff) < ARRAY_SIZE(psxHLEt)) {
          set_mnemonic(i, "HLECALL");
          type = HLECALL;
        }
        break;
      default:
        break;
    }
    if (type == INTCALL)
      SysPrintf_lim("NI %08x @%08x (%08x)\n", src, st->start + i*4, st->start);
    dops[i].itype = type;
    dops[i].opcode2 = op2;
    dops[i].ls_type = ls_type;
    /* Get registers/immediates */
    dops[i].use_lt1=0;
    gte_rs[i]=gte_rt[i]=0;
    dops[i].rs1 = 0;
    dops[i].rs2 = 0;
    dops[i].rt1 = 0;
    dops[i].rt2 = 0;
    switch(type) {
      case LOAD:
        dops[i].rs1 = (src >> 21) & 0x1f;
        dops[i].rt1 = (src >> 16) & 0x1f;
        cinfo[i].imm = (short)src;
        break;
      case STORE:
      case STORELR:
        dops[i].rs1 = (src >> 21) & 0x1f;
        dops[i].rs2 = (src >> 16) & 0x1f;
        cinfo[i].imm = (short)src;
        break;
      case LOADLR:
        // LWL/LWR only load part of the register,
        // therefore the target register must be treated as a source too
        dops[i].rs1 = (src >> 21) & 0x1f;
        dops[i].rs2 = (src >> 16) & 0x1f;
        dops[i].rt1 = (src >> 16) & 0x1f;
        cinfo[i].imm = (short)src;
        break;
      case IMM16:
        if (op==0x0f) dops[i].rs1=0; // LUI instruction has no source register
        else dops[i].rs1 = (src >> 21) & 0x1f;
        dops[i].rs2 = 0;
        dops[i].rt1 = (src >> 16) & 0x1f;
        if(op>=0x0c&&op<=0x0e) { // ANDI/ORI/XORI
          cinfo[i].imm = (unsigned short)src;
        }else{
          cinfo[i].imm = (short)src;
        }
        break;
      case UJUMP:
        // The JAL instruction writes to r31.
        if (op&1) {
          dops[i].rt1=31;
        }
        dops[i].rs2=CCREG;
        break;
      case RJUMP:
        dops[i].rs1 = (src >> 21) & 0x1f;
        // The JALR instruction writes to rd.
        if (op2&1) {
          dops[i].rt1 = (src >> 11) & 0x1f;
        }
        dops[i].rs2=CCREG;
        break;
      case CJUMP:
        dops[i].rs1 = (src >> 21) & 0x1f;
        dops[i].rs2 = (src >> 16) & 0x1f;
        if(op&2) { // BGTZ/BLEZ
          dops[i].rs2=0;
        }
        break;
      case SJUMP:
        dops[i].rs1 = (src >> 21) & 0x1f;
        dops[i].rs2 = CCREG;
        if (op2 == 0x10 || op2 == 0x11) { // BxxAL
          dops[i].rt1 = 31;
          // NOTE: If the branch is not taken, r31 is still overwritten
        }
        break;
      case ALU:
        dops[i].rs1=(src>>21)&0x1f; // source
        dops[i].rs2=(src>>16)&0x1f; // subtract amount
        dops[i].rt1=(src>>11)&0x1f; // destination
        break;
      case MULTDIV:
        dops[i].rs1=(src>>21)&0x1f; // source
        dops[i].rs2=(src>>16)&0x1f; // divisor
        dops[i].rt1=HIREG;
        dops[i].rt2=LOREG;
        break;
      case MOV:
        if(op2==0x10) dops[i].rs1=HIREG; // MFHI
        if(op2==0x11) dops[i].rt1=HIREG; // MTHI
        if(op2==0x12) dops[i].rs1=LOREG; // MFLO
        if(op2==0x13) dops[i].rt1=LOREG; // MTLO
        if((op2&0x1d)==0x10) dops[i].rt1=(src>>11)&0x1f; // MFxx
        if((op2&0x1d)==0x11) dops[i].rs1=(src>>21)&0x1f; // MTxx
        break;
      case SHIFT:
        dops[i].rs1=(src>>16)&0x1f; // target of shift
        dops[i].rs2=(src>>21)&0x1f; // shift amount
        dops[i].rt1=(src>>11)&0x1f; // destination
        break;
      case SHIFTIMM:
        dops[i].rs1=(src>>16)&0x1f;
        dops[i].rs2=0;
        dops[i].rt1=(src>>11)&0x1f;
        cinfo[i].imm=(src>>6)&0x1f;
        break;
      case COP0:
        if(op2==0) dops[i].rt1=(src>>16)&0x1F; // MFC0
        if(op2==4) dops[i].rs1=(src>>16)&0x1F; // MTC0
        if(op2==4&&((src>>11)&0x1e)==12) dops[i].rs2=CCREG;
        break;
      case COP2:
        if(op2<3) dops[i].rt1=(src>>16)&0x1F; // MFC2/CFC2
        if(op2>3) dops[i].rs1=(src>>16)&0x1F; // MTC2/CTC2
        int gr=(src>>11)&0x1F;
        switch(op2)
        {
          case 0x00: gte_rs[i]=1ll<<gr; break; // MFC2
          case 0x04: gte_rt[i]=1ll<<gr; break; // MTC2
          case 0x02: gte_rs[i]=1ll<<(gr+32); break; // CFC2
          case 0x06: gte_rt[i]=1ll<<(gr+32); break; // CTC2
        }
        break;
      case C2LS:
        dops[i].rs1=(src>>21)&0x1F;
        cinfo[i].imm=(short)src;
        if(op==0x32) gte_rt[i]=1ll<<((src>>16)&0x1F); // LWC2
        else gte_rs[i]=1ll<<((src>>16)&0x1F); // SWC2
        break;
      case C2OP:
        gte_rs[i]=gte_reg_reads[src&0x3f];
        gte_rt[i]=gte_reg_writes[src&0x3f];
        gte_rt[i]|=1ll<<63; // every op changes flags
        if((src&0x3f)==GTE_MVMVA) {
          int v = (src >> 15) & 3;
          gte_rs[i]&=~0xe3fll;
          if(v==3) gte_rs[i]|=0xe00ll;
          else gte_rs[i]|=3ll<<(v*2);
        }
        break;
      case SYSCALL:
      case HLECALL:
      case INTCALL:
        dops[i].rs1=CCREG;
        break;
      default:
        break;
    }
}
