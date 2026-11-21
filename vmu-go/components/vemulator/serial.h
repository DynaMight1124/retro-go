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

#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "common.h"
#include "ram.h"
#include "interrupts.h"
#include "state.h"

/* The LC86K's two synchronous serial channels, SIO0 and SIO1 -- the cable a
   VMU uses to talk to another VMU.
 *
 * Modelled a byte at a time rather than a bit at a time: on hardware a channel
 * is bit-banged over Port 1 (SO0/SI0/SCK0 on P1.0-2, SO1/SI1/SCK1 on P1.3-5)
 * and a transfer is eight clock edges at the rate SBR sets, where here the
 * whole byte moves at the tick the eighth edge would have landed. Three limits
 * follow from that:
 *
 *  - Software that watches the pins mid-byte, or clocks by hand instead of
 *    letting the serial block do it, will not be fooled.
 *  - SCON is sampled once a cycle rather than hooked on write, so a guest that
 *    cleared and re-set CTRL without a cycle in between would have its restart
 *    missed. Real code cannot: instructions take cycles.
 *  - A transfer is atomic on the wire but not in time here: the sender holds
 *    its completion open briefly after the eighth edge for the peer's half to
 *    cross the transport. On hardware both halves land on the same edge.
 */

#define SERIAL_IFACE_COUNT 2

/* SCON0/SCON1 bits. Only SCON0 has POL; the rest are common to both. */
#define SCON_IE_MASK   0x01  /* interrupt enable                        */
#define SCON_END_MASK  0x02  /* transfer end flag                       */
#define SCON_MSB_MASK  0x04  /* 1: MSB first, 0: LSB first              */
#define SCON_CTRL_MASK 0x08  /* 1: start, 0: stop                       */
#define SCON_LEN_MASK  0x10  /* 0: 8-bit transfer, 1: continuous        */
#define SCON_OV_MASK   0x40  /* overrun                                 */
#define SCON_POL_MASK  0x80  /* SIO0 only: clock polarity               */

class VE_VMS_SERIAL
{
public:
    VE_VMS_SERIAL(VE_VMS_RAM *_ram, VE_VMS_INTERRUPTS *_intHandler);

    void serialize(VE_STATE &s);

    /* Advance both channels by one CPU cycle. Picks up a transfer the guest
       started by setting SCON.CTRL since the last call. */
    void runCycle();

    /* A byte this VMU clocked out, ready for the cable. Returns false when
       nothing is outstanding. `iface` is the channel it left BY; the transport
       crosses it, because two VMUs clip together face to face and one unit's
       SIO0 lands on the other's SIO1. */
    bool takeOutgoing(int *iface, byte *value);

    /* A byte that arrived from the peer. Lands in SBUF, raises the channel's
       interrupt if the guest asked for one. A listening channel answers with
       whatever its own SBUF holds: the link is full duplex. */
    void deliver(int iface, byte value);

    /* Whether anything is cabled to us. A channel with nothing on the other
       end still completes its transfers -- a real VMU clocking into thin air
       does too -- but nothing is queued for the bus. */
    void setCabled(bool cabled);

    /* Cycles a whole byte takes at the current SBR setting, for stamping a
       send on the peer's timeline. */
    int byteCycles() const;

private:
    struct Iface
    {
        size_t sconAddr;
        size_t sbufAddr;
        byte   sckMask;   /* this channel's SCK bit within P1          */
        byte   siMask;    /* this channel's SI bit within P1           */
        byte   soMask;    /* this channel's SO bit within P1           */
        byte   sconOld;
        bool   sending;
        int    cyclesLeft;
        byte   shiftOut;
        bool   pendingOut;
        byte   outValue;
        bool   awaitingReply;  /* waiting for the peer's half of the exchange */
        int    replyTimeout;
    };

    bool isSender(const Iface &iface) const;
    void startTransfer(Iface &iface);
    void finishSend(Iface &iface, int index);
    void restartTransfer(Iface &iface);
    void completeTransfer(Iface &iface, int index);
    void raiseInterrupt(int index);

    VE_VMS_RAM        *ram;
    VE_VMS_INTERRUPTS *intHandler;
    Iface              ifaces[SERIAL_IFACE_COUNT];
    bool               cabled;
};

#endif // _SERIAL_H_
