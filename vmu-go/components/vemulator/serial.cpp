/*
    VeMUlator - A Dreamcast Visual Memory Unit emulator for libretro
    Copyright (C) 2018  Mahmoud Jaoune

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "serial.h"

VE_VMS_SERIAL::VE_VMS_SERIAL(VE_VMS_RAM *_ram, VE_VMS_INTERRUPTS *_intHandler)
{
   int i;

   ram        = _ram;
   intHandler = _intHandler;
   cabled     = false;

   for(i = 0; i < SERIAL_IFACE_COUNT; i++)
   {
      Iface &iface = ifaces[i];

      /* SIO0 sits on P1.0-2, SIO1 on P1.3-5. */
      iface.sconAddr   = i ? SCON1 : SCON0;
      iface.sbufAddr   = i ? SBUF1 : SBUF0;
      iface.soMask     = (byte)(1 << (i * 3));
      iface.siMask     = (byte)(2 << (i * 3));
      iface.sckMask    = (byte)(4 << (i * 3));
      iface.sconOld    = 0;
      iface.sending    = false;
      iface.cyclesLeft = 0;
      iface.shiftOut   = 0;
      iface.pendingOut = false;
      iface.outValue   = 0;
      iface.awaitingReply = false;
      iface.replyTimeout  = 0;
   }
}

void VE_VMS_SERIAL::setCabled(bool _cabled)
{
   cabled = _cabled;
}

/* A byte is eight clock periods, and SBR sets the period: the serial block
   divides the CPU clock by (256 - SBR) * 2. SBR untouched gives the slowest
   rate, which is also the safe answer when the guest never set it. */
int VE_VMS_SERIAL::byteCycles() const
{
   int sbr    = ram->readByte_RAW(SBR) & 0xFF;
   int period = (256 - sbr) * 2;

   if(period < 2)
      period = 2;

   return period * 8;
}

/* Which end of the cable this is: the side driving SCK generates the clock, so
   it times the transfer and everything else waits to be clocked. A pin is a
   serial output only when P1DDR and P1FCR agree on it.
 *
 * SO is deliberately not consulted. A slave drives SO too -- that is the half
 * of the exchange it shifts back while the master clocks -- so reading SO as
 * "this side is the master" turns a unit waiting to be spoken to into one
 * talking to itself. */
bool VE_VMS_SERIAL::isSender(const Iface &iface) const
{
   byte ddr = ram->readByte_RAW(P1DDR);
   byte fcr = ram->readByte_RAW(P1FCR);

   return (ddr & iface.sckMask) && (fcr & iface.sckMask);
}

void VE_VMS_SERIAL::startTransfer(Iface &iface)
{
   byte scon = ram->readByte_RAW(iface.sconAddr);

   /* Clear the end flag for the duration; the guest polls it. */
   ram->writeByte_RAW(iface.sconAddr, (byte)(scon & ~SCON_END_MASK));

   iface.awaitingReply = false;

   if(isSender(iface))
   {
      iface.sending    = true;
      iface.shiftOut   = ram->readByte_RAW(iface.sbufAddr);
      iface.cyclesLeft = byteCycles();
   }
   else
   {
      /* Listening. The transfer completes when a byte arrives, not on a
         timer -- the peer's clock drives it. */
      iface.sending    = false;
      iface.cyclesLeft = 0;
   }
}

void VE_VMS_SERIAL::raiseInterrupt(int index)
{
   byte scon = ram->readByte_RAW(ifaces[index].sconAddr);

   if(!(scon & SCON_IE_MASK))
      return;

   if(index == 0)
      intHandler->setSIO0();
   else
      intHandler->setSIO1();
}

void VE_VMS_SERIAL::completeTransfer(Iface &iface, int index)
{
   byte scon = ram->readByte_RAW(iface.sconAddr);

   scon |= SCON_END_MASK;

   /* An 8-bit transfer clears CTRL when it finishes; a continuous one keeps
      running until the guest stops it. */
   if(!(scon & SCON_LEN_MASK))
      scon &= ~SCON_CTRL_MASK;

   ram->writeByte_RAW(iface.sconAddr, scon);
   iface.sconOld = scon;

   raiseInterrupt(index);

   /* Continuous mode does not stop at a byte boundary: the shift register is
      reloaded and the clock keeps running. This is the mode VMU-to-VMU
      software uses. */
   if(scon & SCON_LEN_MASK)
      restartTransfer(iface);
}

