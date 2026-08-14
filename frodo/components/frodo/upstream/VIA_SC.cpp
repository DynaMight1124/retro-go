/*
 *  VIA_SC.cpp - Single-cycle 6522 emulation
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
 * Incompatibilities:
 * ------------------
 *
 *  - No port latches, no timers on port B, no CA2/CB2, no shift register
 */

#include "sysdeps.h"

#include "VIA.h"
#include "CPU1541.h"


/*
 *  Reset VIA
 */

void MOS6522::Reset()
{
	// Note: 6522 reset doesn't actually touch the timers nor the shift
	// register, but we want to avoid undefined behavior.
	pra = ddra = prb = ddrb = 0;
	t1c = t1l = t2c = t2l = 0xffff;
	sr = 0;
	acr = pcr = 0;
	ifr = ier = 0;

	t1_irq_blocked = false;
	t2_irq_blocked = false;
	t1_load_delay = 0;
	t2_load_delay = 0;
	t2_input_delay = 0;
	irq_delay = 0;
}


/*
 *  Get VIA register state
 */

void MOS6522::GetState(MOS6522State * s) const
{
	s->pra = pra; s->ddra = ddra;
	s->prb = prb; s->ddrb = ddrb;
	s->t1c = t1c; s->t1l  = t1l;
	s->t2c = t2c; s->t2l  = t2l;
	s->sr  = sr;
	s->acr = acr; s->pcr  = pcr;
	s->ifr = ifr; s->ier  = ier;

	s->t1_irq_blocked = t1_irq_blocked;
	s->t2_irq_blocked = t2_irq_blocked;
	s->t1_load_delay = t1_load_delay;
	s->t2_load_delay = t2_load_delay;
	s->t2_input_delay = t2_input_delay;
	s->irq_delay = irq_delay;
}


/*
 *  Set VIA register state
 */

void MOS6522::SetState(const MOS6522State * s)
{
	pra = s->pra; ddra = s->ddra;
	prb = s->prb; ddrb = s->ddrb;
	t1c = s->t1c; t1l  = s->t1l;
	t2c = s->t2c; t2l  = s->t2l;
	sr  = s->sr;
	acr = s->acr; pcr  = s->pcr;
	ifr = s->ifr; ier  = s->ier;

	t1_irq_blocked = s->t1_irq_blocked;
	t2_irq_blocked = s->t2_irq_blocked;
	t1_load_delay = s->t1_load_delay;
	t2_load_delay = s->t2_load_delay;
	t2_input_delay = s->t2_input_delay;
	irq_delay = s->irq_delay;
}


/*
 *  Interrupt functions
 */

// Clear VIA interrupt flag, deassert IRQ line if no interrupts are pending
void MOS6522::clear_irq(uint8_t flag)
{
	ifr &= ~flag;
	if ((ifr & ier & 0x7f) == 0) {
		ifr &= 0x7f;
		irq_delay = 0;
	}
}

// Trigger external CA1 interrupt
void MOS6522::TriggerCA1Interrupt()
{
	if (pcr & 0x01) {		// CA1 positive edge (1541 gets inverted bus signals)
		ifr |= 0x02;
	}
}


/*
 *  Emulate VIAs for one cycle
 */

void MOS6522::EmulateCycle()
{
	// Shift delay lines
	t1_load_delay <<= 1;
	t2_load_delay <<= 1;
	t2_input_delay <<= 1;
	irq_delay <<= 1;

	// Update timer inputs
	if ((acr & 0x20) == 0) {		// Don't count PB6 pulses
		t2_input_delay |= 1;
	}

	// Reload or count timer 1
	if (t1_load_delay & 2) {		// One cycle of load delay
		t1c = t1l;
	} else {
		--t1c;
		if (t1c == 0xffff) {
			if (!t1_irq_blocked) {
				ifr |= 0x40;
			}
			if ((acr & 0x40) == 0) {	// One-shot mode
				t1_irq_blocked = true;
			}
			t1_load_delay |= 1;		// Reload in next cycle
		}
	}

	// Reload or count timer 2
	if (t2_load_delay & 2) {		// One cycle of load delay
		t2c = t2l;
	} else {
		if (t2_input_delay & 2) {	// One cycle of input delay
			--t2c;
			if (t2c == 0xffff) {
				if (!t2_irq_blocked) {
					t2_irq_blocked = true;
					ifr |= 0x20;
				}
			}
		}
	}

	// Update IRQ status
	if (ifr & ier) {
		irq_delay |= 1;
	}
	if (irq_delay & 2) {			// One cycle of IRQ delay
		if ((ifr & 0x80) == 0) {
			ifr |= 0x80;
			the_cpu->TriggerInterrupt(irq_type);
		}
	} else {
		the_cpu->ClearInterrupt(irq_type);
	}
}
