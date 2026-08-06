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

#include <string.h>
// #include <streams/file_stream.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include "race-memory.h"
#include "types.h"
#include "flash.h"

/* Manuf ID's
Supported
0x98		Toshiba
0xEC		Samsung
0xB0		Sharp

Other
0x89		Intel
0x01		AMD
0xBF		SST
*/
unsigned char manufID = 0x98;   /* we're always Toshiba! */
unsigned char deviceID = 0x2F;
unsigned char cartSize = 32;
unsigned int bootBlockStartAddr = 0x1F0000;
unsigned char bootBlockStartNum = 31;
unsigned char ngpSaveBufActive = 0;
unsigned char ngpSaveBufDirty = 0;
unsigned char ngpSaveBufValid = 0;
unsigned char ngpSaveBufChip = 0;
unsigned char *ngpSaveBuf = NULL;
static size_t saveBufSize = 0x10000;

/* with selector, I get
* writeSaveGameFile: Couldn't open Battery//mnt/sd/Games/race/ChryMast.ngf file
*/
extern char retro_save_directory[3];
#define SAVEGAME_DIR retro_save_directory

unsigned char currentWriteCycle = 1;  /* can be 1 through 6 */
unsigned char currentCommand = NO_COMMAND;

#define FLASH_VALID_ID  0x0053

struct NGFheaderStruct
{
   unsigned short version;		/* always 0x53? */
   unsigned short numBlocks;	/* how many blocks are in the file */
   unsigned int fileLen;		/* length of the file */
};

struct blockStruct
{
   unsigned int NGPCaddr;  /* where this block starts (in NGPC memory map) */
   unsigned int len;  /* length of following data */
};

unsigned char blocksDirty[2][MAX_BLOCKS];  /* max of 2 chips */
unsigned char needToWriteFile = 0;
char *ngfFilename = NULL;

#define FLASH_WRITE 0
#define FLASH_ERASE 1

void setupFlashParams(void)
{
   switch(cartSize)
   {
      default:
      case 32:
         deviceID = 0x2F;  /* the upper chip will always be 16bit */
         bootBlockStartAddr = 0x1F0000;
         bootBlockStartNum = 31;
         break;
      case 16:
         deviceID = 0x2F;
         bootBlockStartAddr = 0x1F0000;
         bootBlockStartNum = 31;
         break;
      case 8:
         deviceID = 0x2C;
         bootBlockStartAddr = 0xF0000;
         bootBlockStartNum = 15;
         break;
      case 4:
         deviceID = 0xAB;
         bootBlockStartAddr = 0x70000;
         bootBlockStartNum = 7;
         break;
      case 0:
         manufID = 0x00;
         deviceID = 0x00;
         bootBlockStartAddr = 0x00000;
         bootBlockStartNum = 0;
         break;
   }
}

unsigned char blockNumFromAddr(unsigned int addr)
{
   addr &= 0x1FFFFF/* & cartAddrMask*/;

   if(addr >= bootBlockStartAddr)
   {
      unsigned int bootAddr = addr-bootBlockStartAddr;
      /* boot block is 32k, 8k, 8k, 16k (0x8000,0x2000,0x2000,0x4000) */
      if(bootAddr < 0x8000)
         return (bootBlockStartAddr / 0x10000);
      else if(bootAddr < 0xA000)
         return (bootBlockStartAddr / 0x10000) + 1;
      else if(bootAddr < 0xC000)
         return (bootBlockStartAddr / 0x10000) + 2;
      else if(bootAddr < 0x10000)
         return (bootBlockStartAddr / 0x10000) + 3;
   }

   return addr / 0x10000;
}

unsigned int blockNumToAddr(unsigned char chip, unsigned char blockNum)
{
   unsigned int addr;

   if(blockNum >= bootBlockStartNum)
   {
      unsigned char bootBlock;

      addr      = bootBlockStartNum * 0x10000;
      bootBlock = blockNum - bootBlockStartNum;
      if(bootBlock>=1)
         addr+= 0x8000;
      if(bootBlock>=2)
         addr+= 0x2000;
      if(bootBlock>=3)
         addr+= 0x2000;
   }
   else
      addr = blockNum * 0x10000;

   if (chip)
      addr+=0x200000;

   return addr;
}

unsigned int blockSize(unsigned char blockNum)
{
   if(blockNum >= bootBlockStartNum)
   {
      unsigned char bootBlock = blockNum - bootBlockStartNum;
      if(bootBlock==3)
         return 0x4000;
      if(bootBlock==2)
         return 0x2000;
      if(bootBlock==1)
         return 0x2000;
      if(bootBlock==0)
         return 0x8000;
   }

   return 0x10000;
}

