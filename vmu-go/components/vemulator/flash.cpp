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

#include "flash.h"


/* Forward declarations */
extern "C" {
   RFILE* rfopen(const char *path, const char *mode);
   int rfclose(RFILE* stream);
   int64_t rfseek(RFILE* stream, int64_t offset, int origin);
   int rfputc(int character, RFILE * stream);
}

VE_VMS_FLASH::VE_VMS_FLASH(VE_VMS_RAM *_ram)
{
	data          = new byte[0x20000]();
	
	// The destructor closes this when set; only a writable .bin sets it.
	flashWriter   = NULL;
	IsRealFlash   = true;
	IsSaveEnabled = true;
	
	ram           = _ram;
}

VE_VMS_FLASH::~VE_VMS_FLASH()
{
	if(flashWriter)
		rfclose(flashWriter);
   flashWriter = NULL;
   delete []data;
}

///Loads raw VMS data to be easily accessed.
//romType 0: Memory Dump (.bin)
//romType 1: VMS file
//romType 2: DCI file
bool VE_VMS_FLASH::loadROM(byte *d, size_t buffSize, int romType, const char *fileName, bool enableSave)
{
   if (!d || romType < 0 || romType > 2) return false;
   size_t payload = buffSize;
   if (romType == 2) {
      if (buffSize < 32 || ((buffSize - 32) % 4)) return false;
      payload -= 32;
   }
   if (!payload || payload > (romType == 0 ? DATA_SIZE : 200 * 512) ||
       (romType == 0 && payload != DATA_SIZE)) return false;
   if (flashWriter) { rfclose(flashWriter); flashWriter = NULL; }
   IsRealFlash = (romType == 0);
   size_t i, j, k;
   byte *romData  = new byte[0x20000]();
   size_t romSize = buffSize;

   if(romType == 2)
      romSize -= 32;

   if(romType == 2)
   {
      for(i = 0; i < romSize; i += 4)
         for(k = 0; k < 4; ++k)
            romData[i + k] = d[32 + i + 3 - k];
   }
   else 
   {
      for (i = 0; i < romSize; i++)
         romData[i] = d[i] & 0xFF;
   }

   //If VMS or DCI, create a bogus flash memory to contain it in.
   if(romType == 1 || romType == 2)
   {
      int rootPtr = 255*512;
      int FATPtr  = 254*512;
      int dirPtr  = 253*512;
      int sz      = (romSize+511) >> 9;
      int i       = 0;

      IsRealFlash = false;

      //FAT init
      for(; i < 256*2; i += 2)
      {
         romData[FATPtr + i]     = 0xFC;
         romData[FATPtr + i + 1] = 0xFF;
      }

      for(i = 0; i < sz; i++)
      {
         romData[FATPtr + 2*i]     = i+1;
         romData[FATPtr + (2*i)+1] = 0;
      }

      if((--i) >= 0)
      {
         romData[FATPtr + 2*i]     = 0xFA;
         romData[FATPtr + (2*i)+1] = 0xFF;
      }
      romData[FATPtr + 254*2]      = 0xFA;
      romData[FATPtr + (254*2)+1]  = 0xFF;
      romData[FATPtr + 255*2]      = 0xFA;
      romData[FATPtr + (255*2)+1]  = 0xFF;

      for(i = 253; i > 241; --i)
      {
         romData[FATPtr + 2*i]     = i-1;
         romData[FATPtr + (2*i)+1] = 0;
      }
      romData[FATPtr + 241*2]       = 0xFA;
      romData[FATPtr + (241*2) + 1] = 0xFF;

      //Dir init
      romData[dirPtr] = 0xCC;
      //Fill bogus name (Spaces)
      for(i = 4; i < 16; i++)
         romData[dirPtr + i] = ' ';

      romData[dirPtr + 0x18] = sz & 0xFF;
      romData[dirPtr + 0x19] = sz >> 8;
      romData[dirPtr + 0x1A] = 1;

      for(i = 0; i < 16; i++)
         romData[rootPtr + i] = 0x55;

      romData[rootPtr + 0x10] = 1;

      for(i = 0; i < 8; i++)
         romData[rootPtr + 0x30 + i] = romData[dirPtr+0x10 + i];

      romData[rootPtr+ 0x44]  = 255;
      romData[rootPtr + 0x46] = 254;
      romData[rootPtr + 0x48] = 1;
      romData[rootPtr + 0x4A] = 253;
      romData[rootPtr + 0x4C] = 13;
      romData[rootPtr + 0x50] = 200;
   }

   //We also need data as a whole
   for(j = 0; j < DATA_SIZE; j++)
      data[j] = romData[j];

   IsSaveEnabled = enableSave;

   /* Only a whole flash image can be written back in place. A NULL result is
      left as it is and checked at every write: the machine still runs, it just
      does not persist. */
   if(IsSaveEnabled && romType == 0)
      flashWriter = rfopen(fileName, "r+b");
   delete []romData;
   return true;
}

///Returns raw VMS data and its size
size_t VE_VMS_FLASH::getROM(byte *out)
{
   memcpy(out, data, DATA_SIZE);
   return DATA_SIZE;
}

byte *VE_VMS_FLASH::getDataPointer()
{
	return data;
}

///Returns data
size_t VE_VMS_FLASH::getData(byte *out)
{
   memcpy(out, data, DATA_SIZE);
   return DATA_SIZE;
}


//Operations
///Returns byte at address. (No banking)
byte VE_VMS_FLASH::getByte(size_t address)
{
	return address < DATA_SIZE ? data[address] : 0xFF;
}

