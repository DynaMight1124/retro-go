#ifndef RG_MAME_CORE_H
#define RG_MAME_CORE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "retrogo/rg_psram.h"
#include "driver.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include "driver.h"
#include "cpuintrf.h"
#include "mame.h"
#include "f68000.h"

#ifndef NEOMAME
extern const struct GameDriver driver_indytemp;
extern const struct GameDriver driver_indytem2;
extern const struct GameDriver driver_indytem3;
extern const struct GameDriver driver_indytem4;
extern const struct GameDriver driver_gauntlet; 
extern const struct GameDriver driver_gauntir1;
extern const struct GameDriver driver_gauntir2;
extern const struct GameDriver driver_gaunt2p;
extern const struct GameDriver driver_gaunt2;
extern const struct GameDriver driver_vindctr2;
extern const struct GameDriver driver_cyberbal; // +1M68K
extern const struct GameDriver driver_cyberba2; // +1M68K
extern const struct GameDriver driver_cyberbt; // +1M68K
extern const struct GameDriver driver_xybots;
extern const struct GameDriver driver_hydra;
extern const struct GameDriver driver_hydrap;
extern const struct GameDriver driver_pitfight;
extern const struct GameDriver driver_pitfigh3;
extern const struct GameDriver driver_rampart;
extern const struct GameDriver driver_ramprt2p;
extern const struct GameDriver driver_rampartj;
extern const struct GameDriver driver_arcadecr;
extern const struct GameDriver driver_marble;
extern const struct GameDriver driver_marble2;
extern const struct GameDriver driver_marblea;
extern const struct GameDriver driver_peterpak;
extern const struct GameDriver driver_roadrunn;
extern const struct GameDriver driver_roadblst; // works with musashi?
#endif

#define FAME_N_ENTRIES 64

//#define DEBUG_FAME
//#define DEBUG_FAME_FULL_START 60
//#define DEBUG_FAME_FULL_STOP 72
//#define FAME_ONLY_HANDLER
//#define FAME_ONE_HANDLER

typedef struct {
	M68K_CONTEXT regs;
} Fame_Regs;

static Fame_Regs fame;

int nofame=0;

M68K_PROGRAM prg[FAME_N_ENTRIES+1];
M68K_DATA dt_read8[FAME_N_ENTRIES+1];
M68K_DATA dt_write8[FAME_N_ENTRIES+1];
M68K_DATA dt_read16[FAME_N_ENTRIES+1];
M68K_DATA dt_write16[FAME_N_ENTRIES+1];

#define M68KCONTEXT famem68kcontext
extern M68K_CONTEXT M68KCONTEXT;

#define HT_BANK1  1
#define HT_BANKMAX (HT_BANK1 + MAX_BANKS - 1)
#if LSB_FIRST
#define BYTE_XOR_BE(a) ((a) ^ 1)
#else
#define BYTE_XOR_BE(a) (a)
#endif

extern MHELE readhardware[];
extern MHELE writehardware[];
extern data_t (*memoryreadhandler[MH_HARDMAX])(offs_t address);
extern int memoryreadoffset[MH_HARDMAX];
extern void (*memorywritehandler[MH_HARDMAX])(offs_t address,data_t data);
extern int memorywriteoffset[MH_HARDMAX];

extern data_t cpu_readmem32(offs_t offset);
extern data_t cpu_readmem32_word(offs_t offset);
extern void cpu_writemem32(offs_t offset,data_t data);
extern void cpu_writemem32_word(offs_t offset,data_t data);
extern void cpu_setOPbase24(int pc);

typedef int (*miread8_func)(unsigned int);

unsigned miread8_offs[FAME_N_ENTRIES];
miread8_func miread8_funcs[FAME_N_ENTRIES];

#define MIREAD8_FUNC(NAME) \
static int miread8_##NAME (unsigned int address) \
{ \
	/* int data, shift = ((address ^ 1) & 1) << 3; */ \
	/* address &= ~1; */ \
	/* data = miread8_funcs[(NAME)](address - miread8_offs[(NAME)]); */ \
	/* return (data >> shift) & 0xff; */ \
	return ((miread8_funcs[(NAME)])((address & ~1) - miread8_offs[(NAME)]) >> (((address & 1) << 3) ^ 8)) & 0xff; \
} 

MIREAD8_FUNC(0)
MIREAD8_FUNC(1)
MIREAD8_FUNC(2)
MIREAD8_FUNC(3)
MIREAD8_FUNC(4)
MIREAD8_FUNC(5)
MIREAD8_FUNC(6)
MIREAD8_FUNC(7)
MIREAD8_FUNC(8)
MIREAD8_FUNC(9)
MIREAD8_FUNC(10)
MIREAD8_FUNC(11)
MIREAD8_FUNC(12)
MIREAD8_FUNC(13)
MIREAD8_FUNC(14)
MIREAD8_FUNC(15)
#if FAME_N_ENTRIES > 16
MIREAD8_FUNC(16)
MIREAD8_FUNC(17)
MIREAD8_FUNC(18)
MIREAD8_FUNC(19)
MIREAD8_FUNC(20)
MIREAD8_FUNC(21)
MIREAD8_FUNC(22)
MIREAD8_FUNC(23)
#if FAME_N_ENTRIES > 24
MIREAD8_FUNC(24)
MIREAD8_FUNC(25)
MIREAD8_FUNC(26)
MIREAD8_FUNC(27)
MIREAD8_FUNC(28)
MIREAD8_FUNC(29)
MIREAD8_FUNC(30)
MIREAD8_FUNC(31)
MIREAD8_FUNC(32)
#if FAME_N_ENTRIES > 32
MIREAD8_FUNC(33)
MIREAD8_FUNC(34)
MIREAD8_FUNC(35)
MIREAD8_FUNC(36)
MIREAD8_FUNC(37)
MIREAD8_FUNC(38)
MIREAD8_FUNC(39)
#if FAME_N_ENTRIES > 40
MIREAD8_FUNC(40)
MIREAD8_FUNC(41)
MIREAD8_FUNC(42)
MIREAD8_FUNC(43)
MIREAD8_FUNC(44)
MIREAD8_FUNC(45)
MIREAD8_FUNC(46)
MIREAD8_FUNC(47)
#if FAME_N_ENTRIES > 48
MIREAD8_FUNC(48)
MIREAD8_FUNC(49)
MIREAD8_FUNC(50)
MIREAD8_FUNC(51)
MIREAD8_FUNC(52)
MIREAD8_FUNC(53)
MIREAD8_FUNC(54)
MIREAD8_FUNC(55)
#if FAME_N_ENTRIES > 56
MIREAD8_FUNC(56)
MIREAD8_FUNC(57)
MIREAD8_FUNC(58)
MIREAD8_FUNC(59)
MIREAD8_FUNC(60)
MIREAD8_FUNC(61)
MIREAD8_FUNC(62)
MIREAD8_FUNC(63)
#endif
#endif
#endif
#endif
#endif
#endif

