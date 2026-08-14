/*
 *  VIA.cpp - 6522 emulation
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
	s->t1_load_delay = 0;
	s->t2_load_delay = 0;
	s->t2_input_delay = 0;
	s->irq_delay = 0;
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
		the_cpu->ClearInterrupt(irq_type);
	}
}

// Trigger VIA interrupt
inline void MOS6522::trigger_irq()
{
	ifr |= 0x80;
	the_cpu->TriggerInterrupt(irq_type);
}

// Trigger external CA1 interrupt
void MOS6522::TriggerCA1Interrupt()
{
	if (pcr & 0x01) {		// CA1 positive edge (1541 gets inverted bus signals)
		ifr |= 0x02;
		if (ier & 0x02) {	// CA1 interrupt enabled?
			trigger_irq();
		}
	}
}


/*
 *  Count VIA timers
 */

void MOS6522::CountTimers(int cycles)
{
	unsigned long tmp;

	t1c = tmp = t1c - cycles;
	if (tmp > 0xffff) {
		if (!t1_irq_blocked) {
			ifr |= 0x40;
			if (ier & 0x40) {
				trigger_irq();
			}
		}
		if ((acr & 0x40) == 0) {	// One-shot mode
			t1_irq_blocked = true;
		}
		t1c = t1l;					// Reload from latch
	}

	if ((acr & 0x20) == 0) {		// Only count in one-shot mode
		t2c = tmp = t2c - cycles;
		if (tmp > 0xffff) {
			if (!t2_irq_blocked) {
				t2_irq_blocked = true;
				ifr |= 0x20;
				if (ier & 0x20) {
					trigger_irq();
				}
			}
		}
	}
}
