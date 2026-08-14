/*---------------------------------------------------------------------------
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version. See also the license.txt file for
 *	additional informations.
 *---------------------------------------------------------------------------
 */

/*
 * Flash chip emulation by Flavor
 *   with ideas from Koyote (who originally got ideas from Flavor :)
 * for emulation of NGPC carts
 */

#define NO_COMMAND              0x00
#define COMMAND_BYTE_PROGRAM    0xA0
#define COMMAND_BLOCK_ERASE     0x30
#define COMMAND_CHIP_ERASE      0x10
#define COMMAND_INFO_READ       0x90
#define MAX_BLOCKS 35 /* a 16m chip has 35 blocks (SA0-SA34) */

#ifdef __cplusplus
extern "C" {
#endif

#include "retro_compat.h"

/* what command are we currently on (if any) */
extern unsigned char currentCommand; 
extern unsigned bootBlockStartAddr;
extern unsigned char bootBlockStartNum;
extern unsigned char blocksDirty[2][MAX_BLOCKS];
extern unsigned char *ngpSaveBuf;
extern unsigned char ngpSaveBufDirty;
extern unsigned char ngpSaveBufActive;
extern unsigned char ngpSaveBufChip;

int ngpSaveDecodeCpuAddr(unsigned int cpuAddr, unsigned char *chip, unsigned int *localAddr);
int ngpSaveIsLocalAddrInWindow(unsigned int localAddr);
int ngpSaveIsCpuAddrInWindow(unsigned int cpuAddr);
unsigned int ngpSaveCpuAddrToOffset(unsigned int cpuAddr);

void flashChipWrite(unsigned int addr, unsigned char data);
void vectFlashWrite(unsigned char chip, unsigned int to,
      unsigned char *fromAddr, unsigned int numBytes);
void vectFlashErase(unsigned char chip, unsigned char blockNum);
void vectFlashChipErase(unsigned char chip);

void setFlashSize(unsigned int romSize);
unsigned char flashReadInfo(unsigned int addr);
void flashShutdown(void);

extern unsigned char needToWriteFile;

#ifdef __cplusplus
}
#endif