miread8_func miread8_funcs_indirect[]= {
	miread8_0, miread8_1, miread8_2, miread8_3, miread8_4, miread8_5, miread8_6, miread8_7,
	miread8_8, miread8_9, miread8_10, miread8_11, miread8_12, miread8_13, miread8_14, miread8_15,
#if FAME_N_ENTRIES > 16
	miread8_16, miread8_17, miread8_18, miread8_19, miread8_20, miread8_21, miread8_22, miread8_23,
#if FAME_N_ENTRIES > 24
	miread8_24, miread8_25, miread8_26, miread8_27, miread8_28, miread8_29, miread8_30, miread8_31,
#if FAME_N_ENTRIES > 32
	miread8_32, miread8_33, miread8_34, miread8_35, miread8_36, miread8_37, miread8_38, miread8_39,
#if FAME_N_ENTRIES > 40
	miread8_40, miread8_41, miread8_42, miread8_43, miread8_44, miread8_45, miread8_46, miread8_47,
#if FAME_N_ENTRIES > 48
	miread8_48, miread8_49, miread8_50, miread8_51, miread8_52, miread8_53, miread8_54, miread8_55,
#if FAME_N_ENTRIES > 56
	miread8_56, miread8_57, miread8_58, miread8_59, miread8_60, miread8_61, miread8_62, miread8_63
#endif
#endif
#endif
#endif
#endif
#endif
};


typedef int (*miread16_func)(unsigned int);

unsigned miread16_offs[FAME_N_ENTRIES];
miread16_func miread16_funcs[FAME_N_ENTRIES];

#define MIREAD16_FUNC(NAME) \
static int miread16_##NAME (unsigned int address) \
{ \
	return miread16_funcs[(NAME)](address - miread16_offs[(NAME)]); \
} 

MIREAD16_FUNC(0)
MIREAD16_FUNC(1)
MIREAD16_FUNC(2)
MIREAD16_FUNC(3)
MIREAD16_FUNC(4)
MIREAD16_FUNC(5)
MIREAD16_FUNC(6)
MIREAD16_FUNC(7)
MIREAD16_FUNC(8)
MIREAD16_FUNC(9)
MIREAD16_FUNC(10)
MIREAD16_FUNC(11)
MIREAD16_FUNC(12)
MIREAD16_FUNC(13)
MIREAD16_FUNC(14)
MIREAD16_FUNC(15)
#if FAME_N_ENTRIES > 16
MIREAD16_FUNC(16)
MIREAD16_FUNC(17)
MIREAD16_FUNC(18)
MIREAD16_FUNC(19)
MIREAD16_FUNC(20)
MIREAD16_FUNC(21)
MIREAD16_FUNC(22)
MIREAD16_FUNC(23)
#if FAME_N_ENTRIES > 24
MIREAD16_FUNC(24)
MIREAD16_FUNC(25)
MIREAD16_FUNC(26)
MIREAD16_FUNC(27)
MIREAD16_FUNC(28)
MIREAD16_FUNC(29)
MIREAD16_FUNC(30)
MIREAD16_FUNC(31)
#if FAME_N_ENTRIES > 32
MIREAD16_FUNC(32)
MIREAD16_FUNC(33)
MIREAD16_FUNC(34)
MIREAD16_FUNC(35)
MIREAD16_FUNC(36)
MIREAD16_FUNC(37)
MIREAD16_FUNC(38)
MIREAD16_FUNC(39)
#if FAME_N_ENTRIES > 40
MIREAD16_FUNC(40)
MIREAD16_FUNC(41)
MIREAD16_FUNC(42)
MIREAD16_FUNC(43)
MIREAD16_FUNC(44)
MIREAD16_FUNC(45)
MIREAD16_FUNC(46)
MIREAD16_FUNC(47)
#if FAME_N_ENTRIES > 48
MIREAD16_FUNC(48)
MIREAD16_FUNC(49)
MIREAD16_FUNC(50)
MIREAD16_FUNC(51)
MIREAD16_FUNC(52)
MIREAD16_FUNC(53)
MIREAD16_FUNC(54)
MIREAD16_FUNC(55)
#if FAME_N_ENTRIES > 56
MIREAD16_FUNC(56)
MIREAD16_FUNC(57)
MIREAD16_FUNC(58)
MIREAD16_FUNC(59)
MIREAD16_FUNC(60)
MIREAD16_FUNC(61)
MIREAD16_FUNC(62)
MIREAD16_FUNC(63)
#endif
#endif
#endif
#endif
#endif
#endif

miread16_func miread16_funcs_indirect[]= {
	miread16_0, miread16_1, miread16_2, miread16_3, miread16_4, miread16_5, miread16_6, miread16_7,
	miread16_8, miread16_9, miread16_10, miread16_11, miread16_12, miread16_13, miread16_14, miread16_15,
#if FAME_N_ENTRIES > 16
	miread16_16, miread16_17, miread16_18, miread16_19, miread16_20, miread16_21, miread16_22, miread16_23,
#if FAME_N_ENTRIES > 24
	miread16_24, miread16_25, miread16_26, miread16_27, miread16_28, miread16_29, miread16_30, miread16_31,
#if FAME_N_ENTRIES > 32
	miread16_32, miread16_33, miread16_34, miread16_35, miread16_36, miread16_37, miread16_38, miread16_39,
#if FAME_N_ENTRIES > 40
	miread16_40, miread16_41, miread16_42, miread16_43, miread16_44, miread16_45, miread16_46, miread16_47,
#if FAME_N_ENTRIES > 48
	miread16_48, miread16_49, miread16_50, miread16_51, miread16_52, miread16_53, miread16_54, miread16_55,
#if FAME_N_ENTRIES > 56
	miread16_56, miread16_57, miread16_58, miread16_59, miread16_60, miread16_61, miread16_62, miread16_63,
#endif
#endif
#endif
#endif
#endif
#endif
};


typedef void (*miwrite8_func)(unsigned int, int);

unsigned miwrite8_offs[FAME_N_ENTRIES];
miwrite8_func miwrite8_funcs[FAME_N_ENTRIES];

#define MIWRITE8_FUNC(NAME) \
static void miwrite8_##NAME (unsigned int address, int data) \
{ \
	/* int shift = ((address ^ 1) & 1) << 3; */ \
	/* address &= ~1; */ \
	/* miwrite8_funcs[(NAME)](address - miwrite8_offs[(NAME)],(0xff000000 >> shift) | ((data & 0xff) << shift)); */ \
	int shift = ((address & 1) << 3)^8;  \
	data = (0xff000000 >> shift) | ((data & 0xff) << shift); \
	address &= ~1; \
	miwrite8_funcs[(NAME)](address - miwrite8_offs[(NAME)], data); \
} 

