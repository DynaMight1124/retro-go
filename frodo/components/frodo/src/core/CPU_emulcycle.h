/*
 *  CPU_emulcycle.h - SC 6510/6502 emulation core (body of
 *                    EmulateCycle() function, the same for
 *                    both 6510 and 6502)
 *
 *  Frodo Copyright (C) Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


/*
 *  Stack macros
 */

// Pop processor flags from the stack
#define pop_flags() \
	read_to(sp | 0x100, data); \
	n_flag = data; \
	v_flag = data & 0x40; \
	d_flag = data & 0x08; \
	i_flag = data & 0x04; \
	z_flag = !(data & 0x02); \
	c_flag = data & 0x01;

// Push processor flags onto the stack
#define push_flags(b_flag) \
	data = 0x20 | (n_flag & 0x80); \
	CHECK_SO; \
	if (v_flag) data |= 0x40; \
	if (b_flag) data |= 0x10; \
	if (d_flag) data |= 0x08; \
	if (i_flag) data |= 0x04; \
	if (!z_flag) data |= 0x02; \
	if (c_flag) data |= 0x01; \
	write_byte(sp-- | 0x100, data);


/*
 *  Other macros
 */

// Branch (cycle 1)
#define Branch(flag) \
		read_to(pc++, data);  \
		check_interrupts(); \
		if (flag) { \
			ar = pc + (int8_t)data; \
			if ((ar ^ pc) & 0xff00) { \
				if (data & 0x80) { \
					state = O_BRANCH_BP; \
				} else { \
					state = O_BRANCH_FP; \
				} \
			} else { \
				state = O_BRANCH_NP; \
			} \
		} else { \
			state = O_FETCH; \
		} \
		break;

// Set N and Z flags according to byte
#define set_nz(x) (z_flag = n_flag = (x))

// Address fetch of RMW instruction done, now read and write operand
#define DoRMW state = RMW_DO_IT; break;

// Operand fetch done, now execute opcode
#define Execute state = OpTab[op]; break;

// Last cycle of opcode, check for pending interrupts
#define Last check_interrupts(); state = O_FETCH; break;


