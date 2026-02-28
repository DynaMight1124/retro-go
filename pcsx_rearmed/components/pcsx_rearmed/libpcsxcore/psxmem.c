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

/*
 * PSX memory functions.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <esp_heap_caps.h>
#include <esp_heap_caps.h>
#include "psxcommon.h"
#include "psxmem_map.h"
#include "r3000a.h"
#include "psxbios.h"
#include "psxhw.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

static void *psxMapDefault(unsigned long addr, size_t size,
		enum psxMapTag tag, int *can_retry_addr)
{
	void *ptr;
#if !P_HAVE_MMAP
	*can_retry_addr = 0;
    size_t alignment = (tag == MAP_TAG_RAM) ? (512 * 1024) : 64;
	ptr = heap_caps_aligned_alloc(alignment, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
	return ptr ? ptr : MAP_FAILED;
#else
	int flags = MAP_PRIVATE | MAP_ANONYMOUS;

	*can_retry_addr = 1;
	ptr = mmap((void *)(uintptr_t)addr, size,
		    PROT_READ | PROT_WRITE, flags, -1, 0);
	return ptr;
#endif
}

static void psxUnmapDefault(void *ptr, size_t size, enum psxMapTag tag)
{
#if !P_HAVE_MMAP
	free(ptr);
#else
	munmap(ptr, size);
#endif
}

void *(*psxMapHook)(unsigned long addr, size_t size,
		enum psxMapTag tag, int *can_retry_addr) = psxMapDefault;
void (*psxUnmapHook)(void *ptr, size_t size,
		     enum psxMapTag tag) = psxUnmapDefault;

void *psxMap(unsigned long addr, size_t size, int is_fixed,
		enum psxMapTag tag)
{
	int try_, can_retry_addr = 0;
	void *ret = MAP_FAILED;

	for (try_ = 0; try_ < 3; try_++)
	{
		if (ret != MAP_FAILED)
			psxUnmap(ret, size, tag);
		ret = psxMapHook(addr, size, tag, &can_retry_addr);
		if (ret == MAP_FAILED)
			return MAP_FAILED;

		if (addr != 0 && ret != (void *)(uintptr_t)addr) {
			if (is_fixed) {
				psxUnmap(ret, size, tag);
				return MAP_FAILED;
			}
            // On ESP32 we accept any address the hook gives us
		}
		break;
	}

	return ret;
}

void psxUnmap(void *ptr, size_t size, enum psxMapTag tag)
{
	psxUnmapHook(ptr, size, tag);
}

static int psxMemInitMap(void)
{
	u8 *ptr;

	ptr = psxMap(0x80000000, 0x00280000, 1, MAP_TAG_RAM);
	if (ptr == MAP_FAILED)
		ptr = psxMap(0x77000000, 0x00280000, 0, MAP_TAG_RAM);
	if (ptr == MAP_FAILED) {
		return -1;
	}
	psxRegs.ptrs.psxM = ptr;
	psxRegs.ptrs.psxP = ptr + 0x200000;

	ptr = psxMap(0x1f800000, 0x80000, 0, MAP_TAG_OTHER);
	if (ptr == MAP_FAILED) {
		return -1;
	}
	psxRegs.ptrs.psxH = ptr;

	ptr = psxMap(0x1fc00000, 0x80000, 0, MAP_TAG_OTHER);
	if (ptr == MAP_FAILED) {
		return -1;
	}
	psxRegs.ptrs.psxR = ptr;

	return 0;
}

static void psxMemFreeMap(void)
{
	if (psxRegs.ptrs.psxM) psxUnmap(psxRegs.ptrs.psxM, 0x00280000, MAP_TAG_RAM);
	if (psxRegs.ptrs.psxH) psxUnmap(psxRegs.ptrs.psxH, 0x80000, MAP_TAG_OTHER);
	if (psxRegs.ptrs.psxR) psxUnmap(psxRegs.ptrs.psxR, 0x80000, MAP_TAG_OTHER);
	psxRegs.ptrs.psxM = psxRegs.ptrs.psxH = psxRegs.ptrs.psxR = NULL;
	psxRegs.ptrs.psxP = NULL;
}

static void lutMap(uintptr_t *lut, u8 *mem, u32 size, u32 start, u32 end)
{
	u32 i;
	for (i = start; i < end; i += (1u << PSXM_SHIFT))
		lut[i >> PSXM_SHIFT] = (uintptr_t)mem - (i & ~(size - 1));
}

static void mapRam(int isMapped)
{
	uintptr_t *wLUT = psxRegs.ptrs.memWLUT;

	if (isMapped) {
		u8 *ram = psxRegs.ptrs.psxM;
		u32 i;
		for (i = 0; i < 0x800000; i += 0x200000) {
			lutMap(wLUT, ram, 0x200000, 0x00000000u + i, 0x00200000u + i);
			lutMap(wLUT, ram, 0x200000, 0x80000000u + i, 0x80200000u + i);
			lutMap(wLUT, ram, 0x200000, 0xa0000000u + i, 0xa0200000u + i);
		}
	}
	else {
		size_t len = (0x200000 >> PSXM_SHIFT) * sizeof(wLUT[0]);
		memset(wLUT + (0x00000000 >> PSXM_SHIFT), INVALID_PTR_VAL, len);
		memset(wLUT + (0x80000000 >> PSXM_SHIFT), INVALID_PTR_VAL, len);
	}
}

int psxMemInit(void)
{
	size_t table_size = 1ul << (32 - PSXM_SHIFT);
	uintptr_t *memRLUT;
	uintptr_t *memWLUT;
	unsigned int i;
	int ret;

	ret = psxMemInitMap();
	if (ret) {
		psxMemShutdown();
		return -1;
	}

	if (DISABLE_MEM_LUTS)
		return 0;

#if defined(CONFIG_IDF_TARGET_ESP32P4)
	memRLUT = heap_caps_malloc(table_size * sizeof(memRLUT[0]) * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
    memRLUT = NULL; // Save DRAM on non-P4 targets
#endif

	if (memRLUT == NULL) {
		memRLUT = malloc(table_size * sizeof(memRLUT[0]) * 2);
	}
	if (memRLUT == NULL) {
		psxMemShutdown();
		return -1;
	}

	memset(memRLUT, INVALID_PTR_VAL, table_size * sizeof(memRLUT[0]) * 2);
	memWLUT = memRLUT + table_size;
	psxRegs.ptrs.memRLUT = memRLUT;
	psxRegs.ptrs.memWLUT = memWLUT;

	// ram
	for (i = 0; i < 0x800000; i += 0x200000) {
		u8 *ram = psxRegs.ptrs.psxM;
		lutMap(memRLUT, ram, 0x200000, 0x00000000u + i, 0x00200000u + i);
		lutMap(memRLUT, ram, 0x200000, 0x80000000u + i, 0x80200000u + i);
		lutMap(memRLUT, ram, 0x200000, 0xa0000000u + i, 0xa0200000u + i);
	}
	mapRam(1);

	// bios
	lutMap(memRLUT, psxRegs.ptrs.psxR, 0x80000, 0x1fc00000u, 0x1fc80000u);
	lutMap(memRLUT, psxRegs.ptrs.psxR, 0x80000, 0x9fc00000u, 0x9fc80000u);
	lutMap(memRLUT, psxRegs.ptrs.psxR, 0x80000, 0xbfc00000u, 0xbfc80000u);

	return 0;
}

void psxMemReset() {
	memset(psxRegs.ptrs.psxM, 0, 0x00200000);
	memset(psxRegs.ptrs.psxP, 0xff, 0x00010000);

	if (!DISABLE_MEM_LUTS)
		mapRam(1);

	if (strcmp(Config.Bios, "HLE") != 0) {
		char bios[1024];
		sprintf(bios, "%s/%s", Config.BiosDir, Config.Bios);
		FILE *f = fopen(bios, "rb");
		if (f) {
			fread(psxRegs.ptrs.psxR, 1, 0x80000, f);
			fclose(f);
		}
	}
}

void psxMemShutdown(void)
{
	psxMemFreeMap();
	if (psxRegs.ptrs.memRLUT) {
		free(psxRegs.ptrs.memRLUT);
		psxRegs.ptrs.memRLUT = NULL;
		psxRegs.ptrs.memWLUT = NULL;
	}
}

void psxMemOnIsolate(int enable) {
	psxCpu->Notify(enable ? R3000ACPU_NOTIFY_CACHE_ISOLATED
			: R3000ACPU_NOTIFY_CACHE_UNISOLATED, NULL);
}

u8 psxMemRead8(psxRegisters *regs, u32 mem) {
	u8 *p;
	if (psxm_(&p, regs, mem, 0)) return *p;
	return psxHwRead8(mem);
}

u16 psxMemRead16(psxRegisters *regs, u32 mem) {
	u8 *p;
	if (psxm_(&p, regs, mem, 0)) return SWAP16(*(u16 *)p);
	return psxHwRead16(mem);
}

u32 psxMemRead32(psxRegisters *regs, u32 mem) {
	u8 *p;
	if (psxm_(&p, regs, mem, 0)) return SWAP32(*(u32 *)p);
	return psxHwRead32(mem);
}

void psxMemWrite8(psxRegisters *regs, u32 mem, u32 value) {
	u8 *p;
	if (psxm_(&p, regs, mem, 1)) { *p = (u8)value; return; }
	psxHwWrite8(mem, value);
}

void psxMemWrite16(psxRegisters *regs, u32 mem, u32 value) {
	u8 *p;
	if (psxm_(&p, regs, mem, 1)) { *(u16 *)p = SWAP16((u16)value); return; }
	psxHwWrite16(mem, value);
}

void psxMemWrite32(psxRegisters *regs, u32 mem, u32 value) {
	u8 *p;
	if (psxm_(&p, regs, mem, 1)) { *(u32 *)p = SWAP32(value); return; }
	psxHwWrite32(mem, value);
}

#ifdef __cplusplus
}
#endif