///Returns byte at address (With banking)
byte VE_VMS_FLASH::readByte(size_t address)
{
	if((ram->readByte(0x154) & 1) == 1)
      address += 0x10000;

	return address < DATA_SIZE ? data[address] : 0xFF;
}

//Returns int16 at address (Little-endian)
int VE_VMS_FLASH::getWord(size_t address)
{
   if (address >= DATA_SIZE - 1) return 0;
   return data[address] | (data[address + 1] << 8);
}

///Writes int to address
void VE_VMS_FLASH::writeByte(size_t address, byte d)
{
	if((ram->readByte(0x154) & 2) != 0) return;   //An EXT similar register but for Flash

	if((ram->readByte(0x154) & 1) == 1) address += 0x10000;

	if (address >= DATA_SIZE) return;
	data[address] = d & 0xFF;

	/* If playing a flashrom, save changes in real time.
	 *
	 * flashWriter is NULL whenever the image could not be opened for writing
	 * -- a read-only file, a read-only directory, a frontend holding it open --
	 * and rfopen's failure was not being checked here. The guest writes flash
	 * whenever the BIOS touches the filesystem, so the first such write took
	 * the whole process down. Losing the save is the right failure; crashing
	 * is not. */
	if(IsRealFlash && IsSaveEnabled && flashWriter)
	{
		rfseek(flashWriter, address, SEEK_SET);
		rfputc(d, flashWriter);
	}
}

//Writes int to raw address
void VE_VMS_FLASH::writeByte_RAW(size_t address, byte d)
{
	if (address >= DATA_SIZE) return;
	data[address] = d & 0xFF;

	//If playing a flashrom, save changes in real time
	if(IsRealFlash && IsSaveEnabled && flashWriter)
	{
		rfseek(flashWriter, address, SEEK_SET);
		rfputc(d, flashWriter);
	}
}

///Writes int16 to address (Little-endian)
void VE_VMS_FLASH::writeWord(size_t address, int d)
{
   if (address >= DATA_SIZE - 1) return;
   writeByte_RAW(address, (byte)d);
   writeByte_RAW(address + 1, (byte)(d >> 8));
}

///Get block data (512)
void VE_VMS_FLASH::readBlock(int blockNumber, byte *out)
{
	for(int j = 0; j < 512; j++)
		out[j] = getByte((blockNumber * 512) + j);
}
///Write data to block (512)
void VE_VMS_FLASH::writeBlock(int blockNumber, byte *in)
{
	for(int j = 0; j < 512; j++)
		writeByte((blockNumber * 512 + j), in[j]);
}

///Checks if card is corrupt
bool VE_VMS_FLASH::IsCorrupt()
{
	for(int i = 0; i < 0x0F; ++i)
		if(data[0x1FE00 + i] != 0x55)
			return true;

	return false;
}

///Counts files in directory
int VE_VMS_FLASH::countFiles()
{
	int count = 0;

	//Read entries in dictionary (Each entry is 32-bytes long)
	for(int i = 0x1E200; i < 0x1FC00; i += 32)
	{
		byte d = getByte(i);

		//If first int in entry is not 0x00, it means that the file is either data or a game
		if(d != 0x00)
			++count;
	}

	return count;
}

//File operations
///Get file info from directory depending on its index
VE_VMS_FLASH_FILE VE_VMS_FLASH::getFileAt(int index)
{
   if (index < 0 || index >= 13 * 16) return VE_VMS_FLASH_FILE();
   const byte *entry = data + (253 - index / 16) * 512 + (index % 16) * 32;
   char name[13] = {};
   memcpy(name, entry + 4, 12);
   for (int i = 11; i >= 0 && name[i] == ' '; --i) name[i] = 0;
   return VE_VMS_FLASH_FILE(index, VE_VMS_FLASH_FILE::getType(entry[0]),
       entry[2] | (entry[3] << 8), name,
       entry[0x18] | (entry[0x19] << 8), entry[0x1A] | (entry[0x1B] << 8));
}

///Get file info from directory depending on its name
VE_VMS_FLASH_FILE VE_VMS_FLASH::getFile(const char *name)
{
   int i;
   VE_VMS_FLASH_FILE file;
   int fileCount = countFiles();

   for(i = 0; i < fileCount; ++i)
   {
      file = getFileAt(i);

      if(!strcmp(file.getFileName(), name))
         break;
   }

   return file;
}

///Counts number of mini-games found in ROM
int VE_VMS_FLASH::countGames()
{
   int i;
	int fileCount = countFiles();
	int gameCount = 0;

	for(i = 0; i < fileCount; i++)
		if(getFileAt(i).getType() == GAME)
			gameCount++;

	return gameCount;
}


//FAT operations
///Get data of file from FAT FS
size_t VE_VMS_FLASH::getFileData(VE_VMS_FLASH_FILE fileinfo, byte *out)
{
   int block = fileinfo.getStartBlock();
   int count = fileinfo.getFileSize();
   if (!out || count < 1 || count > 200) return 0;
   byte visited[200] = {};
   for (int i = 0; i < count; ++i) {
      if (block < 0 || block >= 200 || visited[block]) return 0;
      visited[block] = 1;
      memcpy(out + i * 512, data + block * 512, 512);
      int next = data[0x1FC00 + block * 2] | (data[0x1FC01 + block * 2] << 8);
      if (i == count - 1) return next == 0xFFFA ? (size_t)count * 512 : 0;
      block = next;
   }
   return 0;
}

/* The complete flash is authoritative, including directory and FAT. */
void VE_VMS_FLASH::serialize(VE_STATE &s)
{
   s.raw(data, DATA_SIZE);
}