/*
 *  EmulCycle() function
 */

	switch (state) {


		// Opcode fetch
		case O_FETCH:
			if (RESET_PENDING) {
				Reset();
				break;
			}
			read_to(pc, op);
			if (nmi_pending) {
				state = O_NMI;
			} else if (irq_pending) {
				state = O_IRQ;
			} else {
				pc++;
				state = ModeTab[op];
			}
			break;


		// IRQ
		case O_IRQ:
			read_idle(pc);
			state = 0x000a;
			break;
		case 0x000a:
			write_byte(sp-- | 0x100, pc >> 8);
			state = 0x000b;
			break;
		case 0x000b:
			write_byte(sp-- | 0x100, pc);
			if (nmi_triggered) {	// Recognize NMI
				nmi_pending = true;
				nmi_triggered = false;
			}
			state = 0x000c;
			break;
		case 0x000c:
			irq_pending = false;
			push_flags(false);
			i_flag = true;
			if (nmi_pending) {		// IRQ interrupted by NMI?
				nmi_pending = false;
				state = 0x0015;
			} else {
				state = 0x000d;
			}
			break;
		case 0x000d:
			read_to(0xfffe, pc);
			state = 0x000e;
			break;
		case 0x000e:
			read_to(0xffff, data);
			pc |= data << 8;
			state = O_FETCH;
			break;


		// NMI
		case O_NMI:
			read_idle(pc);
			state = 0x0012;
			break;
		case 0x0012:
			write_byte(sp-- | 0x100, pc >> 8);
			state = 0x0013;
			break;
		case 0x0013:
			write_byte(sp-- | 0x100, pc);
			state = 0x0014;
			break;
		case 0x0014:
			irq_pending = nmi_pending = false;
			push_flags(false);
			i_flag = true;
			state = 0x0015;
			break;
		case 0x0015:
			read_to(0xfffa, pc);
			state = 0x0016;
			break;
		case 0x0016:
			read_to(0xfffb, data);
			pc |= data << 8;
			state = O_FETCH;
			break;


		// BRK
		case O_BRK:
			read_idle(pc++);
			state = O_BRK1;
			break;
		case O_BRK1:
			write_byte(sp-- | 0x100, pc >> 8);
			state = O_BRK2;
			break;
		case O_BRK2:
			write_byte(sp-- | 0x100, pc);
			if (nmi_triggered) {	// Recognize NMI
				nmi_pending = true;
				nmi_triggered = false;
			}
			state = O_BRK3;
			break;
		case O_BRK3:
			irq_pending = false;
			push_flags(true);
			i_flag = true;
			if (nmi_pending) {		// BRK interrupted by NMI?
				nmi_pending = false;
				state = 0x0015;
			} else {
				state = O_BRK4;
			}
			break;
		case O_BRK4:
			read_to(0xfffe, pc);
			state = O_BRK5;
			break;
		case O_BRK5:
			read_to(0xffff, data);
			pc |= data << 8;
			state = O_FETCH;
			break;


		// Addressing modes: Fetch effective address, no extra cycles (-> ar)
		case A_ZERO:
			read_to(pc++, ar);
			Execute;

		case A_ZEROX:
			read_to(pc++, ar);
			state = A_ZEROX1;
			break;
		case A_ZEROX1:
			read_idle(ar);
			ar = (ar + x) & 0xff;
			Execute;

		case A_ZEROY:
			read_to(pc++, ar);
			state = A_ZEROY1;
			break;
		case A_ZEROY1:
			read_idle(ar);
			ar = (ar + y) & 0xff;
			Execute;

		case A_ABS:
			read_to(pc++, ar);
			state = A_ABS1;
			break;
		case A_ABS1:
			read_to(pc++, data);
			ar = ar | (data << 8);
			Execute;

		case A_ABSX:
			read_to(pc++, ar);
			state = A_ABSX1;
			break;
		case A_ABSX1:
			read_to(pc++, ar2);	// Note: Some undocumented opcodes rely on the value of ar2
			if (ar + x < 0x100) {
				state = A_ABSX2;
			} else {
				state = A_ABSX3;
			}
			ar = ((ar + x) & 0xff) | (ar2 << 8);
			break;
		case A_ABSX2:	// No page crossed
#ifndef IS_CPU_1541
			if (BALow) {
				ar2 = 0xfe;		// SHA/SHS/SHX/SHY during DMA
			}
#endif
			read_idle(ar);
			Execute;
		case A_ABSX3:	// Page crossed
#ifndef IS_CPU_1541
			if (BALow) {
				ar2 = 0xfe;		// SHA/SHS/SHX/SHY during DMA
			}
#endif
			read_idle(ar);
			ar += 0x100;
			Execute;

		case A_ABSY:
			read_to(pc++, ar);
			state = A_ABSY1;
			break;
		case A_ABSY1:
			read_to(pc++, ar2);	// Note: Some undocumented opcodes rely on the value of ar2
			if (ar + y < 0x100) {
				state = A_ABSY2;
			} else {
				state = A_ABSY3;
			}
			ar = ((ar + y) & 0xff) | (ar2 << 8);
			break;
		case A_ABSY2:	// No page crossed
#ifndef IS_CPU_1541
			if (BALow) {
				ar2 = 0xfe;		// SHA/SHS/SHX/SHY during DMA
			}
#endif
			read_idle(ar);
			Execute;
		case A_ABSY3:	// Page crossed
#ifndef IS_CPU_1541
			if (BALow) {
				ar2 = 0xfe;		// SHA/SHS/SHX/SHY during DMA
			}
#endif
			read_idle(ar);
			ar += 0x100;
			Execute;

		case A_INDX:
			read_to(pc++, ar2);
			state = A_INDX1;
			break;
		case A_INDX1:
			read_idle(ar2);
			ar2 = (ar2 + x) & 0xff;
			state = A_INDX2;
			break;
		case A_INDX2:
			read_to(ar2, ar);
			state = A_INDX3;
			break;
		case A_INDX3:
			read_to((ar2 + 1) & 0xff, data);
			ar = ar | (data << 8);
			Execute;

		case A_INDY:
			read_to(pc++, ar2);
			state = A_INDY1;
			break;
		case A_INDY1:
			read_to(ar2, ar);
			state = A_INDY2;
			break;
		case A_INDY2:
			read_to((ar2 + 1) & 0xff, ar2);	// Note: Some undocumented opcodes rely on the value of ar2
			if (ar + y < 0x100) {
				state = A_INDY3;
			} else {
				state = A_INDY4;
			}
			ar = ((ar + y) & 0xff) | (ar2 << 8);
			break;
		case A_INDY3:	// No page crossed
#ifndef IS_CPU_1541
			if (BALow) {
				ar2 = 0xfe;		// SHA during DMA
			}
#endif
			read_idle(ar);
			Execute;
		case A_INDY4:	// Page crossed
#ifndef IS_CPU_1541
			if (BALow) {
				ar2 = 0xfe;		// SHA during DMA
			}
#endif
			read_idle(ar);
			ar += 0x100;
			Execute;


		// Addressing modes: Fetch effective address, extra cycle on page crossing (-> ar)
		case AE_ABSX:
			read_to(pc++, ar);
			state = AE_ABSX1;
			break;
		case AE_ABSX1:
			read_to(pc++, data);
			if (ar + x < 0x100) {
				ar = ((ar + x) & 0xff) | (data << 8);
				Execute;
			} else {
				ar = ((ar + x) & 0xff) | (data << 8);
				state = AE_ABSX2;
			}
			break;
		case AE_ABSX2:	// Page crossed
			read_idle(ar);
			ar += 0x100;
			Execute;

		case AE_ABSY:
			read_to(pc++, ar);
			state = AE_ABSY1;
			break;
		case AE_ABSY1:
			read_to(pc++, data);
			if (ar + y < 0x100) {
				ar = ((ar + y) & 0xff) | (data << 8);
				Execute;
			} else {
				ar = ((ar + y) & 0xff) | (data << 8);
				state = AE_ABSY2;
			}
			break;
		case AE_ABSY2:	// Page crossed
			read_idle(ar);
			ar += 0x100;
			Execute;

		case AE_INDY:
			read_to(pc++, ar2);
			state = AE_INDY1;
			break;
		case AE_INDY1:
			read_to(ar2, ar);
			state = AE_INDY2;
			break;
		case AE_INDY2:
			read_to((ar2 + 1) & 0xff, data);
			if (ar + y < 0x100) {
				ar = ((ar + y) & 0xff) | (data << 8);
				Execute;
			} else {
				ar = ((ar + y) & 0xff) | (data << 8);
				state = AE_INDY3;
			}
			break;
		case AE_INDY3:	// Page crossed
			read_idle(ar);
			ar += 0x100;
			Execute;


		// Addressing modes: Read operand, write it back, no extra cycles (-> ar, rdbuf)
		case M_ZERO:
			read_to(pc++, ar);
			DoRMW;

		case M_ZEROX:
			read_to(pc++, ar);
			state = M_ZEROX1;
			break;
		case M_ZEROX1:
			read_idle(ar);
			ar = (ar + x) & 0xff;
			DoRMW;

		case M_ZEROY:
			read_to(pc++, ar);
			state = M_ZEROY1;
			break;
		case M_ZEROY1:
			read_idle(ar);
			ar = (ar + y) & 0xff;
			DoRMW;

		case M_ABS:
			read_to(pc++, ar);
			state = M_ABS1;
			break;
		case M_ABS1:
			read_to(pc++, data);
			ar = ar | (data << 8);
			DoRMW;

		case M_ABSX:
			read_to(pc++, ar);
			state = M_ABSX1;
			break;
		case M_ABSX1:
			read_to(pc++, data);
			if (ar + x < 0x100) {
				state = M_ABSX2;
			} else {
				state = M_ABSX3;
			}
			ar = ((ar + x) & 0xff) | (data << 8);
			break;
		case M_ABSX2:	// No page crossed
			read_idle(ar);
			DoRMW;
		case M_ABSX3:	// Page crossed
			read_idle(ar);
			ar += 0x100;
			DoRMW;

		case M_ABSY:
			read_to(pc++, ar);
			state = M_ABSY1;
			break;
		case M_ABSY1:
			read_to(pc++, data);
			if (ar + y < 0x100) {
				state = M_ABSY2;
			} else {
				state = M_ABSY3;
			}
			ar = ((ar + y) & 0xff) | (data << 8);
			break;
		case M_ABSY2:	// No page crossed
			read_idle(ar);
			DoRMW;
		case M_ABSY3:	// Page crossed
			read_idle(ar);
			ar += 0x100;
			DoRMW;

		case M_INDX:
			read_to(pc++, ar2);
			state = M_INDX1;
			break;
		case M_INDX1:
			read_idle(ar2);
			ar2 = (ar2 + x) & 0xff;
			state = M_INDX2;
			break;
		case M_INDX2:
			read_to(ar2, ar);
			state = M_INDX3;
			break;
		case M_INDX3:
			read_to((ar2 + 1) & 0xff, data);
			ar = ar | (data << 8);
			DoRMW;

		case M_INDY:
			read_to(pc++, ar2);
			state = M_INDY1;
			break;
		case M_INDY1:
			read_to(ar2, ar);
			state = M_INDY2;
			break;
		case M_INDY2:
			read_to((ar2 + 1) & 0xff, data);
			if (ar + y < 0x100) {
				state = M_INDY3;
			} else {
				state = M_INDY4;
			}
			ar = ((ar + y) & 0xff) | (data << 8);
			break;
		case M_INDY3:	// No page crossed
			read_idle(ar);
			DoRMW;
		case M_INDY4:	// Page crossed
			read_idle(ar);
			ar += 0x100;
			DoRMW;

		case RMW_DO_IT:
			read_to(ar, rdbuf);
			state = RMW_DO_IT1;
			break;
		case RMW_DO_IT1:
			write_byte(ar, rdbuf);
			Execute;


		// Load group
		case O_LDA:
			read_to(ar, data);
			set_nz(a = data);
			Last;
		case O_LDA_I:
			read_to(pc++, data);
			set_nz(a = data);
			Last;

		case O_LDX:
			read_to(ar, data);
			set_nz(x = data);
			Last;
		case O_LDX_I:
			read_to(pc++, data);
			set_nz(x = data);
			Last;

		case O_LDY:
			read_to(ar, data);
			set_nz(y = data);
			Last;
		case O_LDY_I:
			read_to(pc++, data);
			set_nz(y = data);
			Last;


		// Store group
		case O_STA:
			write_byte(ar, a);
			Last;

		case O_STX:
			write_byte(ar, x);
			Last;

		case O_STY:
			write_byte(ar, y);
			Last;


		// Transfer group
		case O_TAX:
			read_idle(pc);
			set_nz(x = a);
			Last;

		case O_TXA:
			read_idle(pc);
			set_nz(a = x);
			Last;

		case O_TAY:
			read_idle(pc);
			set_nz(y = a);
			Last;

		case O_TYA:
			read_idle(pc);
			set_nz(a = y);
			Last;

		case O_TSX:
			read_idle(pc);
			set_nz(x = sp);
			Last;

		case O_TXS:
			read_idle(pc);
			sp = x;
			Last;


		// Arithmetic group
		case O_ADC:
			read_to(ar, data);
			do_adc(data);
			Last;
		case O_ADC_I:
			read_to(pc++, data);
			do_adc(data);
			Last;

		case O_SBC:
			read_to(ar, data);
			do_sbc(data);
			Last;
		case O_SBC_I:
			read_to(pc++, data);
			do_sbc(data);
			Last;


		// Increment/decrement group
		case O_INX:
			read_idle(pc);
			set_nz(++x);
			Last;

		case O_DEX:
			read_idle(pc);
			set_nz(--x);
			Last;

		case O_INY:
			read_idle(pc);
			set_nz(++y);
			Last;

		case O_DEY:
			read_idle(pc);
			set_nz(--y);
			Last;

		case O_INC:
			write_byte(ar, set_nz(rdbuf + 1));
			Last;

		case O_DEC:
			write_byte(ar, set_nz(rdbuf - 1));
			Last;


		// Logic group
		case O_AND:
			read_to(ar, data);
			set_nz(a &= data);
			Last;
		case O_AND_I:
			read_to(pc++, data);
			set_nz(a &= data);
			Last;

		case O_ORA:
			read_to(ar, data);
			set_nz(a |= data);
			Last;
		case O_ORA_I:
			read_to(pc++, data);
			set_nz(a |= data);
			Last;

		case O_EOR:
			read_to(ar, data);
			set_nz(a ^= data);
			Last;
		case O_EOR_I:
			read_to(pc++, data);
			set_nz(a ^= data);
			Last;

		// Compare group
		case O_CMP:
			read_to(ar, data);
			set_nz(ar = a - data);
			c_flag = ar < 0x100;
			Last;
		case O_CMP_I:
			read_to(pc++, data);
			set_nz(ar = a - data);
			c_flag = ar < 0x100;
			Last;

		case O_CPX:
			read_to(ar, data);
			set_nz(ar = x - data);
			c_flag = ar < 0x100;
			Last;
		case O_CPX_I:
			read_to(pc++, data);
			set_nz(ar = x - data);
			c_flag = ar < 0x100;
			Last;

		case O_CPY:
			read_to(ar, data);
			set_nz(ar = y - data);
			c_flag = ar < 0x100;
			Last;
		case O_CPY_I:
			read_to(pc++, data);
			set_nz(ar = y - data);
			c_flag = ar < 0x100;
			Last;


		// Bit-test group
		case O_BIT:
			read_to(ar, data);
			z_flag = a & data;
			n_flag = data;
			v_flag = data & 0x40;
			Last;


		// Shift/rotate group
		case O_ASL:
			c_flag = rdbuf & 0x80;
			write_byte(ar, set_nz(rdbuf << 1));
			Last;
		case O_ASL_A:
			read_idle(pc);
			c_flag = a & 0x80;
			set_nz(a <<= 1);
			Last;

		case O_LSR:
			c_flag = rdbuf & 0x01;
			write_byte(ar, set_nz(rdbuf >> 1));
			Last;
		case O_LSR_A:
			read_idle(pc);
			c_flag = a & 0x01;
			set_nz(a >>= 1);
			Last;

		case O_ROL:
			write_byte(ar, set_nz(c_flag ? (rdbuf << 1) | 0x01 : rdbuf << 1));
			c_flag = rdbuf & 0x80;
			Last;
		case O_ROL_A:
			read_idle(pc);
			data = a & 0x80;
			set_nz(a = c_flag ? (a << 1) | 0x01 : a << 1);
			c_flag = data;
			Last;

		case O_ROR:
			write_byte(ar, set_nz(c_flag ? (rdbuf >> 1) | 0x80 : rdbuf >> 1));
			c_flag = rdbuf & 0x01;
			Last;
		case O_ROR_A:
			read_idle(pc);
			data = a & 0x01;
			set_nz(a = (c_flag ? (a >> 1) | 0x80 : a >> 1));
			c_flag = data;
			Last;


		// Stack group
		case O_PHA:
			read_idle(pc);
			state = O_PHA1;
			break;
		case O_PHA1:
			write_byte(sp-- | 0x100, a);
			Last;

		case O_PLA:
			read_idle(pc);
			state = O_PLA1;
			break;
		case O_PLA1:
			read_idle(sp++ | 0x100);
			state = O_PLA2;
			break;
		case O_PLA2:
			read_to(sp | 0x100, data);
			set_nz(a = data);
			Last;

		case O_PHP:
			read_idle(pc);
			state = O_PHP1;
			break;
		case O_PHP1:
			push_flags(true);
			Last;

		case O_PLP:
			read_idle(pc);
			state = O_PLP1;
			break;
		case O_PLP1:
			read_idle(sp++ | 0x100);
			state = O_PLP2;
			break;
		case O_PLP2:
			check_interrupts();	// Flag change is internally delayed, check interrupts first
			pop_flags();
			state = O_FETCH;
			break;


		// Jump/branch group
		case O_JMP:
			read_to(pc++, ar);
			state = O_JMP1;
			break;
		case O_JMP1:
			read_to(pc, data);
			pc = (data << 8) | ar;
			Last;

		case O_JMP_I:
			read_to(ar, pc);
			state = O_JMP_I1;
			break;
		case O_JMP_I1:
			read_to(((ar + 1) & 0xff) | (ar & 0xff00), data);
			pc |= data << 8;
			Last;

		case O_JSR:
			read_to(pc++, ar);
			state = O_JSR1;
			break;
		case O_JSR1:
			read_idle(sp | 0x100);
			state = O_JSR2;
			break;
		case O_JSR2:
			write_byte(sp-- | 0x100, pc >> 8);
			state = O_JSR3;
			break;
		case O_JSR3:
			write_byte(sp-- | 0x100, pc);
			state = O_JSR4;
			break;
		case O_JSR4:
			read_to(pc++, data);
			pc = ar | (data << 8);
			Last;

		case O_RTS:
			read_idle(pc);
			state = O_RTS1;
			break;
		case O_RTS1:
			read_idle(sp++ | 0x100);
			state = O_RTS2;
			break;
		case O_RTS2:
			read_to(sp++ | 0x100, pc);
			state = O_RTS3;
			break;
		case O_RTS3:
			read_to(sp | 0x100, data);
			pc |= data << 8;
			state = O_RTS4;
			break;
		case O_RTS4:
			read_idle(pc++);
			Last;

		case O_RTI:
			read_idle(pc);
			state = O_RTI1;
			break;
		case O_RTI1:
			read_idle(sp++ | 0x100);
			state = O_RTI2;
			break;
		case O_RTI2:
			pop_flags();
			sp++;
			state = O_RTI3;
			break;
		case O_RTI3:
			read_to(sp++ | 0x100, pc);
			state = O_RTI4;
			break;
		case O_RTI4:
			read_to(sp | 0x100, data);
			pc |= data << 8;
			Last;

		case O_BCS:
			Branch(c_flag);

		case O_BCC:
			Branch(!c_flag);

		case O_BEQ:
			Branch(!z_flag);

		case O_BNE:
			Branch(z_flag);

		case O_BVS:
			CHECK_SO;	// Handle SO (GCR byte ready) input on 1541
			Branch(v_flag);

		case O_BVC:
			CHECK_SO;	// Handle SO (GCR byte ready) input on 1541
			Branch(!v_flag);

		case O_BMI:
			Branch(n_flag & 0x80);

		case O_BPL:
			Branch(!(n_flag & 0x80));

		case O_BRANCH_NP:		// No page crossed
			read_idle(pc);
			pc = ar;
			state = O_FETCH;	// No interrupt check
			break;
		case O_BRANCH_BP:		// Page crossed, branch backwards
			read_idle(pc);
			pc = ar;
			state = O_BRANCH_BP1;
			break;
		case O_BRANCH_BP1:
			read_idle(pc + 0x100);
			Last;
		case O_BRANCH_FP:		// Page crossed, branch forwards
			read_idle(pc);
			pc = ar;
			state = O_BRANCH_FP1;
			break;
		case O_BRANCH_FP1:
			read_idle(pc - 0x100);
			Last;


		// Flag group
		case O_SEC:
			read_idle(pc);
			c_flag = true;
			Last;

		case O_CLC:
			read_idle(pc);
			c_flag = false;
			Last;

		case O_SED:
			read_idle(pc);
			d_flag = true;
			Last;

		case O_CLD:
			read_idle(pc);
			d_flag = false;
			Last;

		case O_SEI:
#ifndef IS_CPU_1541
			if (BALow && irq_delay) {
				i_flag = true;	// Waiting for BA cancels the flag change delay
			}
#endif
			read_idle(pc);
			check_interrupts();	// Flag change is internally delayed, check interrupts first
			i_flag = true;
			state = O_FETCH;
			break;

		case O_CLI:
#ifndef IS_CPU_1541
			if (BALow) {
				i_flag = false;	// Waiting for BA cancels the flag change delay
			}
#endif
			read_idle(pc);
			check_interrupts();	// Flag change is internally delayed, check interrupts first
			i_flag = false;
			state = O_FETCH;
			break;

		case O_CLV:
			read_idle(pc);
			v_flag = false;
			Last;


		// NOP group
		case O_NOP:
			read_idle(pc);
			Last;


/*
 *  Undocumented opcodes start here
 */

		// NOP group
		case O_NOP_I:
			read_idle(pc++);
			Last;

		case O_NOP_A:
			read_idle(ar);
			Last;


		// Load A/X group
		case O_LAX:
			read_to(ar, data);
			set_nz(a = x = data);
			Last;


		// Store A/X group
		case O_SAX:
			write_byte(ar, a & x);
			Last;


		// ASL/ORA group
		case O_SLO:
			c_flag = rdbuf & 0x80;
			rdbuf <<= 1;
			write_byte(ar, rdbuf);
			set_nz(a |= rdbuf);
			Last;


		// ROL/AND group
		case O_RLA:
			tmp = rdbuf & 0x80;
			rdbuf = c_flag ? (rdbuf << 1) | 0x01 : rdbuf << 1;
			c_flag = tmp;
			write_byte(ar, rdbuf);
			set_nz(a &= rdbuf);
			Last;


		// LSR/EOR group
		case O_SRE:
			c_flag = rdbuf & 0x01;
			rdbuf >>= 1;
			write_byte(ar, rdbuf);
			set_nz(a ^= rdbuf);
			Last;


		// ROR/ADC group
		case O_RRA:
			tmp = rdbuf & 0x01;
			rdbuf = c_flag ? (rdbuf >> 1) | 0x80 : rdbuf >> 1;
			c_flag = tmp;
			write_byte(ar, rdbuf);
			do_adc(rdbuf);
			Last;


		// DEC/CMP group
		case O_DCP:
			write_byte(ar, --rdbuf);
			set_nz(ar = a - rdbuf);
			c_flag = ar < 0x100;
			Last;


		// INC/SBC group
		case O_ISB:
			write_byte(ar, ++rdbuf);
			do_sbc(rdbuf);
			Last;


		// Complex functions
		case O_ANC_I:
			read_to(pc++, data);
			set_nz(a &= data);
			c_flag = n_flag & 0x80;
			Last;

		case O_ASR_I:
			read_to(pc++, data);
			a &= data;
			c_flag = a & 0x01;
			set_nz(a >>= 1);
			Last;

		case O_ARR_I:
			read_to(pc++, data);
			data &= a;
			a = (c_flag ? (data >> 1) | 0x80 : data >> 1);
			if (!d_flag) {
				set_nz(a);
				c_flag = a & 0x40;
				v_flag = (a & 0x40) ^ ((a & 0x20) << 1);
			} else {
				n_flag = c_flag ? 0x80 : 0;
				z_flag = a;
				v_flag = (data ^ a) & 0x40;
				if ((data & 0x0f) + (data & 0x01) > 5) {
					a = (a & 0xf0) | ((a + 6) & 0x0f);
				}
				if ((c_flag = ((data + (data & 0x10)) & 0x1f0) > 0x50) != 0) {
					a += 0x60;
				}
			}
			Last;

		case O_ANE_I:
			read_to(pc++, data);
			set_nz(a = (a | 0xee) & x & data);
			Last;

		case O_LXA_I:
			read_to(pc++, data);
			set_nz(a = x = (a | 0xee) & data);
			Last;

		case O_SBX_I:
			read_to(pc++, data);
			set_nz(x = ar = (x & a) - data);
			c_flag = ar < 0x100;
			Last;

		case O_LAS:
			read_to(ar, data);
			set_nz(a = x = sp = data & sp);
			Last;

		case O_SHS:		// ar2 contains the high byte of the operand address
			if ((ar & 0xff) < y) {	// Page crossed?
				ar &= ((a & x) << 8) | 0xff;
			}
			write_byte(ar, (ar2 + 1) & (sp = a & x));
			Last;

		case O_SHY:		// ar2 contains the high byte of the operand address
			if ((ar & 0xff) < x) {	// Page crossed?
				ar &= (y << 8) | 0xff;
			}
			write_byte(ar, y & (ar2 + 1));
			Last;

		case O_SHX:		// ar2 contains the high byte of the operand address
			if ((ar & 0xff) < y) {	// Page crossed?
				ar &= (x << 8) | 0xff;
			}
			write_byte(ar, x & (ar2 + 1));
			Last;

		case O_SHA:		// ar2 contains the high byte of the operand address
			if ((ar & 0xff) < y) {	// Page crossed?
				ar &= ((a & x) << 8) | 0xff;
			}
			write_byte(ar, a & x & (ar2 + 1));
			Last;
