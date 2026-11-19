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

#include "flashfile.h"
#include <string.h>


VE_VMS_FLASH_FILE::VE_VMS_FLASH_FILE()
    : type(NONE), startBlock(0), fileName{}, fileSize(0), headerBlock(0), fileIndex(0)
{
}

VE_VMS_FLASH_FILE::VE_VMS_FLASH_FILE(int index, VMS_FILE_TYPE t, int firstblock, char *name, int size, int header)
{ 
	//fileNumber means directory entry
	fileIndex   = index;
	type        = t;
	startBlock  = firstblock;
	setFileName(name);
	fileSize    = size;
	headerBlock = header;
}

VMS_FILE_TYPE VE_VMS_FLASH_FILE::getType(int d)
{
	d &= 0xFF;
	if (d == 0x00)
      return NONE;
	if (d == 0x33)
      return DATA;
	if (d == 0xCC)
      return GAME;
	return UNKNOWN;
}

//Setters and getters
VMS_FILE_TYPE VE_VMS_FLASH_FILE::getType()
{
	return type;
}

int VE_VMS_FLASH_FILE::getStartBlock()
{
	return startBlock;
}

char * VE_VMS_FLASH_FILE::getFileName()
{
	return fileName;
}

int VE_VMS_FLASH_FILE::getFileSize()
{
	return fileSize;
}

int VE_VMS_FLASH_FILE::getHeaderBlock()
{
	return headerBlock;
}

int VE_VMS_FLASH_FILE::getFileIndex() 
{ 
	return fileIndex; 
}

void VE_VMS_FLASH_FILE::setType(VMS_FILE_TYPE t)
{
	type = t;
}

void VE_VMS_FLASH_FILE::setStartBlock(int s)
{
	startBlock = s;
}

void VE_VMS_FLASH_FILE::setFileName(char *name)
{
	strncpy(fileName, name ? name : "", 12);
	fileName[12] = 0;
}

void VE_VMS_FLASH_FILE::setFileSize(int size)
{
	fileSize = size;
}

void VE_VMS_FLASH_FILE::setHeaderBlock(int header)
{
	headerBlock = header;
}

void VE_VMS_FLASH_FILE::setFileIndex(int index) 
{
	fileIndex = index;
}
