/*---------------------------------------------------------------------------
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. See also the license.txt file for
 *  additional informations.
 *---------------------------------------------------------------------------
 */

#include <rg_system.h>
#include <malloc.h>
#include <string.h>

#include "tlcs900h.h"
#include "graphics.h"
#include "neopopsound.h"
#include "types.h"
#include "input.h"
#include "flash.h"
#include "sound.h"
#include "cz80.h"
#include "koyote_bin.h"

#include "cz80_support.h"

unsigned char *mainram = NULL;
unsigned char *cpurom = NULL;
const unsigned char *mainrom = NULL;
unsigned char *cpuram = NULL;
unsigned char ldcRegs[64];

static unsigned char *s_cpuram_2208 = NULL;

size_t g_mainram_size = 0;
size_t g_cpurom_size = 0;

unsigned char realBIOSloaded = 0;

unsigned char (*z80MemReadB)(unsigned short addr);
unsigned short (*z80MemReadW)(unsigned short addr);
void (*z80MemWriteB)(unsigned short addr, unsigned char data);
void (*z80MemWriteW)(unsigned short addr, unsigned short data);
void (*z80PortWriteB)(unsigned char port, unsigned char data);
unsigned char (*z80PortReadB)(unsigned char port);

unsigned char z80ngpMemReadB(unsigned short addr)
{
    unsigned char temp;
    if (addr < 0x4000)
    {
        return mainram[0x3000 + addr];
    }
    switch (addr)
    {
    case 0x4000:
        break;
    case 0x4001:
        break;
    case 0x8000:
        temp = cpuram[0xBC];
        return temp;
    case 0xC000:
        break;
    }
    return 0x00;
}

unsigned short z80ngpMemReadW(unsigned short addr)
{
    return (z80ngpMemReadB(addr + 1) << 8) | z80ngpMemReadB(addr);
}

void z80ngpMemWriteB(unsigned short addr, unsigned char data)
{
    if (addr < 0x4000)
    {
        mainram[0x3000 + addr] = data;
        return;
    }
    switch (addr)
    {
    case 0x4000:
        Write_SoundChipNoise(data);
        return;
    case 0x4001:
        Write_SoundChipTone(data);
        return;
    case 0x8000:
        cpuram[0xBC] = data;
        return;
    case 0xC000:
        tlcs_interrupt_wrapper(0x03);
        return;
    }
}

void z80ngpMemWriteW(unsigned short addr, unsigned short data)
{
    if (addr < 0x4000)
    {
        mainram[0x3000 + addr] = data & 0xFF;
        mainram[0x3000 + addr + 1] = data >> 8;
        return;
    }
    switch (addr)
    {
    case 0x4000:
        Write_SoundChipNoise(data & 0xFF);
        Write_SoundChipNoise(data >> 8);
        return;
    case 0x4001:
        Write_SoundChipTone(data & 0xFF);
        Write_SoundChipTone(data >> 8);
        return;
    case 0x8000:
        cpuram[0xBC] = data & 0xFF;
        cpuram[0xBC] = data >> 8;
        return;
    case 0xC000:
        tlcs_interrupt_wrapper(0x03);
        tlcs_interrupt_wrapper(0x03);
        return;
    }
}

void z80ngpPortWriteB(unsigned char port, unsigned char data) {}
unsigned char z80ngpPortReadB(unsigned char port) { return 0xFF; }

#if defined(DRZ80) || defined(CZ80)
void DrZ80ngpMemWriteB(unsigned char data, unsigned short addr)
{
    if (addr < 0x4000)
    {
        mainram[0x3000 + addr] = data;
        return;
    }
    switch (addr)
    {
    case 0x4000:
        Write_SoundChipNoise(data);
        return;
    case 0x4001:
        Write_SoundChipTone(data);
        return;
    case 0x8000:
        cpuram[0xBC] = data;
        return;
    case 0xC000:
        tlcs_interrupt_wrapper(0x03);
        return;
    }
}