MIWRITE8_FUNC(0)
MIWRITE8_FUNC(1)
MIWRITE8_FUNC(2)
MIWRITE8_FUNC(3)
MIWRITE8_FUNC(4)
MIWRITE8_FUNC(5)
MIWRITE8_FUNC(6)
MIWRITE8_FUNC(7)
MIWRITE8_FUNC(8)
MIWRITE8_FUNC(9)
MIWRITE8_FUNC(10)
MIWRITE8_FUNC(11)
MIWRITE8_FUNC(12)
MIWRITE8_FUNC(13)
MIWRITE8_FUNC(14)
MIWRITE8_FUNC(15)
#if FAME_N_ENTRIES > 16
MIWRITE8_FUNC(16)
MIWRITE8_FUNC(17)
MIWRITE8_FUNC(18)
MIWRITE8_FUNC(19)
MIWRITE8_FUNC(20)
MIWRITE8_FUNC(21)
MIWRITE8_FUNC(22)
MIWRITE8_FUNC(23)
#if FAME_N_ENTRIES > 24
MIWRITE8_FUNC(24)
MIWRITE8_FUNC(25)
MIWRITE8_FUNC(26)
MIWRITE8_FUNC(27)
MIWRITE8_FUNC(28)
MIWRITE8_FUNC(29)
MIWRITE8_FUNC(30)
MIWRITE8_FUNC(31)
#if FAME_N_ENTRIES > 32
MIWRITE8_FUNC(32)
MIWRITE8_FUNC(33)
MIWRITE8_FUNC(34)
MIWRITE8_FUNC(35)
MIWRITE8_FUNC(36)
MIWRITE8_FUNC(37)
MIWRITE8_FUNC(38)
MIWRITE8_FUNC(39)
#if FAME_N_ENTRIES > 40
MIWRITE8_FUNC(40)
MIWRITE8_FUNC(41)
MIWRITE8_FUNC(42)
MIWRITE8_FUNC(43)
MIWRITE8_FUNC(44)
MIWRITE8_FUNC(45)
MIWRITE8_FUNC(46)
MIWRITE8_FUNC(47)
#if FAME_N_ENTRIES > 48
MIWRITE8_FUNC(48)
MIWRITE8_FUNC(49)
MIWRITE8_FUNC(50)
MIWRITE8_FUNC(51)
MIWRITE8_FUNC(52)
MIWRITE8_FUNC(53)
MIWRITE8_FUNC(54)
MIWRITE8_FUNC(55)
#if FAME_N_ENTRIES > 56
MIWRITE8_FUNC(56)
MIWRITE8_FUNC(57)
MIWRITE8_FUNC(58)
MIWRITE8_FUNC(59)
MIWRITE8_FUNC(60)
MIWRITE8_FUNC(61)
MIWRITE8_FUNC(62)
MIWRITE8_FUNC(63)
#endif
#endif
#endif
#endif
#endif
#endif

miwrite8_func miwrite8_funcs_indirect[]= {
	miwrite8_0, miwrite8_1, miwrite8_2, miwrite8_3, miwrite8_4, miwrite8_5, miwrite8_6, miwrite8_7,
	miwrite8_8, miwrite8_9, miwrite8_10, miwrite8_11, miwrite8_12, miwrite8_13, miwrite8_14, miwrite8_15,
#if FAME_N_ENTRIES > 16
	miwrite8_16, miwrite8_17, miwrite8_18, miwrite8_19, miwrite8_20, miwrite8_21, miwrite8_22, miwrite8_23,
#if FAME_N_ENTRIES > 24
	miwrite8_24, miwrite8_25, miwrite8_26, miwrite8_27, miwrite8_28, miwrite8_29, miwrite8_30, miwrite8_31,
#if FAME_N_ENTRIES > 32
	miwrite8_32, miwrite8_33, miwrite8_34, miwrite8_35, miwrite8_36, miwrite8_37, miwrite8_38, miwrite8_39,
#if FAME_N_ENTRIES > 40
	miwrite8_40, miwrite8_41, miwrite8_42, miwrite8_43, miwrite8_44, miwrite8_45, miwrite8_46, miwrite8_47,
#if FAME_N_ENTRIES > 48
	miwrite8_48, miwrite8_49, miwrite8_50, miwrite8_51, miwrite8_52, miwrite8_53, miwrite8_54, miwrite8_55,
#if FAME_N_ENTRIES > 56
	miwrite8_56, miwrite8_57, miwrite8_58, miwrite8_59, miwrite8_60, miwrite8_61, miwrite8_62, miwrite8_63,
#endif
#endif
#endif
#endif
#endif
#endif
};


typedef void (*miwrite16_func)(unsigned int, int);

unsigned miwrite16_offs[FAME_N_ENTRIES];
miwrite16_func miwrite16_funcs[FAME_N_ENTRIES];

#define MIWRITE16_FUNC(NAME) \
static void miwrite16_##NAME (unsigned int address, int data) \
{ \
	miwrite16_funcs[(NAME)](address - miwrite16_offs[(NAME)], data&0xffff ); \
} 

MIWRITE16_FUNC(0)
MIWRITE16_FUNC(1)
MIWRITE16_FUNC(2)
MIWRITE16_FUNC(3)
MIWRITE16_FUNC(4)
MIWRITE16_FUNC(5)
MIWRITE16_FUNC(6)
MIWRITE16_FUNC(7)
MIWRITE16_FUNC(8)
MIWRITE16_FUNC(9)
MIWRITE16_FUNC(10)
MIWRITE16_FUNC(11)
MIWRITE16_FUNC(12)
MIWRITE16_FUNC(13)
MIWRITE16_FUNC(14)
MIWRITE16_FUNC(15)
#if FAME_N_ENTRIES > 16
MIWRITE16_FUNC(16)
MIWRITE16_FUNC(17)
MIWRITE16_FUNC(18)
MIWRITE16_FUNC(19)
MIWRITE16_FUNC(20)
MIWRITE16_FUNC(21)
MIWRITE16_FUNC(22)
MIWRITE16_FUNC(23)
#if FAME_N_ENTRIES > 24
MIWRITE16_FUNC(24)
MIWRITE16_FUNC(25)
MIWRITE16_FUNC(26)
MIWRITE16_FUNC(27)
MIWRITE16_FUNC(28)
MIWRITE16_FUNC(29)
MIWRITE16_FUNC(30)
MIWRITE16_FUNC(31)
#if FAME_N_ENTRIES > 32
MIWRITE16_FUNC(32)
MIWRITE16_FUNC(33)
MIWRITE16_FUNC(34)
MIWRITE16_FUNC(35)
MIWRITE16_FUNC(36)
MIWRITE16_FUNC(37)
MIWRITE16_FUNC(38)
MIWRITE16_FUNC(39)
#if FAME_N_ENTRIES > 40
MIWRITE16_FUNC(40)
MIWRITE16_FUNC(41)
MIWRITE16_FUNC(42)
MIWRITE16_FUNC(43)
MIWRITE16_FUNC(44)
MIWRITE16_FUNC(45)
MIWRITE16_FUNC(46)
MIWRITE16_FUNC(47)
#if FAME_N_ENTRIES > 48
MIWRITE16_FUNC(48)
MIWRITE16_FUNC(49)
MIWRITE16_FUNC(50)
MIWRITE16_FUNC(51)
MIWRITE16_FUNC(52)
MIWRITE16_FUNC(53)
MIWRITE16_FUNC(54)
MIWRITE16_FUNC(55)
#if FAME_N_ENTRIES > 56
MIWRITE16_FUNC(56)
MIWRITE16_FUNC(57)
MIWRITE16_FUNC(58)
MIWRITE16_FUNC(59)
MIWRITE16_FUNC(60)
MIWRITE16_FUNC(61)
MIWRITE16_FUNC(62)
MIWRITE16_FUNC(63)
#endif
#endif
#endif
#endif
#endif
#endif