void setupNGFfilename(void)
{
   const char *romName;
   const char *slash;
   const char *dot;
   size_t dirLen, baseLen, needed;
   char *newBuf;

   romName = m_emuInfo.RomFileName;
   if (!romName || !romName[0])
      return;

   slash = strrchr(romName, path_default_slash_c());
   if (slash)
      romName = slash + 1;

   dot = strrchr(romName, '.');
   baseLen = dot ? (size_t)(dot - romName) : strlen(romName);
   dirLen = strlen(SAVEGAME_DIR);

   needed = dirLen + baseLen + 4 + 1; /* ".ngf" + '\0' */

   newBuf = (char*)realloc(ngfFilename, needed);
   if (!newBuf)
      return;

   ngfFilename = newBuf;

   memcpy(ngfFilename, SAVEGAME_DIR, dirLen);
   memcpy(ngfFilename + dirLen, romName, baseLen);
   memcpy(ngfFilename + dirLen + baseLen, ".ngf", 5);
}

void flashWriteByte(unsigned int addr, unsigned char data, unsigned char operation)
{
   unsigned char chip = 0;
   unsigned int localAddr = addr;
   unsigned int offset;
   
   if(addr < 0x200000)
   {
      chip = 0;
      localAddr = addr;
      blocksDirty[0][blockNumFromAddr(addr)] = 1;
      needToWriteFile = 1;
   }
   else if(addr < 0x400000)
   {
      chip = 1;
      localAddr = addr - 0x200000;
      blocksDirty[1][blockNumFromAddr(addr)] = 1;
      needToWriteFile = 1;
   }
   else
      return;

   if(!ngpSaveBufActive)
      return;

   if(chip != ngpSaveBufChip)
      return;

   if(!ngpSaveIsLocalAddrInWindow(localAddr))
      return;

   offset = localAddr - bootBlockStartAddr;

   if(operation == FLASH_ERASE)
      ngpSaveBuf[offset] = 0xFF;
   else
      ngpSaveBuf[offset] &= data;

   ngpSaveBufDirty = 1;
}

unsigned char flashReadInfo(unsigned int addr)
{
   currentWriteCycle = 1;
   currentCommand = COMMAND_INFO_READ;

   switch(addr&0x03)
   {
      case 0:
         return manufID;
      case 1:
         return deviceID;
      case 2:
         return 0;  /* block not protected */
      case 3:  /* thanks Koyote */
      default:
         return 0x80;
   }
}

void flashChipWrite(unsigned int addr, unsigned char data)
{
   if(addr >= 0x800000 && cartSize != 32)
      return;

   switch(currentWriteCycle)
   {
      case 1:
         if((addr & 0xFFFF) == 0x5555 && data == 0xAA)
            currentWriteCycle++;
         else if(data == 0xF0)
         {
            currentWriteCycle=1; /* this is a reset command */
            
         }
         else
            currentWriteCycle=1;

         currentCommand = NO_COMMAND;
         break;
      case 2:
         if((addr & 0xFFFF) == 0x2AAA && data == 0x55)
            currentWriteCycle++;
         else
            currentWriteCycle=1;

         currentCommand = NO_COMMAND;
         break;
      case 3:
         if((addr & 0xFFFF) == 0x5555 && data == 0x80)
            currentWriteCycle++; /* continue on */
         else if((addr & 0xFFFF) == 0x5555 && data == 0xF0)
         {
            currentWriteCycle=1;
            
         }
         else if((addr & 0xFFFF) == 0x5555 && data == 0x90)
         {
            currentWriteCycle++;
            currentCommand = COMMAND_INFO_READ;
            /* now, the next time we read from flash, 
             * we should return a ID value
             * or a block protect value */
            break;
         }
         else if((addr & 0xFFFF) == 0x5555 && data == 0xA0)
         {
            currentWriteCycle++;
            currentCommand = COMMAND_BYTE_PROGRAM;
            break;
         }
         else
            currentWriteCycle=1;

         currentCommand = NO_COMMAND;
         break;

      case 4:
         /* time to write to flash memory */
         if(currentCommand == COMMAND_BYTE_PROGRAM)
         {
            if(addr >= 0x200000 && addr < 0x400000)
               addr -= 0x200000;
            else if(addr >= 0x800000 && addr < 0xA00000)
               addr -= 0x600000;

            /*should be changed to just write to mainrom */
            flashWriteByte(addr, data, FLASH_WRITE);

            currentWriteCycle=1;
         }
         else if((addr & 0xFFFF) == 0x5555 && data == 0xAA)
            currentWriteCycle++;
         else
            currentWriteCycle=1;

         currentCommand = NO_COMMAND;
         break;
      case 5:
         if((addr & 0xFFFF) == 0x2AAA && data == 0x55)
            currentWriteCycle++;
         else
            currentWriteCycle=1;

         currentCommand = NO_COMMAND;
         break;
      case 6:
         /* chip erase */
         if((addr & 0xFFFF) == 0x5555 && data == 0x10)
         {
            currentWriteCycle=1;
            currentCommand = COMMAND_CHIP_ERASE;

            /* erase the entire chip
             * memset it to all 0xFF
             * I think we won't implement this
             */

            break;
         }
         /* block erase */
         if(data == 0x30 || data == 0x50)
         {
            unsigned char chip=0;
            currentWriteCycle=1;
            currentCommand = COMMAND_BLOCK_ERASE;

            /* erase the entire block that contains addr
             * memset it to all 0xFF */

            if(addr >= 0x800000)
               chip = 1;

            vectFlashErase(chip, blockNumFromAddr(addr));
            break;
         }
         else
            currentWriteCycle=1;

         currentCommand = NO_COMMAND;
         break;


      default:
         currentWriteCycle = 1;
         currentCommand = NO_COMMAND;
         break;
   }
}

