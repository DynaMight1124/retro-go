/*
 *  1541fs.h - 1541 emulation in host file system
 *
 *  Frodo Copyright (C) Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef C1541FS_H
#define C1541FS_H

#include "IEC.h"

#include <filesystem>
#include <string>


class FSDrive : public Drive {
public:
	FSDrive(IEC *iec, const std::string & path);
	virtual ~FSDrive();

	uint8_t Open(int channel, const uint8_t *name, int name_len) override;
	uint8_t Close(int channel) override;
	uint8_t Read(int channel, uint8_t &byte) override;
	uint8_t Write(int channel, uint8_t byte, bool eoi) override;
	void Reset() override;

private:
	bool change_dir(const std::string & path);

	uint8_t open_file(int channel, const uint8_t *name, int name_len);
	uint8_t open_directory(int channel, const uint8_t *pattern, int pattern_len);
	void find_first_file(char *pattern);
	void close_all_channels();

	void initialize_cmd() override;
	void validate_cmd() override;

	std::filesystem::path dir_path;	// Path to directory
	uint8_t dir_title[16];			// Directory title in PETSCII
	FILE *file[16];					// File pointers for each of the 16 channels

	uint8_t read_char[16];	// Buffers for one-byte read-ahead
};


#endif // ndef C1541FS_H
