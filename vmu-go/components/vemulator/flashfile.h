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

#ifndef _FLASHFILE_H_
#define _FLASHFILE_H_

enum VMS_FILE_TYPE
{
	DATA,
	GAME,
	NONE,
	UNKNOWN = -1
};

///This describes a file inside the VMS
class VE_VMS_FLASH_FILE
{
	
public:	
    VE_VMS_FLASH_FILE();

    VE_VMS_FLASH_FILE(int index, VMS_FILE_TYPE t, int firstblock, char *name, int size, int header);

    ///Copy constructor
    VE_VMS_FLASH_FILE(const VE_VMS_FLASH_FILE &c) = default;
    VE_VMS_FLASH_FILE &operator=(const VE_VMS_FLASH_FILE &c) = default;

    static VMS_FILE_TYPE getType(int d);

    //Setters and getters
    VMS_FILE_TYPE getType();

    int getStartBlock();

    char *getFileName();

    int getFileSize();

    int getHeaderBlock();

    int getFileIndex();

    void setType(VMS_FILE_TYPE t);

    void setStartBlock(int s);

    void setFileName(char *name);

    void setFileSize(int size);

    void setHeaderBlock(int header);

    void setFileIndex(int index);
    
private:
    VMS_FILE_TYPE type;
    int startBlock;
    char fileName[13];
    int fileSize;   ///In blocks
    int headerBlock;
    int fileIndex;
};

#endif // _FLASHFILE_H_