/* this should be called when a ROM is unloaded */
void flashShutdown(void)
{
   if (ngfFilename) {
      free(ngfFilename);
      ngfFilename = NULL;
   }

   if (ngpSaveBuf) {
      if (ngpSaveBufDirty)
         

      free(ngpSaveBuf);
      ngpSaveBuf = NULL;
   }

   
}

/* this should be called when a ROM is loaded */
void flashStartup(void)
{
   if (!ngpSaveBuf) {
      ngpSaveBuf = (uint8_t*)malloc(saveBufSize);
   }

   if (ngpSaveBuf) {
      memcpy(ngpSaveBuf, &mainrom[bootBlockStartAddr], saveBufSize);
      ngpSaveBufDirty = false;
      ngpSaveBufActive = true;
   } else {
      ngpSaveBufActive = false;
   }
}

void vectFlashWrite(unsigned char chip, unsigned int to, unsigned char *fromAddr, unsigned int numBytes)
{

   if(chip)
      to+=0x200000;

   while(numBytes--)
   {
      flashWriteByte(to, *fromAddr, FLASH_WRITE);
      fromAddr++;
      to++;
   }
}

void vectFlashErase(unsigned char chip, unsigned char blockNum)
{
   /* this needs to be modified to take into account boot block areas (less than 64k) */
   unsigned int blockAddr = blockNumToAddr(chip, blockNum);
   unsigned int numBytes = blockSize(blockNum);

   while(numBytes--)
   {
      flashWriteByte(blockAddr, 0xFF, FLASH_ERASE);
      blockAddr++;
   }
}

void vectFlashChipErase(unsigned char chip)
{
}

void setFlashSize(unsigned int romSize)
{
   /* add individual hacks here. */

   /*delta warp */
   if(strncmp((const char *)&mainrom[0x24], "DELTA WARP ", 11)==0)
      cartSize = 8;   /* 1 8mbit chip */
   else if(romSize > 0x200000)
      cartSize = 32; /* 2 16mbit chips */
   else if(romSize > 0x100000)
      cartSize = 16; /* 1 16mbit chip */
   else if(romSize > 0x080000) /* 1 8mbit chip */
      cartSize = 8;
   else if(romSize > 0x040000) /* 1 4mbit chip */
      cartSize = 4;
   else if(romSize == 0)  /* no cart, just emu BIOS */
      cartSize = 0;
   else
   {
      /* we don't know.  It's probably a homebrew or something cut down
       * so just pretend we're a Bung! cart
       * 2 16mbit chips */
      cartSize = 32;
   }

   setupFlashParams();


   flashStartup();
}

int ngpSaveDecodeCpuAddr(unsigned int cpuAddr, unsigned char *chip, unsigned int *localAddr)
{
   cpuAddr &= 0x00FFFFFF;

   if(cpuAddr >= 0x00200000 && cpuAddr < 0x00400000)
   {
      *chip = 0;
      *localAddr = cpuAddr - 0x00200000;
      return 1;
   }

   if(cpuAddr >= 0x00800000 && cpuAddr < 0x00A00000)
   {
      *chip = 1;
      *localAddr = cpuAddr - 0x00800000;
      return 1;
   }

   return 0;
}

int ngpSaveIsLocalAddrInWindow(unsigned int localAddr)
{
   return (localAddr >= bootBlockStartAddr &&
           localAddr < (bootBlockStartAddr + 0x10000));
}

int ngpSaveIsCpuAddrInWindow(unsigned int cpuAddr)
{
   unsigned char chip;
   unsigned int localAddr;

   if(!ngpSaveDecodeCpuAddr(cpuAddr, &chip, &localAddr))
      return 0;

   if(!ngpSaveIsLocalAddrInWindow(localAddr))
      return 0;

   if(!ngpSaveBufActive)
      return 0;

   if(chip != ngpSaveBufChip)
      return 0;

   return 1;
}

unsigned int ngpSaveCpuAddrToOffset(unsigned int cpuAddr)
{
   unsigned char chip;
   unsigned int localAddr;

   if(!ngpSaveDecodeCpuAddr(cpuAddr, &chip, &localAddr))
      return 0;

   return localAddr - bootBlockStartAddr;
}