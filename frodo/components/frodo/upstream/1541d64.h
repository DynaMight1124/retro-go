/*
 *  1541d64.h - 1541 emulation in disk image files (.d64/.x64)
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

#ifndef C1541D64_H
#define C1541D64_H

#include "IEC.h"

#include <string>
#include <vector>


/*
 *  Definitions
 */

// Constants
constexpr unsigned NUM_SECTORS_35 = 683;	// Number of sectors in a 35-track image
constexpr unsigned NUM_SECTORS_40 = 768;	// Number of sectors in a 40-track image

// Disk image types
enum {
	TYPE_D64,			// D64 file
	TYPE_X64			// x64 file
};

// Channel descriptor
struct channel_desc {
	int mode;			// Channel mode
	bool writing;		// Flag: writing to file (for file channels)
	int buf_num;		// Buffer number for direct access and file channels
	uint8_t *buf;		// Pointer to start of buffer
	uint8_t *buf_ptr;	// Pointer to current position in buffer
	int buf_len;		// Remaining bytes in buffer
	int track, sector;	// Track and sector the buffer will be written to (for writing to file channels)
	int num_blocks;		// Number of blocks in file (for writing to file channels)
	int dir_track;		// Track...
	int dir_sector;		// ...and sector of directory block containing file entry
	int entry;			// Number of entry in directory block
};

// Disk image file descriptor
struct image_file_desc {
	int type;				// See definitions above
	int header_size;		// Size of file header
	int num_tracks;			// Number of tracks
	uint8_t id1, id2;		// Block header ID (as opposed to BAM ID)
	uint8_t error_info[NUM_SECTORS_40]; // Sector error information (1 byte/sector)
	bool has_error_info;	// Flag: error info present in file
};

// Disk image drive class
class ImageDrive : public Drive {
public:
	ImageDrive(IEC *iec, const std::string & filepath);
	virtual ~ImageDrive();

	uint8_t Open(int channel, const uint8_t *name, int name_len) override;
	uint8_t Close(int channel) override;
	uint8_t Read(int channel, uint8_t &byte) override;
	uint8_t Write(int channel, uint8_t byte, bool eoi) override;
	void Reset() override;

	static int ConvErrorInfo(uint8_t error);

private:
	void close_image();
	bool change_image(const std::string & path);

	uint8_t open_file(int channel, const uint8_t *name, int name_len);
	uint8_t open_file_ts(int channel, int track, int sector);
	uint8_t create_file(int channel, const uint8_t *name, int name_len, int type, bool overwrite = false);
	uint8_t open_directory(const uint8_t *pattern, int pattern_len);
	uint8_t open_direct(int channel, const uint8_t *filename);
	void close_all_channels();

	int alloc_buffer(int want);
	void free_buffer(int buf);

	bool find_file(const uint8_t *pattern, int pattern_len, int &dir_track, int &dir_sector, int &entry, bool cont);
	bool find_first_file(const uint8_t *pattern, int pattern_len, int &dir_track, int &dir_sector, int &entry);
	bool find_next_file(const uint8_t *pattern, int pattern_len, int &dir_track, int &dir_sector, int &entry);
	bool alloc_dir_entry(int &track, int &sector, int &entry);

	bool is_block_free(int track, int sector);
	int num_free_blocks(int track);
	int alloc_block(int track, int sector);
	int free_block(int track, int sector);
	bool alloc_block_chain(int track, int sector);
	bool free_block_chain(int track, int sector);
	bool alloc_next_block(int &track, int &sector, int interleave);

	bool read_sector(int track, int sector, uint8_t *buffer);
	bool write_sector(int track, int sector, uint8_t *buffer);
	void write_error_info();

	void block_read_cmd(int channel, int track, int sector, bool user_cmd = false) override;
	void block_write_cmd(int channel, int track, int sector, bool user_cmd = false) override;
	void block_allocate_cmd(int track, int sector) override;
	void block_free_cmd(int track, int sector) override;
	void buffer_pointer_cmd(int channel, int pos) override;
	void mem_read_cmd(uint16_t adr, uint8_t len) override;
	void mem_write_cmd(uint16_t adr, uint8_t len, uint8_t *p) override;
	void copy_cmd(const uint8_t *new_file, int new_file_len, const uint8_t *old_files, int old_files_len) override;
	void rename_cmd(const uint8_t *new_file, int new_file_len, const uint8_t *old_file, int old_file_len) override;
	void scratch_cmd(const uint8_t *files, int files_len) override;
	void initialize_cmd() override;
	void new_cmd(const uint8_t *name, int name_len, const uint8_t *comma) override;
	void validate_cmd() override;

	FILE *the_file;			// File pointer for image file
	image_file_desc desc;	// Image file descriptor
	bool write_protected;	// Flag: image file write-protected

	uint8_t ram[0x800];		// 2k 1541 RAM
	uint8_t dir[258];		// Buffer for directory blocks
	uint8_t *bam;			// Pointer to BAM in 1541 RAM (buffer 4, upper 256 bytes)
	bool bam_dirty;			// Flag: BAM modified, needs to be written back

	channel_desc ch[18];	// Descriptors for channels 0..17 (16 = internal read, 17 = internal write)
	bool buf_free[4];		// Flags: buffer 0..3 free?
};


/*
 *  Functions
 */

// Check whether file with given header (64 bytes) and size looks like one
// of the file types supported by this module
extern bool IsDiskImageFile(const std::string & path, const uint8_t *header, long size);

// Read directory of disk image file into (empty) c64_dir_entry vector
extern bool ReadDiskImageDirectory(const std::string & path, std::vector<c64_dir_entry> &vec);

// Create new blank disk image file
extern bool CreateDiskImageFile(const std::string & path);

// Determine the name of the possible "next" disk image file in a series
extern std::string NextDiskImageFile(const std::string & path);


#endif // ndef C1541D64_H