miwrite16_func miwrite16_funcs_indirect[]= {
	miwrite16_0, miwrite16_1, miwrite16_2, miwrite16_3, miwrite16_4, miwrite16_5, miwrite16_6, miwrite16_7,
	miwrite16_8, miwrite16_9, miwrite16_10, miwrite16_11, miwrite16_12, miwrite16_13, miwrite16_14, miwrite16_15,
#if FAME_N_ENTRIES > 16
	miwrite16_16, miwrite16_17, miwrite16_18, miwrite16_19, miwrite16_20, miwrite16_21, miwrite16_22, miwrite16_23,
#if FAME_N_ENTRIES > 24
	miwrite16_24, miwrite16_25, miwrite16_26, miwrite16_27, miwrite16_28, miwrite16_29, miwrite16_30, miwrite16_31,
#if FAME_N_ENTRIES > 32
	miwrite16_32, miwrite16_33, miwrite16_34, miwrite16_35, miwrite16_36, miwrite16_37, miwrite16_38, miwrite16_39,
#if FAME_N_ENTRIES > 40
	miwrite16_40, miwrite16_41, miwrite16_42, miwrite16_43, miwrite16_44, miwrite16_45, miwrite16_46, miwrite16_47,
#if FAME_N_ENTRIES > 48
	miwrite16_48, miwrite16_49, miwrite16_50, miwrite16_51, miwrite16_52, miwrite16_53, miwrite16_54, miwrite16_55,
#if FAME_N_ENTRIES > 56
	miwrite16_56, miwrite16_57, miwrite16_58, miwrite16_59, miwrite16_60, miwrite16_61, miwrite16_62, miwrite16_63,
#endif
#endif
#endif
#endif
#endif
#endif
};

static int get_cpu_readmem32 (int address, unsigned **punt)
{
	MHELE hw = cur_mrhard[(unsigned)address >> (ABITS2_32 + ABITS_MIN_32)];
	if (hw <= HT_BANKMAX)
	{
		*punt=(unsigned *)(&cpu_bankbase[hw][0 - memoryreadoffset[hw]]);
		return 0;
	}
	if (hw >= MH_HARDMAX)
	{
		hw -= MH_HARDMAX;
		hw = readhardware[(hw << MH_SBITS) + (((unsigned)address >> ABITS_MIN_32) & MHMASK(ABITS2_32))];
		if (hw <= HT_BANKMAX)
		{
			*punt=(unsigned *)(&cpu_bankbase[hw][0 - memoryreadoffset[hw]]);
			return 0;
		}
	}
	*punt=(unsigned *)memoryreadhandler[hw];
	return 1;
}

static int get_cpu_writemem32 (int address, unsigned **punt)
{
	MHELE hw = cur_mwhard[(unsigned)address >> (ABITS2_32 + ABITS_MIN_32)];
	if (hw <= HT_BANKMAX)
	{
		*punt=(unsigned *)(&cpu_bankbase[hw][0 - memorywriteoffset[hw]]);
		return 0;
	}
	if (hw >= MH_HARDMAX)
	{
		hw -= MH_HARDMAX;
		hw = writehardware[(hw << MH_SBITS) + (((unsigned)address >> ABITS_MIN_32) & MHMASK(ABITS2_32))];
		if (hw <= HT_BANKMAX)
		{
			*punt=(unsigned *)(&cpu_bankbase[hw][0 - memorywriteoffset[hw]]);
			return 0;
		}
	}

	*punt=(unsigned *)memorywritehandler[hw];
	return 1;
}


static int get_cpu_readoffset(int address)
{
	MHELE hw = cur_mrhard[(unsigned)address >> (ABITS2_32 + ABITS_MIN_32)];
	if (hw >= MH_HARDMAX)
	{
		MHELE back=hw;
		hw -= MH_HARDMAX;
		hw = readhardware[(hw << MH_SBITS) + (((unsigned)address >> ABITS_MIN_32) & MHMASK(ABITS2_32))];
		if (hw <= HT_BANKMAX)
			hw=back;
	}
	return (int)memoryreadoffset[hw];
}

static int get_cpu_writeoffset(int address)
{
	MHELE hw = cur_mwhard[(unsigned)address >> (ABITS2_32 + ABITS_MIN_32)];
	if (hw >= MH_HARDMAX)
	{
		MHELE back = hw;
		hw -= MH_HARDMAX;
		hw = writehardware[(hw << MH_SBITS) + (((unsigned)address >> ABITS_MIN_32) & MHMASK(ABITS2_32))];
		if (hw <= HT_BANKMAX)
			hw = back;
	}
	return (int)memorywriteoffset[hw];
}


static unsigned get_data_read (int address)
{
	MHELE hw = cur_mrhard[(unsigned)address >> (ABITS2_32 + ABITS_MIN_32)];
	if (hw <= HT_BANKMAX)
		return (unsigned)(&cpu_bankbase[hw][address - memoryreadoffset[hw]]);
	if (hw >= MH_HARDMAX)
	{
		hw -= MH_HARDMAX;
		hw = readhardware[(hw << MH_SBITS) + (((unsigned)address >> ABITS_MIN_32) & MHMASK(ABITS2_32))];
		if (hw <= HT_BANKMAX)
			return (unsigned)(&cpu_bankbase[hw][address - memoryreadoffset[hw]]);
	}
	return 0xFFFFFFFF;
}