void VE_VMS_SERIAL::restartTransfer(Iface &iface)
{
   if(!isSender(iface))
   {
      iface.sending    = false;
      iface.cyclesLeft = 0;
      return;
   }

   iface.sending    = true;
   iface.shiftOut   = ram->readByte_RAW(iface.sbufAddr);
   iface.cyclesLeft = byteCycles();
}

void VE_VMS_SERIAL::runCycle()
{
   int i;

   for(i = 0; i < SERIAL_IFACE_COUNT; i++)
   {
      Iface &iface = ifaces[i];
      byte scon    = ram->readByte_RAW(iface.sconAddr);

      /* CTRL going 0 -> 1 is the guest starting a transfer. */
      if((scon & SCON_CTRL_MASK) && !(iface.sconOld & SCON_CTRL_MASK))
         startTransfer(iface);

      /* CTRL going 1 -> 0 is the guest aborting one. */
      if(!(scon & SCON_CTRL_MASK) && (iface.sconOld & SCON_CTRL_MASK))
      {
         iface.sending    = false;
         iface.cyclesLeft = 0;
      }

      iface.sconOld = ram->readByte_RAW(iface.sconAddr);

      if(iface.awaitingReply && --iface.replyTimeout <= 0)
      {
         iface.awaitingReply = false;
         completeTransfer(iface, i);
      }

      if(!iface.sending || iface.cyclesLeft <= 0)
         continue;

      if(--iface.cyclesLeft > 0)
         continue;

      /* Eighth edge. The byte is on the wire. */
      finishSend(iface, i);
   }
}

/* The sender's eighth edge. On hardware the peer's half lands on that same
   edge; here it has to cross the transport first, so the transfer is held open
   for a moment to let it and ends on the timeout if nothing comes. */
void VE_VMS_SERIAL::finishSend(Iface &iface, int index)
{
   iface.sending = false;

   if(!cabled)
   {
      completeTransfer(iface, index);
      return;
   }

   if(!iface.pendingOut)
   {
      iface.pendingOut = true;
      iface.outValue   = iface.shiftOut;
   }

   iface.awaitingReply = true;
   iface.replyTimeout  = byteCycles() * 4;
}

bool VE_VMS_SERIAL::takeOutgoing(int *iface, byte *value)
{
   int i;

   for(i = 0; i < SERIAL_IFACE_COUNT; i++)
   {
      if(!ifaces[i].pendingOut)
         continue;

      ifaces[i].pendingOut = false;
      *iface = i;
      *value = ifaces[i].outValue;
      return true;
   }

   return false;
}

void VE_VMS_SERIAL::deliver(int index, byte value)
{
   if(index < 0 || index >= SERIAL_IFACE_COUNT)
      return;

   {
      Iface &iface = ifaces[index];
      byte scon    = ram->readByte_RAW(iface.sconAddr);
      byte reply;

      /* The other half of a transfer this side clocked out. It belongs to the
         byte just sent, so it lands in SBUF and ends that transfer. It is not
         answered again, or the two would volley forever. */
      if(iface.awaitingReply)
      {
         iface.awaitingReply = false;
         ram->writeByte_RAW(iface.sbufAddr, value);
         completeTransfer(iface, index);
         return;
      }

      /* A byte arriving while the guest is not listening is an overrun on
         hardware, and the guest is told so rather than quietly fed. */
      if(!(scon & SCON_CTRL_MASK))
      {
         ram->writeByte_RAW(iface.sconAddr, (byte)(scon | SCON_OV_MASK));
         iface.sconOld = ram->readByte_RAW(iface.sconAddr);
         return;
      }

      /* Listening. The byte lands in SBUF and the transfer is over. The shift
         register is one register, so what was in SBUF went out to the peer on
         the same eight edges that brought this in. A side that was clocking
         its own byte out at the same time is exchanging, not colliding, so its
         completion is left to run out on its own timer. */
      reply = ram->readByte_RAW(iface.sbufAddr);
      ram->writeByte_RAW(iface.sbufAddr, value);

      if(cabled && !iface.pendingOut)
      {
         iface.pendingOut = true;
         iface.outValue   = reply;
      }

      if(!iface.sending)
         completeTransfer(iface, index);
   }
}

void VE_VMS_SERIAL::serialize(VE_STATE &s)
{
   int i;

   for(i = 0; i < SERIAL_IFACE_COUNT; i++)
   {
      Iface &iface = ifaces[i];

      s.u8(iface.sconOld);
      s.b(iface.sending);
      s.i32(iface.cyclesLeft);
      s.u8(iface.shiftOut);
      s.b(iface.pendingOut);
      s.u8(iface.outValue);
      s.b(iface.awaitingReply);
      s.i32(iface.replyTimeout);
   }

   s.b(cabled);
}
