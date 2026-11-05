/* gameplaySP
 *
 * Copyright (C) 2023 David Guillen Fandos <david@davidgf.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */
#define SERIAL_MODE_DISABLED      0
#define SERIAL_MODE_GBP           1  // Connected to the GB Player
#define SERIAL_MODE_RFU           2  // Wifi Adapter
#define SERIAL_MODE_SERIAL_POKE   3  // Connected to serial cable (Pokemon emulation mode)
#define SERIAL_MODE_SERIAL_AW1    4  // Connected to serial cable (AdvWars emulation mode)
#define SERIAL_MODE_SERIAL_AW2    5  // Connected to serial cable (AdvWars emulation mode)
#define SERIAL_MODE_AUTO          6  // Choose best fit automatically

#else /* GBSP_BACKEND_VARIANT */
#define SERIAL_MODE_DISABLED      0
#define SERIAL_MODE_RFU           1  // Wifi Adapter
#define SERIAL_MODE_GBP           2  // Connected to the GB Player
#define SERIAL_MODE_AUTO          3  // Choose best fit automatically

#endif /* GBSP_BACKEND_VARIANT */
extern int serial_mode;

// Register writes
cpu_alert_type write_siocnt(u16 value);
cpu_alert_type write_rcnt(u16 value);

// Serial IRQ interface
u32 serial_next_event();
bool update_serial(unsigned cycles);
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

// Savestate accessors
u32 serial_get_irq_cycles(void);
void serial_set_irq_cycles(u32 v);

// RFU interface
#elif defined(XTENSA_ARCH) /* GBSP_BACKEND_VARIANT */

/* update_gba calls update_serial on every event, and with the port disabled
   and no pending IRQ the function dispatches a switch and returns false --
   0.39% of core 0 on the 2026-10-10 Mario Kart profile, for nothing. The two
   loads that decide it are cheaper than the call, and the condition is exactly
   the one under which update_serial has no effect. */
extern u32 serial_irq_cycles;
static inline bool update_serial_idle(void)
{
  return serial_mode == SERIAL_MODE_DISABLED && !serial_irq_cycles;
}

// RFU interface
#else /* GBSP_BACKEND_VARIANT */

// RFU interface
#endif /* GBSP_BACKEND_VARIANT */
void rfu_reset(void);
bool rfu_update(unsigned cycles);
u32 rfu_transfer(u32 value);
void rfu_frame_update(void);
void rfu_net_receive(const void* buf, size_t len, uint16_t client_id);
#if defined(RISCV_ARCH) /* GBSP_BACKEND_VARIANT */

// Serial implementations
void serialproto_reset(void);

void serialpoke_frame_update(void);
void serialpoke_master_send(void);
bool serialpoke_update(unsigned cycles);
void serialpoke_net_receive(const void* buf, size_t len, uint16_t client_id);

void serialaw_frame_update(void);
void serialaw_master_send(void);
bool serialaw_update(unsigned cycles);
void serialaw_net_receive(const void* buf, size_t len, uint16_t client_id);

// GBP interface
u32 gbp_transfer(u32 value);
void gbp_reset(void);
u32 gbp_get_state(void);
void gbp_set_state(u32 v);
#else /* GBSP_BACKEND_VARIANT */

// GBP interface
u32 gbp_transfer(u32 value);
#endif /* GBSP_BACKEND_VARIANT */