static unsigned get_data_write (int address)
{
	MHELE hw = cur_mwhard[(unsigned)address >> (ABITS2_32 + ABITS_MIN_32)];
	if (hw <= HT_BANKMAX)
		return (unsigned)(&cpu_bankbase[hw][address - memorywriteoffset[hw]]);
	if (hw >= MH_HARDMAX)
	{
		hw -= MH_HARDMAX;
		hw = writehardware[(hw << MH_SBITS) + (((unsigned)address >> ABITS_MIN_32) & MHMASK(ABITS2_32))];
		if (hw <= HT_BANKMAX)
			return (unsigned)(&cpu_bankbase[hw][address - memorywriteoffset[hw]]);
	}
	return 0xFFFFFFFF;
}


#ifdef DEBUG_FAME
static void show_banks(void)
{
	int i=0;
	puts("\n-----------");
	puts("- PROGRAM -");
	puts("-----------");
	while((prg[i].offset!=0)&&(prg[i].high_addr!=0xFFFFFFFF))
	{
		printf("%i:\t%.6X %.6X -> %p\n",i,prg[i].low_addr,prg[i].high_addr,prg[i].offset);
		i++;
	}
	i=0;
	puts("\n--------");
	puts("- READ -");
	puts("--------");
	while((dt_read8[i].mem_handler!=NULL)||(dt_read8[i].data!=NULL))
	{
		if ((!dt_read8[i].mem_handler)&&(dt_read8[i].data))
			printf("%i\t%.6X %.6X ->            %p\n",i,dt_read8[i].low_addr,dt_read8[i].high_addr,dt_read8[i].data);
		else
		if ((dt_read8[i].mem_handler)&&(!dt_read8[i].data))
		{
			if (dt_read8[i].mem_handler==(void *)&cpu_readmem32)
				printf("%i\t%.6X %.6X -> DEFAULT\n",i,dt_read8[i].low_addr,dt_read8[i].high_addr);
			else
			{
				int j;
				for (j=0;j<FAME_N_ENTRIES;j++)
					if (dt_read8[i].mem_handler==(void *)miread8_funcs_indirect[j]) break;
				if (j>=FAME_N_ENTRIES)
					printf("%i\t%.6X %.6X -> %p\n",i,dt_read8[i].low_addr,dt_read8[i].high_addr,dt_read8[i].mem_handler);
				else
					printf("%i\t%.6X %.6X -> FUNC-%i (%p,%.6X)\n",i,dt_read8[i].low_addr,dt_read8[i].high_addr,j,miread8_funcs[j],miread8_offs[j]);
			}
		}
		else
			printf("%i\t%.6X %.6X -> %p %p\n",i,dt_read8[i].low_addr,dt_read8[i].high_addr,dt_read8[i].mem_handler,dt_read8[i].data);
			
		i++;
	}
	i=0;
	puts("\n---------");
	puts("- WRITE -");
	puts("---------");
	while((dt_write8[i].mem_handler!=NULL)||(dt_write8[i].data!=NULL))
	{
		if ((!dt_write8[i].mem_handler)&&(dt_write8[i].data))
			printf("%i\t%.6X %.6X ->            %p\n",i,dt_write8[i].low_addr,dt_write8[i].high_addr,dt_write8[i].data);
		else
		if ((dt_write8[i].mem_handler)&&(!dt_write8[i].data))
		{
			if (dt_write8[i].mem_handler==(void *)&cpu_writemem32)
				printf("%i\t%.6X %.6X -> DEFAULT\n",i,dt_write8[i].low_addr,dt_write8[i].high_addr);
			else
			{
				int j;
				for (j=0;j<FAME_N_ENTRIES;j++)
					if (dt_write8[i].mem_handler==(void *)miwrite8_funcs_indirect[j]) break;
				if (j>=FAME_N_ENTRIES)
					printf("%i\t%.6X %.6X -> %p\n",i,dt_write8[i].low_addr,dt_write8[i].high_addr,dt_write8[i].mem_handler);
				else
					printf("%i\t%.6X %.6X -> FUNC-%i (%p,%.6X)\n",i,dt_write8[i].low_addr,dt_write8[i].high_addr,j,miwrite8_funcs[j],miwrite8_offs[j]);
			}
		}
		else
			printf("%i\t%.6X %.6X -> %p %p\n",i,dt_write8[i].low_addr,dt_write8[i].high_addr,dt_write8[i].mem_handler,dt_write8[i].data);
		i++;
	}
	puts("\n"); fflush(stdout);
}
#else
#define show_banks()
#endif


#define CALCULA_OFFS(DI) ((unsigned *)((((unsigned)(DI))-actual+begin)&0xffffffc0))
#define QUITA_BANCO(DI) ((unsigned*)(((unsigned)(DI))&0xffffffc0))

#define TEST_BANK_READWRITE(_R_OR_W_) \
static int test_back_##_R_OR_W_(unsigned begin, unsigned theend) \
{ \
	unsigned start=begin; \
	unsigned end=theend; \
	unsigned actual=(((start+end)>>1)+4)&0xfffffffe; \
	unsigned *punt; \
	get_cpu_##_R_OR_W_##mem32(start,&punt); \
	unsigned *pstart=QUITA_BANCO(punt); \
	while(actual<end) \
	{ \
		unsigned *p; \
		int f=get_cpu_##_R_OR_W_##mem32(actual,&p); \
		if (p!=punt) \
		{ \
			if (f) \
				return 0; \
			if ((CALCULA_OFFS(p)!=pstart)) \
				return 0; \
		} \
		punt=p; \
		start=actual; \
		actual=(((start+end)>>1)+4)&0xfffffffe; \
	} \
	start=begin; \
	actual=((start+end)>>1)-2; \
	while((actual>start)&&(actual<0x1000000)) \
	{ \
		unsigned *p; \
		int f=get_cpu_##_R_OR_W_##mem32(actual,&p); \
		if (p!=punt) \
		{ \
			if (f) \
				return 0; \
			if ((CALCULA_OFFS(p)!=pstart)) \
				return 0; \
		} \
		punt=p; \
		end=actual; \
		actual=(((start+end)>>1)-4)&0xfffffffe; \
	} \
	for(actual=begin;actual<theend;actual+=64) \
	{ \
		unsigned *p; \
		int f=get_cpu_##_R_OR_W_##mem32(actual,&p); \
		if (p!=punt) \
		{ \
			if (f) \
				return 0; \
			if ((CALCULA_OFFS(p)!=pstart)) \
				return 0; \
		} \
	} \
	return 1; \
}

TEST_BANK_READWRITE(read)
TEST_BANK_READWRITE(write)

#undef TEST_BANK_READWRITE
#undef CALCULA_OFFS
#undef QUITA_BANCO

static void put_prg(M68K_PROGRAM *p, unsigned low, unsigned high, unsigned offset)
{
	p->low_addr=low;
	p->high_addr=high;
	p->offset=offset;
}

static void put_end_prg(M68K_PROGRAM *p)
{
	p->low_addr=(unsigned)-1;
	p->high_addr=(unsigned)-1;
	p->offset=(unsigned)NULL;
}

