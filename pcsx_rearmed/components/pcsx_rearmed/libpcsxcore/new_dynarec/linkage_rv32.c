/* SPDX-License-Identifier: GPL-2.0-or-later
 * Slow memory bridge for the Ari64 RV32 backend.
 *
 * The two-level tables store either a direct base or a C handler as
 * (pointer >> 1), with the top bit selecting the handler form. Generated
 * code handles ordinary RAM directly; these helpers cover the second-level
 * IO tables and rare unaligned writes while preserving Ari64 cycle state.
 */
#include <stdint.h>

#include "emu_if.h"
#include "../psxmem.h"

extern int last_count;

typedef void (*write_handler_t)(u32 data);

/* Keep this diagnostic at the architectural I/O boundary rather than at any
 * title-specific PC. It shows whether native code reaches the same PSX
 * devices as the established ARM backends. Do not leave its classification
 * work on the normal path: polling-heavy games can cross this boundary
 * hundreds of thousands of times per second. */
#ifdef DRC_DBG
struct rv32_io_stats {
	u32 total;
	u32 reads;
	u32 writes;
	u32 irq;
	u32 dma2;
	u32 timers;
	u32 cdrom;
	u32 gpu_data;
	u32 gpu_status;
};

static struct rv32_io_stats io_stats;

static void io_trace(u32 addr, int write)
{
	u32 reg = addr & 0xffff;
	u32 total;

	/* Only the PSX hardware page is relevant; accept all three mirrors. */
	if ((addr & 0x1ffff000) != 0x1f801000)
		return;

	total = ++io_stats.total;
	if (write)
		io_stats.writes++;
	else
		io_stats.reads++;
	if (reg == 0x1070 || reg == 0x1074)
		io_stats.irq++;
	else if (reg >= 0x10a0 && reg <= 0x10ac)
		io_stats.dma2++;
	else if (reg >= 0x1100 && reg <= 0x1128)
		io_stats.timers++;
	else if (reg >= 0x1800 && reg <= 0x1803)
		io_stats.cdrom++;
	else if (reg == 0x1810)
		io_stats.gpu_data++;
	else if (reg == 0x1814)
		io_stats.gpu_status++;

	if ((total & (total - 1)) == 0)
		SysPrintf("RV32 IO: total=%u r/w=%u/%u irq=%u dma2=%u "
			"timer=%u cd=%u gp0/gp1=%u/%u last=%c%04x\n",
			total, io_stats.reads, io_stats.writes, io_stats.irq,
			io_stats.dma2, io_stats.timers, io_stats.cdrom,
			io_stats.gpu_data, io_stats.gpu_status,
			write ? 'W' : 'R', reg);
}
#else
#define io_trace(addr, write) ((void)0)
#endif

static inline void memhandler_pre(u32 cycles)
{
	psxRegs.cycle = last_count + cycles;
}

static inline u32 memhandler_post(void)
{
	last_count = psxRegs.next_interupt;
	return psxRegs.cycle - last_count;
}

u32 jump_handler_write_h(u32 addr, u32 data, u32 cycles, void *handler)
{
	io_trace(addr, 1);
	address = addr;
	memhandler_pre(cycles);
	((write_handler_t)handler)(data);
	return memhandler_post();
}

static inline void unaligned_pre(u32 cycles)
{
	memhandler_pre(cycles);
}

u32 jump_handle_swl(u32 addr, u32 data, u32 cycles)
{
	unaligned_pre(cycles);
	switch (addr & 3) {
	case 0:
		psxMemWrite8(&psxRegs, addr, data >> 24);
		break;
	case 1:
		psxMemWrite16(&psxRegs, addr & ~3u, data >> 16);
		break;
	case 2:
		psxMemWrite16(&psxRegs, addr & ~3u, (data >> 8) & 0xffff);
		psxMemWrite8(&psxRegs, addr, data >> 24);
		break;
	default:
		psxMemWrite32(&psxRegs, addr & ~3u, data);
		break;
	}
	return memhandler_post();
}

u32 jump_handle_swr(u32 addr, u32 data, u32 cycles)
{
	unaligned_pre(cycles);
	switch (addr & 3) {
	case 0:
		psxMemWrite32(&psxRegs, addr, data);
		break;
	case 1:
		psxMemWrite8(&psxRegs, addr, data);
		psxMemWrite16(&psxRegs, addr + 1, (data >> 8) & 0xffff);
		break;
	case 2:
		psxMemWrite16(&psxRegs, addr, data);
		break;
	default:
		psxMemWrite8(&psxRegs, addr, data);
		break;
	}
	return memhandler_post();
}
