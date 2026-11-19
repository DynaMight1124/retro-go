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

#ifndef _FLASH_H_
#define _FLASH_H_

#include <string.h>
#include <streams/file_stream.h>

#include "common.h"
#include "flashfile.h"
#include "ram.h"
#include "state.h"

class VE_VMS_FLASH
{
public:
    VE_VMS_FLASH(VE_VMS_RAM *_ram);
    ~VE_VMS_FLASH();

    void serialize(VE_STATE &s);

    ///Loads raw VMS data to be easily accessed.
    //romType 0: Memory Dump (.bin)
    //romType 1: VMS file
    //romType 2: DCI file
    bool loadROM(byte *d, size_t buffSize, int romType, const char *fileName, bool enableSave);

    ///Returns raw VMS data and its size
    size_t getROM(byte *out);

    ///Returns data
    size_t getData(byte *out);

    /* The flash itself, for the frontend to persist and restore. */
    byte *getDataPointer();

    static const size_t DATA_SIZE = 0x20000;


    //Operations
    ///Returns byte at address. (No banking)
    byte getByte(size_t address);

    ///Returns byte at address (With banking)
    byte readByte(size_t address);

    //Returns int16 at address (Little-endian)
    int getWord(size_t address);

    ///Writes int to address
    void writeByte(size_t address, byte d);

    //Writes int to raw address
    void writeByte_RAW(size_t address, byte d);

    ///Writes int16 to address (Little-endian)
    void writeWord(size_t address, int d);

    ///Get block data (512)
    void readBlock(int blockNumber, byte *out);
    
    ///Write data to block (512)
    void writeBlock(int blockNumber, byte *in);
    
    ///Checks if card is corrupt
    bool IsCorrupt();

    ///Counts files in directory
    int countFiles();

    //File operations
    ///Get file info from directory depending on its index
    VE_VMS_FLASH_FILE getFileAt(int index);

    ///Get file info from directory depending on its name
    VE_VMS_FLASH_FILE getFile(const char *name);
    
    ///Counts number of mini-games found in ROM
    int countGames();

    //FAT operations
    ///Get data of file from FAT FS
    size_t getFileData(VE_VMS_FLASH_FILE fileinfo, byte *out);
    
private:
    //File structure
    byte *data;
    RFILE *flashWriter;
    bool IsRealFlash;
    bool IsSaveEnabled;
    
    VE_VMS_RAM *ram;
};

#endif // _FLASH_H_
