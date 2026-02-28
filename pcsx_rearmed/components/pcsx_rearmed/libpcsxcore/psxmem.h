/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

#ifndef __PSXMEMORY_H__
#define __PSXMEMORY_H__

#ifdef __plusplus
extern "C" {
#endif

#include "psxcommon.h"
#include "r3000a.h"

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SWAP16(v) __builtin_bswap16(v)
#define SWAP32(v) __builtin_bswap32(v)
#define SWAPu32(v) SWAP32((u32)(v))
#define SWAPs32(v) SWAP32((s32)(v))
#define SWAPu16(v) __builtin_bswap16((u16)(v))
#define SWAPs16(v) __builtin_bswap16((s16)(v))
#else
#define SWAP16(b) (b)
#define SWAP32(b) (b)
#define SWAPu32(b) (b)
#define SWAPs32(b) (b)
#define SWAPu16(b) (b)
#define SWAPs16(b) (b)
#endif

#define PSXM_SHIFT 16
#define INVALID_PTR_VAL ((uintptr_t)-1)
#define INVALID_PTR ((void *)INVALID_PTR_VAL)
#define DISABLE_MEM_LUTS 0

static inline int psxm_lut(u8 **ret, const psxRegisters *regs, u32 mem, int write, const uintptr_t *lut)
{
	uintptr_t p = lut[mem >> PSXM_SHIFT];
	if (p != INVALID_PTR_VAL) {
		*ret = (u8 *)(p + mem);
		return 1;
	}

	if (mem - 0x1f800000u < 0x1000u) {
		*ret = regs->ptrs.psxH + (mem - 0x1f800000u);
		return 1;
	}

	if (!write && mem - 0x1fc00000u < 0x80000u) {
		*ret = regs->ptrs.psxR + (mem - 0x1fc00000u);
		return 1;
	}

	return 0;
}

static inline int psxm_(u8 **ret, const psxRegisters *regs, u32 mem, int write)
{
	return psxm_lut(ret, regs, mem, write,
		write ? regs->ptrs.memWLUT : regs->ptrs.memRLUT);
}

static inline void * psxm_ptr(u32 mem, int write)
{
	u8 *ret;
	if (psxm_(&ret, &psxRegs, mem, write))
		return ret;
    return NULL;
}

#define PSXM(mem) psxm_ptr(mem, 0)
#define psxMu32(mem)      (*(u32 *)PSXM(mem))
#define psxMu32ref(mem)   (*(u32 *)PSXM(mem))
#define psxMu16(mem)      (*(u16 *)PSXM(mem))
#define psxMu16ref(mem)   (*(u16 *)PSXM(mem))
#define psxMu8(mem)       (*(u8  *)PSXM(mem))
#define psxMu8ref(mem)    (*(u8  *)PSXM(mem))
#define PSXMu32ref(mem)   (*(u32 *)PSXM(mem))
#define PSXMu16(mem)      (*(u16 *)PSXM(mem))
#define PSXMu8(mem)       (*(u8  *)PSXM(mem))

#define psxHu8(mem)       (*(u8  *)(psxRegs.ptrs.psxH + ((mem) & 0xffff)))
#define psxHu8ref(mem)    (*(u8  *)(psxRegs.ptrs.psxH + ((mem) & 0xffff)))
#define psxHu16(mem)      (*(u16 *)(psxRegs.ptrs.psxH + ((mem) & 0xffff)))
#define psxHu16ref(mem)   (*(u16 *)(psxRegs.ptrs.psxH + ((mem) & 0xffff)))
#define psxHu32(mem)      (*(u32 *)(psxRegs.ptrs.psxH + ((mem) & 0xffff)))
#define psxHu32ref(mem)   (*(u32 *)(psxRegs.ptrs.psxH + ((mem) & 0xffff)))

#define PSX_Ra0 ((u8 *)PSXM(psxRegs.GPR.n.r0)) // Fallback to safe name
#define PSX_Ra1 ((u8 *)PSXM(psxRegs.GPR.n.at))

extern int psxMemInit();
extern void psxMemReset();
extern void psxMemShutdown();
extern void psxMemOnIsolate(int enable);

extern u8 psxMemRead8(psxRegisters *regs, u32 addr);
extern u16 psxMemRead16(psxRegisters *regs, u32 addr);
extern u32 psxMemRead32(psxRegisters *regs, u32 addr);
extern void psxMemWrite8(psxRegisters *regs, u32 addr, u32 value);
extern void psxMemWrite16(psxRegisters *regs, u32 addr, u32 value);
extern void psxMemWrite32(psxRegisters *regs, u32 addr, u32 value);

#ifdef __cplusplus
}
#endif
#endif