void DrZ80ngpMemWriteW(unsigned short data, unsigned short addr)
{
    if (addr < 0x4000)
    {
        mainram[0x3000 + addr] = data & 0xFF;
        mainram[0x3000 + addr + 1] = data >> 8;
        return;
    }
    switch (addr)
    {
    case 0x4000:
        Write_SoundChipNoise(data & 0xFF);
        Write_SoundChipNoise(data >> 8);
        return;
    case 0x4001:
        Write_SoundChipTone(data & 0xFF);
        Write_SoundChipTone(data >> 8);
        return;
    case 0x8000:
        cpuram[0xBC] = data & 0xFF;
        cpuram[0xBC] = data >> 8;
        return;
    case 0xC000:
        tlcs_interrupt_wrapper(0x03);
        tlcs_interrupt_wrapper(0x03);
        return;
    }
}

void DrZ80ngpPortWriteB(unsigned short port, unsigned char data) {}
unsigned char DrZ80ngpPortReadB(unsigned short port) { return 0xFF; }
#endif

/* Power-on values and HLE BIOS stubs from RACE. The previous ESP32 port
 * removed these while splitting the original static memory arena into
 * explicit allocations, leaving the CPU with no valid boot environment. */
static const unsigned char ngpcpuram[256] = {
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x08, 0xFF, 0xFF,
    0x34, 0x3C, 0xFF, 0xFF, 0xFF, 0x3F, 0x00, 0x00, 0x3F, 0xFF, 0x2D, 0x01, 0xFF, 0xFF, 0x03, 0xB2,
    0x80, 0x00, 0x01, 0x90, 0x03, 0xB0, 0x90, 0x62, 0x05, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x4C, 0x4C,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x20, 0xFF, 0x80, 0x7F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x20, 0x69, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x17, 0x03, 0x03, 0x02, 0x00, 0x00, 0x00,
    0x02, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const unsigned char ngpInterruptCode[] = {
    0x07,
    0xD1, 0xBA, 0x6F, 0x04, 0xD1, 0xB8, 0x6F, 0x04, 0x0E,
    0xD1, 0xBE, 0x6F, 0x04, 0xD1, 0xBC, 0x6F, 0x04, 0x0E,
    0xD1, 0xC2, 0x6F, 0x04, 0xD1, 0xC0, 0x6F, 0x04, 0x0E,
    0xD1, 0xC6, 0x6F, 0x04, 0xD1, 0xC4, 0x6F, 0x04, 0x0E,
    0xD1, 0xCA, 0x6F, 0x04, 0xD1, 0xC8, 0x6F, 0x04, 0x0E,
    0xD1, 0xCE, 0x6F, 0x04, 0xD1, 0xCC, 0x6F, 0x04, 0x0E,
    0xD1, 0xD2, 0x6F, 0x04, 0xD1, 0xD0, 0x6F, 0x04, 0x0E,
    0xD1, 0xD6, 0x6F, 0x04, 0xD1, 0xD4, 0x6F, 0x04, 0x0E,
    0xD1, 0xDA, 0x6F, 0x04, 0xD1, 0xD8, 0x6F, 0x04, 0x0E,
    0xD1, 0xDE, 0x6F, 0x04, 0xD1, 0xDC, 0x6F, 0x04, 0x0E,
    0xD1, 0xE2, 0x6F, 0x04, 0xD1, 0xE0, 0x6F, 0x04, 0x0E,
    0xD1, 0xE6, 0x6F, 0x04, 0xD1, 0xE4, 0x6F, 0x04, 0x0E,
    0xD1, 0xEA, 0x6F, 0x04, 0xD1, 0xE8, 0x6F, 0x04, 0x0E,
    0xD1, 0xEE, 0x6F, 0x04, 0xD1, 0xEC, 0x6F, 0x04, 0x0E,
    0xD1, 0xF2, 0x6F, 0x04, 0xD1, 0xF0, 0x6F, 0x04, 0x0E,
    0xD1, 0xF6, 0x6F, 0x04, 0xD1, 0xF4, 0x6F, 0x04, 0x0E,
    0xD1, 0xFA, 0x6F, 0x04, 0xD1, 0xF8, 0x6F, 0x04, 0x0E,
    0xD1, 0xFE, 0x6F, 0x04, 0xD1, 0xFC, 0x6F, 0x04, 0x0E,
};

static const unsigned int ngpVectors[0x21] = {
    0x00FFF800, 0x00FFF000, 0x00FFF800, 0x00FFF801,
    0x00FFF80A, 0x00FFF813, 0x00FFF81C, 0x00FFF800,
    0x00FFF800, 0x00FFF800, 0x00FFF825, 0x00FFF82E,
    0x00FFF837, 0x00FFF800, 0x00FFF800, 0x00FFF800,
    0x00FFF840, 0x00FFF849, 0x00FFF852, 0x00FFF85B,
    0x00FFF800, 0x00FFF800, 0x00FFF800, 0x00FFF800,
    0x00FFF864, 0x00FFF86D, 0x00FFF800, 0x00FFF800,
    0x00FFF800, 0x00FFF87F, 0x00FFF888, 0x00FFF891,
    0x00FFF89A,
};

static void write_le32(unsigned char *dst, unsigned int value)
{
    dst[0] = (unsigned char)value;
    dst[1] = (unsigned char)(value >> 8);
    dst[2] = (unsigned char)(value >> 16);
    dst[3] = (unsigned char)(value >> 24);
}

bool ngp_mem_alloc_init(size_t mainram_sz, size_t cpurom_sz)
{
    if (!mainram)
    {
        mainram = (unsigned char *)rg_alloc(mainram_sz, MEM_FAST);
        if (!mainram)
            return false;
        g_mainram_size = mainram_sz;
        memset(mainram, 0, g_mainram_size);
    }
    if (!cpurom)
    {
        cpurom = (unsigned char *)rg_alloc(cpurom_sz, MEM_SLOW);
        if (!cpurom)
            return false;
        g_cpurom_size = cpurom_sz;
        memset(cpurom, 0, g_cpurom_size);
    }
    memset(ldcRegs, 0, 64);
    if (!s_cpuram_2208)
    {
        s_cpuram_2208 = (unsigned char *)rg_alloc(2208, MEM_FAST);
        if (!s_cpuram_2208)
            return false;
        memset(s_cpuram_2208, 0, 2208);
    }
    if (!cpuram)
        cpuram = s_cpuram_2208;
    return true;
}

void ngp_mem_free(void)
{
    if (mainram)
    {
        free(mainram);
        mainram = NULL;
        g_mainram_size = 0;
    }
    if (cpurom)
    {
        free(cpurom);
        cpurom = NULL;
        g_cpurom_size = 0;
    }
    if (s_cpuram_2208)
    {
        free(s_cpuram_2208);
        s_cpuram_2208 = NULL;
    }
    cpuram = NULL;
}

void ngp_mem_init(void)
{
    unsigned int i;
    int x;

    if (!mainram || !cpurom || !s_cpuram_2208)
    {
        if (!ngp_mem_alloc_init(64 * 1024, 0x10000))
            return;
    }

    if (g_mainram_size < 0x7000 || g_cpurom_size < 0x10000)
    {
        rg_system_panic("NGP MEM", "Allocated memory is too small for RACE");
        return;
    }

    cpuram = s_cpuram_2208;
    memset(mainram, 0, g_mainram_size);
    memset(cpuram, 0, 2208);
    memset(ldcRegs, 0, sizeof(ldcRegs));
    realBIOSloaded = 0;
    memset(cpurom, 0, g_cpurom_size);

    /* Build the high-level BIOS call table at FF:E000 and its pointer table
     * at FF:FE00. Opcode C8 1A invokes doBios() in the TLCS core. */
    for (i = 0; i < 0x40; ++i)
    {
        const unsigned int stub = 0xE000 + 0x40 * i;
        cpurom[stub + 0] = 0xC8;
        cpurom[stub + 1] = 0x1A;
        cpurom[stub + 2] = (unsigned char)i;
        cpurom[stub + 3] = 0x0E;
        write_le32(&cpurom[0xFE00 + 4 * i], 0x00FFE000 + 0x40 * i);
    }

    /* SWI 1 dispatcher and vector. */
    x = 0xF000;
    cpurom[x++] = 0x17; cpurom[x++] = 0x03;
    cpurom[x++] = 0x3C;
    cpurom[x++] = 0xC8; cpurom[x++] = 0xCC; cpurom[x++] = 0x1F;
    cpurom[x++] = 0xC8; cpurom[x++] = 0x80;
    cpurom[x++] = 0xC8; cpurom[x++] = 0x80;
    cpurom[x++] = 0x44; cpurom[x++] = 0x00;
    cpurom[x++] = 0xFE; cpurom[x++] = 0xFF; cpurom[x++] = 0x00;
    cpurom[x++] = 0xE3; cpurom[x++] = 0x03;
    cpurom[x++] = 0xF0; cpurom[x++] = 0xE1; cpurom[x++] = 0x24;
    cpurom[x++] = 0xB4; cpurom[x++] = 0xE8; cpurom[x++] = 0x5C;
    cpurom[x++] = 0x07;
    write_le32(&cpurom[0xFF04], 0x00FFF000);

    memcpy(&cpurom[0xF800], ngpInterruptCode, sizeof(ngpInterruptCode));
    for (i = 0; i < sizeof(ngpVectors) / sizeof(ngpVectors[0]); ++i)
        write_le32(&cpurom[0xFF00 + 4 * i], ngpVectors[i]);

    memcpy(cpuram, ngpcpuram, sizeof(ngpcpuram));
    memcpy(mainram, koyote_bin, KOYOTE_BIN_SIZE);

    /* Default RAM interrupt vectors and BIOS-visible power-on state. */
    for (i = 0; i < 18; ++i)
        write_le32(&mainram[0x2FB8 + 4 * i], 0x00FFF800);

    mainram[0x2F80] = 0xFF;
    mainram[0x2F81] = 0x03;
    mainram[0x2F95] = 0x10;
    mainram[0x2F91] = 0x10;
    mainram[0x2F84] = 0x40;
    mainram[0x2F85] = 0x00;
    mainram[0x2F86] = 0x00;
    mainram[0x2F87] = 0x01;

    /* VDP power-on defaults. These indexes correspond to CPU addresses
     * 0x8000 onward in the RACE address map. */
    mainram[0x4000] = 0xC0;
    mainram[0x4004] = 0xFF;
    mainram[0x4005] = 0xFF;
    mainram[0x4006] = 0xC6;
    for (i = 0; i < 5; ++i)
    {
        mainram[0x4101 + 4 * i] = 0x07;
        mainram[0x4102 + 4 * i] = 0x07;
        mainram[0x4103 + 4 * i] = 0x07;
    }
    mainram[0x4118] = 0x07;
    mainram[0x43E0] = mainram[0x43F0] = 0xFF;
    mainram[0x43E1] = mainram[0x43F1] = 0x0F;
    mainram[0x43E2] = mainram[0x43F2] = 0xDD;
    mainram[0x43E3] = mainram[0x43F3] = 0x0D;
    mainram[0x43E4] = mainram[0x43F4] = 0xBB;
    mainram[0x43E5] = mainram[0x43F5] = 0x0B;
    mainram[0x43E6] = mainram[0x43F6] = 0x99;
    mainram[0x43E7] = mainram[0x43F7] = 0x09;
    mainram[0x43E8] = mainram[0x43F8] = 0x77;
    mainram[0x43E9] = mainram[0x43F9] = 0x07;
    mainram[0x43EA] = mainram[0x43FA] = 0x44;
    mainram[0x43EB] = mainram[0x43FB] = 0x04;
    mainram[0x43EC] = mainram[0x43FC] = 0x33;
    mainram[0x43ED] = mainram[0x43FD] = 0x03;
    mainram[0x43EE] = mainram[0x43FE] = 0x00;
    mainram[0x43EF] = mainram[0x43FF] = 0x00;

    z80MemReadB = z80ngpMemReadB;
    z80MemReadW = z80ngpMemReadW;
    z80MemWriteB = z80ngpMemWriteB;
    z80MemWriteW = z80ngpMemWriteW;
    z80PortWriteB = z80ngpPortWriteB;
    z80PortReadB = z80ngpPortReadB;
}

bool ngp_mem_set_rom(const void *rom_base, size_t rom_len)
{
    if (!rom_base || rom_len == 0)
        return false;
    mainrom = (const unsigned char *)rom_base;
    return true;
}