static void put_addr_low(M68K_DATA *r8, M68K_DATA *r16, unsigned low_addr)
{
	r8->low_addr=r16->low_addr=low_addr;
}

static void put_addr_high(M68K_DATA *r8, M68K_DATA *r16, unsigned high_addr)
{
	r8->high_addr=r16->high_addr=high_addr;
}

static void put_addr(M68K_DATA *r8, M68K_DATA *r16,
		unsigned low_addr , unsigned high_addr)
{
	put_addr_low(r8,r16,low_addr);
	put_addr_high(r8,r16,high_addr);
}

static void put_handler(M68K_DATA *r, void *handler)
{
	r->mem_handler=handler;
	r->data=NULL;
}

static void put_end(M68K_DATA *r8, M68K_DATA *r16)
{
	r8->low_addr=r16->low_addr=(unsigned)-1;
	r8->high_addr=r16->high_addr=(unsigned)-1;
	r8->mem_handler=r16->mem_handler=NULL;
	r8->data=NULL;	r16->data=NULL;
}

#define PUT_READWRITE(_R_OR_W_) \
static void put_default_##_R_OR_W_##handler(M68K_DATA *r8, M68K_DATA *r16) \
{ \
	put_handler(r8,(void *)&cpu_##_R_OR_W_##mem32); \
	put_handler(r16,(void *)&cpu_##_R_OR_W_##mem32_word); \
} \
 \
static void put_custom_##_R_OR_W_(unsigned j, unsigned start, unsigned back) \
{ \
	mi##_R_OR_W_##8_funcs[j]=(mi##_R_OR_W_##8_func)back; \
	mi##_R_OR_W_##8_offs[j]=get_cpu_##_R_OR_W_##offset(start); \
	put_handler(&dt_##_R_OR_W_##8[j],(void *)mi##_R_OR_W_##8_funcs_indirect[j]); \
	mi##_R_OR_W_##16_funcs[j]=(mi##_R_OR_W_##16_func)back; \
	mi##_R_OR_W_##16_offs[j]=mi##_R_OR_W_##8_offs[j]; \
	put_handler(&dt_##_R_OR_W_##16[j],(void *)mi##_R_OR_W_##16_funcs_indirect[j]); \
} \
 \
static int put_data_##_R_OR_W_(M68K_DATA *r8, M68K_DATA *r16, unsigned start, unsigned back) \
{ \
	back--; \
	r8->mem_handler=r16->mem_handler=NULL; \
	unsigned data=get_data_##_R_OR_W_(start); \
	if (data&3) \
	{ \
		put_default_##_R_OR_W_##handler(r8,r16); \
		return 1; \
	} \
	r8->data=r16->data=(void *)(data-start); \
	return 0; \
} 

PUT_READWRITE(read)
PUT_READWRITE(write)

#undef PUT_READWRITE

#define SEARCH_PRG() \
{ \
	change_pc32(i); \
	if (back_op!=(unsigned)OP_ROM) \
	{ \
		if (start_op==1) \
		{ \
			start_op=0; \
			back_op=(unsigned)OP_ROM; \
		} \
		else \
		{ \
			put_prg(&prg[j_op],start_op,i-1,back_op); \
			j_op++; \
			start_op=i; \
		} \
		back_op=(unsigned)OP_ROM; \
	} \
}

#define SEARCH_BANK_READWRITE(_R_OR_W_,_LIMIT_) \
{ \
	put_addr(&dt_##_R_OR_W_##8[j_##_R_OR_W_],&dt_##_R_OR_W_##16[j_##_R_OR_W_],start_##_R_OR_W_,(_LIMIT_)-1); \
	if (test_back_##_R_OR_W_(start_##_R_OR_W_,(_LIMIT_))) \
	{ \
		if (func_##_R_OR_W_) \
			put_custom_##_R_OR_W_(j_##_R_OR_W_,start_##_R_OR_W_,back_##_R_OR_W_); \
		else \
			func_##_R_OR_W_=put_data_##_R_OR_W_(&dt_##_R_OR_W_##8[j_##_R_OR_W_],&dt_##_R_OR_W_##16[j_##_R_OR_W_],start_##_R_OR_W_,back_##_R_OR_W_); \
	} \
	else \
		put_default_##_R_OR_W_##handler(&dt_##_R_OR_W_##8[j_##_R_OR_W_],&dt_##_R_OR_W_##16[j_##_R_OR_W_]); \
	j_##_R_OR_W_++; \
	start_##_R_OR_W_=(_LIMIT_); \
} \

#define SEARCH_READWRITE(_R_OR_W_) \
{ \
	func=get_cpu_##_R_OR_W_##mem32(i,&punt); \
	if ((j_##_R_OR_W_<(FAME_N_ENTRIES-1)) && (i>start_##_R_OR_W_) && \
	    ( (func!=func_##_R_OR_W_) || \
 	      (func)&&((unsigned)punt!=(unsigned)back_##_R_OR_W_) || \
 	      (!func)&& \
	      (((unsigned)punt-(i-start_##_R_OR_W_))!=(unsigned)back_##_R_OR_W_)&& \
	      ((unsigned)punt!=(unsigned)back_##_R_OR_W_) \
	   )) \
	{ \
		if (start_##_R_OR_W_==1) \
		{ \
			start_##_R_OR_W_=0; \
			back_##_R_OR_W_=(unsigned)punt; \
			func_##_R_OR_W_=(unsigned)func; \
		} \
		else \
		if (i&0xFFF) \
		{ \
			if ((!func_##_R_OR_W_)&&((i-start_##_R_OR_W_)>4096)) \
			{ \
				put_addr(&dt_##_R_OR_W_##8[j_##_R_OR_W_],&dt_##_R_OR_W_##16[j_##_R_OR_W_],start_##_R_OR_W_,(i&0xFFF000)-1); \
				put_data_##_R_OR_W_(&dt_##_R_OR_W_##8[j_##_R_OR_W_],&dt_##_R_OR_W_##16[j_##_R_OR_W_],start_##_R_OR_W_,back_##_R_OR_W_); \
				j_##_R_OR_W_++; \
				put_addr_low(&dt_##_R_OR_W_##8[j_##_R_OR_W_],&dt_##_R_OR_W_##16[j_##_R_OR_W_],(i&0xFFF000)); \
				j_##_R_OR_W_++; \
				 \
			} \
			else \
			if ((dt_##_R_OR_W_##8[j_##_R_OR_W_-1].high_addr+1-dt_##_R_OR_W_##8[j_##_R_OR_W_-1].low_addr)>=8192) \
			{ \
				dt_##_R_OR_W_##8[j_##_R_OR_W_-1].high_addr-=4096; \
				put_addr_low(&dt_##_R_OR_W_##8[j_##_R_OR_W_],&dt_##_R_OR_W_##16[j_##_R_OR_W_],start_##_R_OR_W_-4096); \
				j_##_R_OR_W_++; \
			} \
			put_default_##_R_OR_W_##handler(&dt_##_R_OR_W_##8[j_##_R_OR_W_-1],&dt_##_R_OR_W_##16[j_##_R_OR_W_-1]); \
			put_addr_high(&dt_##_R_OR_W_##8[j_##_R_OR_W_-1],&dt_##_R_OR_W_##16[j_##_R_OR_W_-1],(i&0xFFF000)+0xFFF); \
			func=1; \
			back_##_R_OR_W_=(unsigned)dt_##_R_OR_W_##8[j_##_R_OR_W_-1].mem_handler; \
			start_##_R_OR_W_=(i&0xFFF000)+0x1000; \
		} \
		else \
			SEARCH_BANK_READWRITE(_R_OR_W_,i) \
		back_##_R_OR_W_=(unsigned)punt; \
		func_##_R_OR_W_=func; \
	} \
}

#define FINISH_SEARCH_PRG() \
{ \
	put_prg(&prg[j_op],start_op,0xFFFFFF,back_op); \
	put_end_prg(&prg[j_op+1]); \
}

#define FINISH_SEARCH_READWRITE(_R_OR_W_) \
{ \
	if (dt_##_R_OR_W_##8[j_##_R_OR_W_-1].high_addr!=0xFFFFFF) \
		SEARCH_BANK_READWRITE(_R_OR_W_,0x1000000) \
	put_end(&dt_##_R_OR_W_##8[j_##_R_OR_W_],&dt_##_R_OR_W_##16[j_##_R_OR_W_]); \
}	

static void search_memory_banks(void)
{
	unsigned i;
       	unsigned j_op=0, j_read=0, j_write=0;
	unsigned start_op=1, start_read=1, start_write=1;
	unsigned func_read=0xFF000FFF, func_write=0xFFFFFFFF;
	unsigned back_op=0xFFFFFFFF, back_read=0xFFFFFFFF, back_write=0xFFFFFFFF;

	for(i=0;i<0x1000000;i+=2)
	{
		unsigned *punt;
		int func;
		if (!(i&0xFFF))
			SEARCH_PRG()
#ifndef FAME_ONLY_HANDLER
		SEARCH_READWRITE(read)
		SEARCH_READWRITE(write)
#endif
	}
	FINISH_SEARCH_PRG()
#ifndef FAME_ONLY_HANDLER
	FINISH_SEARCH_READWRITE(read)
	FINISH_SEARCH_READWRITE(write)
#endif

#ifdef FAME_ONLY_HANDLER
	put_addr(&dt_read8[0],&dt_read16[0],0,0xFFFFFF);
	put_default_readhandler(&dt_read8[0],&dt_read16[0]);
	put_end(&dt_read8[1],&dt_read16[1]);
	put_addr(&dt_write8[0],&dt_write16[0],0,0xFFFFFF);
	put_default_writehandler(&dt_write8[0],&dt_write16[0]);
	put_end(&dt_write8[1],&dt_write16[1]);
#else
#ifdef FAME_ONE_HANDLER
	for(i=0;i<FAME_N_ENTRIES;i++)
	{
		if (dt_read8[i].mem_handler || dt_read8[i].data)
			put_default_readhandler(&dt_read8[i],&dt_read16[i]);
		if (dt_write8[i].mem_handler || dt_write8[i].data)
			put_default_writehandler(&dt_write8[i],&dt_write16[i]);
	}
#endif
#endif
	show_banks();
}

static void put_default_handlers(int n)
{
	put_default_readhandler(&dt_read8[n],&dt_read16[n]);
	put_default_writehandler(&dt_write8[n],&dt_write16[n]);
}

void fame_reset(void *param)
{
	search_memory_banks();
#ifndef NEOMAME
	// init the memory handlers
	if ((Machine->gamedrv == (const struct GameDriver *)&driver_gauntlet)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_gauntir1)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_gauntir2)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_gaunt2p)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_gaunt2)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_vindctr2)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_cyberbal)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_cyberba2)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_cyberbt)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_xybots)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_hydra)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_hydrap)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_pitfight)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_pitfigh3)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_marble)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_marblea)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_marble2)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_peterpak)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_roadrunn)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_roadblst))
		put_default_handlers(1);
	if ((Machine->gamedrv == (const struct GameDriver *)&driver_rampart)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_ramprt2p)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_rampartj)||
	    (Machine->gamedrv == (const struct GameDriver *)&driver_arcadecr))
		put_default_handlers(14);
#endif

	// Init fame
	famem68k_init();
	memset(&fame, 0,sizeof(fame));
	
	fame.regs.fetch=(M68K_PROGRAM*)&prg;

	fame.regs.read_byte=(M68K_DATA*)&dt_read8;
	fame.regs.read_word=(M68K_DATA*)&dt_read16;

	fame.regs.write_byte=(M68K_DATA*)&dt_write8;
	fame.regs.write_word=(M68K_DATA*)&dt_write16;

	fame.regs.sv_fetch=(M68K_PROGRAM*)&prg;
	fame.regs.sv_read_byte=(M68K_DATA*)&dt_read8;
	fame.regs.sv_read_word=(M68K_DATA*)&dt_read16;
	fame.regs.sv_write_byte=(M68K_DATA*)&dt_write8;
	fame.regs.sv_write_word=(M68K_DATA*)&dt_write16;

	fame.regs.user_fetch=(M68K_PROGRAM*)&prg;
	fame.regs.user_read_byte=(M68K_DATA*)&dt_read8;
	fame.regs.user_read_word=(M68K_DATA*)&dt_read16;
	fame.regs.user_write_byte=(M68K_DATA*)&dt_write8;
	fame.regs.user_write_word=(M68K_DATA*)&dt_write16;

	fame.regs.reset_handler=NULL;
	fame.regs.iack_handler=NULL;

	famem68k_set_context(&fame.regs);

	famem68k_reset();
	change_pc24(famem68k_get_pc());
	fame_ICount=0;
}

unsigned int fame_get_pc(void)
{
	return famem68k_get_pc();
}

void fame_set_context(void *src)
{
	if (src) {
		memcpy(&fame,src,sizeof(Fame_Regs));
		famem68k_set_context(&fame.regs);
	}
}

unsigned fame_get_context(void *dst)
{
	if (dst) {
		famem68k_get_context(&fame.regs);
		memcpy(dst,&fame,sizeof(Fame_Regs));
	}
	return sizeof(Fame_Regs);
}

int fame_execute(int cycles)
{
	M68KCONTEXT.cycles_counter = 0;
	famem68k_emulate(cycles);
	return (M68KCONTEXT.cycles_counter);
}

void fame_exit(void)
{
}

void fame_set_pc(unsigned int val)
{
	famem68k_set_register(M68K_REG_PC,val);
}
unsigned fame_get_sp(void)
{
	return famem68k_get_register(M68K_REG_A7);
}

void fame_set_sp(unsigned int val)
{
	famem68k_set_register(M68K_REG_A7,val);
}

enum
{
	/* NOTE: M68K_SP fetches the current SP, be it USP, ISP, or MSP */
	M68K_PC=1, M68K_SP, M68K_ISP, M68K_USP, M68K_MSP, M68K_SR, M68K_VBR,
	M68K_SFC, M68K_DFC, M68K_CACR, M68K_CAAR, M68K_PREF_ADDR, M68K_PREF_DATA,
	M68K_D0, M68K_D1, M68K_D2, M68K_D3, M68K_D4, M68K_D5, M68K_D6, M68K_D7,
	M68K_A0, M68K_A1, M68K_A2, M68K_A3, M68K_A4, M68K_A5, M68K_A6, M68K_A7
};

unsigned fame_get_reg(int regnum)
{
	switch( regnum )
	{
		case M68K_ISP:
		case M68K_USP: return famem68k_get_register(M68K_REG_ASP);
		case M68K_SR:  return famem68k_get_register(M68K_REG_SR);
		case M68K_D0:  return famem68k_get_register(M68K_REG_D0);
		case M68K_D1:  return famem68k_get_register(M68K_REG_D1);
		case M68K_D2:  return famem68k_get_register(M68K_REG_D2);
		case M68K_D3:  return famem68k_get_register(M68K_REG_D3);
		case M68K_D4:  return famem68k_get_register(M68K_REG_D4);
		case M68K_D5:  return famem68k_get_register(M68K_REG_D5);
		case M68K_D6:  return famem68k_get_register(M68K_REG_D6);
		case M68K_D7:  return famem68k_get_register(M68K_REG_D7);
		case M68K_A0:  return famem68k_get_register(M68K_REG_A0);
		case M68K_A1:  return famem68k_get_register(M68K_REG_A1);
		case M68K_A2:  return famem68k_get_register(M68K_REG_A2);
		case M68K_A3:  return famem68k_get_register(M68K_REG_A3);
		case M68K_A4:  return famem68k_get_register(M68K_REG_A4);
		case M68K_A5:  return famem68k_get_register(M68K_REG_A5);
		case M68K_A6:  return famem68k_get_register(M68K_REG_A6);
		case M68K_SP:
		case M68K_A7:  return famem68k_get_register(M68K_REG_A7);
		case M68K_PREF_ADDR:  return 0; // ??
		case M68K_PREF_DATA:  return 0; // ??
		case REG_PREVIOUSPC:
		case M68K_PC:  return famem68k_get_pc();
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = famem68k_get_register(M68K_REG_A7) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
//					return cpu_readmem24bew_dword( offset );
					return cpu_readmem32_dword( offset );
			}
	}
	return 0;
}

void fame_set_reg(int regnum, unsigned val)
{
    switch( regnum )
    {
		case M68K_PC:  famem68k_set_register(M68K_REG_PC, val); break;
		case M68K_ISP:
		case M68K_USP: famem68k_set_register(M68K_REG_ASP, val); break;
		case M68K_SR:  famem68k_set_register(M68K_REG_SR, val); break;
		case M68K_D0:  famem68k_set_register(M68K_REG_D0, val); break;
		case M68K_D1:  famem68k_set_register(M68K_REG_D1, val); break;
		case M68K_D2:  famem68k_set_register(M68K_REG_D2, val); break;
		case M68K_D3:  famem68k_set_register(M68K_REG_D3, val); break;
		case M68K_D4:  famem68k_set_register(M68K_REG_D4, val); break;
		case M68K_D5:  famem68k_set_register(M68K_REG_D5, val); break;
		case M68K_D6:  famem68k_set_register(M68K_REG_D6, val); break;
		case M68K_D7:  famem68k_set_register(M68K_REG_D7, val); break;
		case M68K_A0:  famem68k_set_register(M68K_REG_A0, val); break;
		case M68K_A1:  famem68k_set_register(M68K_REG_A1, val); break;
		case M68K_A2:  famem68k_set_register(M68K_REG_A2, val); break;
		case M68K_A3:  famem68k_set_register(M68K_REG_A3, val); break;
		case M68K_A4:  famem68k_set_register(M68K_REG_A4, val); break;
		case M68K_A5:  famem68k_set_register(M68K_REG_A5, val); break;
		case M68K_A6:  famem68k_set_register(M68K_REG_A6, val); break;
		case M68K_SP:
		case M68K_A7:  famem68k_set_register(M68K_REG_A7, val); break;
		default:
			if( regnum < REG_SP_CONTENTS )
			{
				unsigned offset = famem68k_get_register(M68K_REG_A7) + 4 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xfffffd )
					cpu_writemem32_dword( offset, val );
//					cpu_writemem24bew_word( offset, val );

			}
	}
}

void fame_set_nmi_line(int state)
{
/*
	switch(state)
	{
		case CLEAR_LINE:
			famem68k_lower_irq(7);
			return;
		case ASSERT_LINE:
			famem68k_raise_irq(7,M68K_AUTOVECTORED_IRQ);
			return;
		default:
			famem68k_raise_irq(7,M68K_AUTOVECTORED_IRQ);
			return;
	}
*/
  if (state == CLEAR_LINE)
    famem68k_lower_irq(7);
  else
    famem68k_raise_irq(7,M68K_AUTOVECTORED_IRQ);
}

void fame_set_irq_line(int irqline, int state)
{
/*
	switch(state)
	{
		case CLEAR_LINE:
			famem68k_lower_irq(irqline);
			return;
		case ASSERT_LINE:
			famem68k_raise_irq(irqline,M68K_AUTOVECTORED_IRQ);
			return;
		default:
			famem68k_raise_irq(irqline,M68K_AUTOVECTORED_IRQ);
			return;
	}
*/
  if (state == CLEAR_LINE)
		famem68k_lower_irq(irqline);
  else
    famem68k_raise_irq(irqline,M68K_AUTOVECTORED_IRQ);
}

void fame_set_irq_callback(int (*callback)(int irqline))
{
#if 0
	fame.MAMEIrqCallback = callback;
#endif
}

const char *fame_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "FAME (Fast & Accurate Motorola 68000)";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "28-10-2009";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright Oscar Orallo Pelaez & Stphane Dallongueville. All rights reserved";
	}
	return "";
}

unsigned fame_dasm(char *buffer, unsigned pc)
{
	change_pc24(pc);
	sprintf(buffer, "$%04X", cpu_readop16(pc) );
	return 2;
}

#ifdef __cplusplus
}
#endif
