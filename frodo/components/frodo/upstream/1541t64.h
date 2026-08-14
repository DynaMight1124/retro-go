/*
 *  1541t64.h - 1541 emulation in archive-type files (.t64/LYNX/.p00)
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

#ifndef C1541T64_H
#define C1541T64_H

#include "IEC.h"

#include <string>
#include <vector>


/*
 *  Definitions
 */

// Archive types
enum {
	TYPE_T64,			// C64S tape file
	TYPE_LYNX,			// C64 LYNX archive
	TYPE_P00			// .p00 file
};

// Archive file drive class
class ArchDrive : public Drive {
public:
	ArchDrive(IEC *iec, const std::string & filepath);
	virtual ~ArchDrive();

	uint8_t Open(int channel, const uint8_t *name, int name_len) override;
	uint8_t Close(int channel) override;
	uint8_t Read(int channel, uint8_t &byte) override;
	uint8_t Write(int channel, uint8_t byte, bool eoi) override;
	void Reset() override;

private:
	bool change_arch(const std::string & path);

	uint8_t open_file(int channel, const uint8_t *name, int name_len);
	uint8_t open_directory(int channel, const uint8_t *pattern, int pattern_len);
	bool find_first_file(const uint8_t *pattern, int pattern_len, int &num);
	void close_all_channels();

	void rename_cmd(const uint8_t *new_file, int new_file_len, const uint8_t *old_file, int old_file_len) override;
	void initialize_cmd() override;
	void validate_cmd() override;

	FILE *the_file;			// File pointer for archive file
	int archive_type;		// File/archive type (see defines above)
	std::vector<c64_dir_entry> file_info;	// Vector of file information structs for all files in the archive

	char dir_title[16];		// Directory title
	FILE *file[16];			// File pointers for each of the 16 channels (all temporary files)

	uint8_t read_char[16];	// Buffers for one-byte read-ahead
};


/*
 *  Functions
 */

// Check whether file with given header (64 bytes) and size looks like one
// of the file types supported by this module
extern bool IsArchFile(const std::string & path, const uint8_t *header, long size);

// Read directory of archive file into (empty) c64_dir_entry vector
extern bool ReadArchDirectory(const std::string & path, std::vector<c64_dir_entry> &vec);


#endif // ndef C1541T64_H
