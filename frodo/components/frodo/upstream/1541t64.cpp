/*
 *  1541t64.cpp - 1541 emulation in archive-type files (.t64/LYNX/.p00)
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

/*
 *  NOTES:
 *   - This module handles access to files inside (uncompressed) archives
 *     and makes the archive look like a disk. It supports C64S tape images
 *     (.t64), C64 LYNX archives and .p00 files.
 *   - If any file is opened, the contents of the file in the archive file are
 *     copied into a temporary file which is used for reading. This is done
 *     to insert the load address.
 *
 *  Incompatibilities:
 *   - Only read accesses possible
 *   - No "raw" directory reading
 *   - No relative/sequential/user files
 *   - Unimplemented commands: B-P, M-R, M-W, C, S, P, N
 *   - Impossible to implement: B-R, B-W, B-E, B-A, B-F, M-E
 */

#include "sysdeps.h"

#include "1541t64.h"
#include "IEC.h"
#include "Prefs.h"

#define DEBUG 0
#include "debug.h"


// Prototypes
static bool is_t64_header(const uint8_t *header);
static bool is_lynx_header(const uint8_t *header);
static bool is_p00_header(const uint8_t *header);
static bool parse_t64_file(FILE *f, std::vector<c64_dir_entry> &vec, char *dir_title);
static bool parse_lynx_file(FILE *f, std::vector<c64_dir_entry> &vec, char *dir_title);
static bool parse_p00_file(FILE *f, std::vector<c64_dir_entry> &vec, char *dir_title);


/*
 *  Constructor: Prepare emulation
 */

ArchDrive::ArchDrive(IEC *iec, const std::string & filepath) : Drive(iec), the_file(nullptr)
{
	for (unsigned i = 0; i < 16; ++i) {
		file[i] = nullptr;
	}
	Reset();

	// Open archive file
	if (change_arch(filepath)) {
		Ready = true;
	}
}


/*
 *  Destructor
 */

ArchDrive::~ArchDrive()
{
	// Close archive file
	if (the_file) {
		close_all_channels();
		fclose(the_file);
	}
	Ready = false;
}


/*
 *  Open the archive file
 */

bool ArchDrive::change_arch(const std::string & path)
{
	FILE *new_file;

	// Open new archive file
	if ((new_file = fopen(path.c_str(), "rb")) != nullptr) {

		file_info.clear();

		// Read header, determine archive type and parse archive contents
		uint8_t header[64];
		fread(header, 1, 64, new_file);
		bool parsed_ok = false;
		if (is_t64_header(header)) {
			archive_type = TYPE_T64;
			parsed_ok = parse_t64_file(new_file, file_info, dir_title);
		} else if (is_lynx_header(header)) {
			archive_type = TYPE_LYNX;
			parsed_ok = parse_lynx_file(new_file, file_info, dir_title);
		} else if (is_p00_header(header)) {
			archive_type = TYPE_P00;
			parsed_ok = parse_p00_file(new_file, file_info, dir_title);
		}

		if (!parsed_ok) {
			fclose(new_file);
			if (the_file) {
				close_all_channels();
				fclose(the_file);
				the_file = nullptr;
			}
			return false;
		}

		// Close old archive if open, and set new file
		if (the_file) {
			close_all_channels();
			fclose(the_file);
			the_file = nullptr;
		}
		the_file = new_file;
		return true;
	}
	return false;
}


/*
 *  Open channel
 */

uint8_t ArchDrive::Open(int channel, const uint8_t *name, int name_len)
{
	D(bug("ArchDrive::Open channel %d, file %s\n", channel, name));

	set_error(ERR_OK);

	// Channel 15: Execute file name as command
	if (channel == 15) {
		execute_cmd(name, name_len);
		return ST_OK;
	}

	// Close previous file if still open
	if (file[channel]) {
		fclose(file[channel]);
		file[channel] = nullptr;
	}

	if (name[0] == '#') {
		set_error(ERR_NOCHANNEL);
		return ST_OK;
	}

	if (the_file == nullptr) {
		set_error(ERR_NOTREADY);
		return ST_OK;
	}

	if (name[0] == '$')
		return open_directory(channel, name + 1, name_len - 1);

	return open_file(channel, name, name_len);
}


/*
 *  Open file
 */

uint8_t ArchDrive::open_file(int channel, const uint8_t *name, int name_len)
{
	uint8_t plain_name[NAMEBUF_LENGTH];
	int plain_name_len;
	int mode = FMODE_READ;
	int type = FTYPE_DEL;
	int rec_len = 0;
	parse_file_name(name, name_len, plain_name, plain_name_len, mode, type, rec_len);

	// Channel 0 is READ, channel 1 is WRITE
	if (channel == 0 || channel == 1) {
		mode = channel ? FMODE_WRITE : FMODE_READ;
		if (type == FTYPE_DEL) {
			type = FTYPE_PRG;
		}
	}

	bool writing = (mode == FMODE_WRITE || mode == FMODE_APPEND);

	// Wildcards are only allowed on reading
	if (writing && (strchr((const char *)plain_name, '*') || strchr((const char *)plain_name, '?'))) {
		set_error(ERR_SYNTAX33);
		return ST_OK;
	}

	// Allow only read accesses
	if (writing) {
		set_error(ERR_WRITEPROTECT);
		return ST_OK;
	}

	// Relative files are not supported
	if (type == FTYPE_REL) {
		set_error(ERR_UNIMPLEMENTED);
		return ST_OK;
	}

	// Find file
	int num;
	if (find_first_file(plain_name, plain_name_len, num)) {

		// Open temporary file
		if ((file[channel] = tmpfile()) != nullptr) {

			// Write load address (.t64 only)
			if (archive_type == TYPE_T64) {
				fwrite(&file_info[num].sa_lo, 1, 1, file[channel]);
				fwrite(&file_info[num].sa_hi, 1, 1, file[channel]);
			}

			// Copy file contents from archive file to temp file
			uint8_t *buf = new uint8_t[file_info[num].size];
			fseek(the_file, file_info[num].offset, SEEK_SET);
			fread(buf, file_info[num].size, 1, the_file);
			fwrite(buf, file_info[num].size, 1, file[channel]);
			rewind(file[channel]);
			delete[] buf;

			if (mode == FMODE_READ) {	// Read and buffer first byte
				read_char[channel] = getc(file[channel]);
			}
		}
	} else {
		set_error(ERR_FILENOTFOUND);
	}

	return ST_OK;
}


/*
 *  Find first file matching wildcard pattern
 */

// Return true if name 'n' matches pattern 'p'
static bool match(const uint8_t *p, int p_len, const uint8_t *n)
{
	while (p_len-- > 0) {
		if (*p == '*') {	// Wildcard '*' matches all following characters
			return true;
		} else if (*p != *n) {
			if ((*n == 0) || (*p != '?'))	// Wildcard '?' matches single character
				return false;
		}
		p++; n++;
	}

	return *n == 0;
}

bool ArchDrive::find_first_file(const uint8_t *pattern, int pattern_len, int &num)
{
	std::vector<c64_dir_entry>::const_iterator i, end = file_info.end();
	for (i = file_info.begin(), num = 0; i != end; i++, num++) {
		if (match(pattern, pattern_len, (uint8_t *)i->name))
			return true;
	}
	return false;
}


/*
 *  Open directory, create temporary file
 */

uint8_t ArchDrive::open_directory(int channel, const uint8_t *pattern, int pattern_len)
{
	// Special treatment for "$0"
	if (pattern[0] == '0' && pattern_len == 1) {
		pattern++;
		pattern_len--;
	}

	// Look for file name pattern after ':'
	const uint8_t *t = (uint8_t *) memchr(pattern, ':', pattern_len);
	if (t) {
		t++;
		pattern_len -= t - pattern;
		pattern = t;
	} else {
		pattern = (uint8_t *) "*";
		pattern_len = 1;
	}

	// Create temporary file
	if ((file[channel] = tmpfile()) == nullptr)
		return ST_OK;

	// Create directory title
	uint8_t buf[] = "\001\004\001\001\0\0\022\042                \042 00 2A";
	for (unsigned i = 0; i < 16 && dir_title[i]; ++i) {
		buf[i + 8] = dir_title[i];
	}
	fwrite(buf, 1, 32, file[channel]);

	// Create and write one line for every directory entry
	std::vector<c64_dir_entry>::const_iterator i, end = file_info.end();
	for (i = file_info.begin(); i != end; i++) {

		// Include only files matching the pattern
		if (match(pattern, pattern_len, (uint8_t *)i->name)) {

			// Clear line with spaces and terminate with null byte
			memset(buf, ' ', 31);
			buf[31] = 0;

			uint8_t *p = (uint8_t *)buf;
			*p++ = 0x01;	// Dummy line link
			*p++ = 0x01;

			// Calculate size in blocks (254 bytes each)
			int n = (i->size + 254) / 254;
			*p++ = n & 0xff;
			*p++ = (n >> 8) & 0xff;

			p++;
			if (n < 10) p++;	// Less than 10: add one space
			if (n < 100) p++;	// Less than 100: add another space

			// Convert and insert file name
			*p++ = '\"';
			uint8_t *q = p;
			for (unsigned j = 0; j < 16 && i->name[j]; ++j) {
				*q++ = i->name[j];
			}
			*q++ = '\"';
			p += 18;

			// File type
			switch (i->type) {
				case FTYPE_DEL:
					*p++ = 'D';
					*p++ = 'E';
					*p++ = 'L';
					break;
				case FTYPE_SEQ:
					*p++ = 'S';
					*p++ = 'E';
					*p++ = 'Q';
					break;
				case FTYPE_PRG:
					*p++ = 'P';
					*p++ = 'R';
					*p++ = 'G';
					break;
				case FTYPE_USR:
					*p++ = 'U';
					*p++ = 'S';
					*p++ = 'R';
					break;
				case FTYPE_REL:
					*p++ = 'R';
					*p++ = 'E';
					*p++ = 'L';
					break;
				default:
					*p++ = '?';
					*p++ = '?';
					*p++ = '?';
					break;
			}

			// Write line
			fwrite(buf, 1, 32, file[channel]);
		}
	}

	// Final line
	fwrite("\001\001\0\0BLOCKS FREE.             \0\0", 1, 32, file[channel]);

	// Rewind file for reading and read first byte
	rewind(file[channel]);
	read_char[channel] = getc(file[channel]);

	return ST_OK;
}


/*
 *  Close channel
 */

uint8_t ArchDrive::Close(int channel)
{
	D(bug("ArchDrive::Close channel %d\n", channel));

	if (channel == 15) {
		close_all_channels();
		return ST_OK;
	}

	if (file[channel]) {
		fclose(file[channel]);
		file[channel] = nullptr;
	}

	return ST_OK;
}


/*
 *  Close all channels
 */

void ArchDrive::close_all_channels()
{
	for (unsigned i = 0; i < 15; ++i) {
		Close(i);
	}

	cmd_len = 0;
}


/*
 *  Read from channel
 */

uint8_t ArchDrive::Read(int channel, uint8_t &byte)
{
	D(bug("ArchDrive::Read channel %d\n", channel));

	// Channel 15: Error channel
	if (channel == 15) {
		byte = *error_ptr++;

		if (byte != '\x0d') {
			return ST_OK;
		} else {	// End of message
			set_error(ERR_OK);
			return ST_EOF;
		}
	}

	if (!file[channel]) return ST_READ_TIMEOUT;

	// Get char from buffer and read next
	byte = read_char[channel];
	int c = getc(file[channel]);
	if (c == EOF) {
		return ST_EOF;
	} else {
		read_char[channel] = c;
		return ST_OK;
	}
}


/*
 *  Write to channel
 */

uint8_t ArchDrive::Write(int channel, uint8_t byte, bool eoi)
{
	D(bug("ArchDrive::Write channel %d, byte %02x, eoi %d\n", channel, byte, eoi));

	// Channel 15: Collect chars and execute command on EOI
	if (channel == 15) {
		if (cmd_len > 58) {
			set_error(ERR_SYNTAX32);
			return ST_TIMEOUT;
		}
		
		cmd_buf[cmd_len++] = byte;

		if (eoi) {
			execute_cmd(cmd_buf, cmd_len);
			cmd_len = 0;
		}
		return ST_OK;
	}

	if (!file[channel]) {
		set_error(ERR_FILENOTOPEN);
	} else {
		set_error(ERR_WRITEPROTECT);
	}

	return ST_TIMEOUT;
}


/*
 *  Execute drive commands
 */

// RENAME:new=old
//        ^   ^
// new_file   old_file
void ArchDrive::rename_cmd(const uint8_t *new_file, int new_file_len, const uint8_t *old_file, int old_file_len)
{
	// Check if destination file is already present
	int num;
	if (find_first_file(new_file, new_file_len, num)) {
		set_error(ERR_FILEEXISTS);
		return;
	}

	// Check if source file is present
	if (!find_first_file(old_file, old_file_len, num)) {
		set_error(ERR_FILENOTFOUND);
		return;
	}

	set_error(ERR_WRITEPROTECT);
}

// INITIALIZE
void ArchDrive::initialize_cmd()
{
	close_all_channels();
}

// VALIDATE
void ArchDrive::validate_cmd()
{
}


/*
 *  Reset drive
 */

void ArchDrive::Reset()
{
	close_all_channels();
	cmd_len = 0;	
	set_error(ERR_STARTUP);
}


/*
 *  Check whether file with given header (64 bytes) and size looks like one
 *  of the file types supported by this module
 */

static bool is_t64_header(const uint8_t *header)
{
	if (memcmp(header, "C64S tape file", 14) == 0
	 || memcmp(header, "C64 tape image", 14) == 0
	 || memcmp(header, "C64S tape image", 15) == 0) {
		return true;
	} else {
		return false;
	}
}

static bool is_lynx_header(const uint8_t *header)
{
	return memcmp(header + 0x38, "USE LYNX", 8) == 0;
}

static bool is_p00_header(const uint8_t *header)
{
	return memcmp(header, "C64File", 7) == 0;
}

bool IsArchFile(const std::string & path, const uint8_t *header, long size)
{
	return is_t64_header(header) || is_lynx_header(header) || is_p00_header(header);
}


/*
 *  Read directory of archive file into (empty) c64_dir_entry vector,
 *  returns false on error
 */

static bool parse_t64_file(FILE *f, std::vector<c64_dir_entry> &vec, char *dir_title)
{
	// Read header and get maximum number of files contained
	fseek(f, 32, SEEK_SET);
	uint8_t buf[32];
	fread(&buf, 32, 1, f);

	unsigned max = (buf[3] << 8) | buf[2];
	if (max == 0) {
		max = 1;
	}

	memcpy(dir_title, buf+8, 16);

	// Allocate buffer for file records and read them
	uint8_t *buf2 = new uint8_t[max * 32];
	fread(buf2, 32, max, f);

	// Determine number of files contained
	unsigned num_files = 0;
	for (unsigned i = 0; i < max; ++i) {
		if (buf2[i*32] == 1) {
			num_files++;
		}
	}

	if (num_files == 0) {
		delete[] buf2;
		return false;
	}

	// Construct file information array
	vec.reserve(num_files);
	uint8_t *b = buf2;
	for (unsigned i = 0; i < max; ++i, b += 32) {
		if (b[0] == 1) {

			// Convert file name (strip trailing spaces)
			uint8_t name_buf[17];
			memcpy(name_buf, b + 16, 16);
			name_buf[16] = 0x20;
			uint8_t *p = name_buf + 16;
			while (*p-- == 0x20) ;
			p[2] = 0;

			// Find file size and offset
			size_t size = ((b[5] << 8) | b[4]) - ((b[3] << 8) | b[2]);
			off_t offset = (b[11] << 24) | (b[10] << 16) | (b[9] << 8) | b[8];

			// Add entry
			vec.push_back(c64_dir_entry(name_buf, FTYPE_PRG, false, false, size, offset, b[2], b[3]));
		}
	}

	delete[] buf2;
	return true;
}

static bool parse_lynx_file(FILE *f, std::vector<c64_dir_entry> &vec, char *dir_title)
{
	// Dummy directory title
	strcpy(dir_title, "LYNX ARCHIVE    ");

	// Read header and get number of directory blocks and files contained
	fseek(f, 0x60, SEEK_SET);
	int dir_blocks;
	fscanf(f, "%d", &dir_blocks);
	while (getc(f) != 0x0d) {
		if (feof(f))
			return false;
	}
	int num_files;
	fscanf(f, "%d\x0d", &num_files);

	// Construct file information array
	vec.reserve(num_files);
	unsigned cur_offset = dir_blocks * 254;
	for (int i = 0; i < num_files; ++i) {

		// Read and convert file name (strip trailing shift-spaces)
		uint8_t name_buf[17];
		fread(name_buf, 16, 1, f);
		name_buf[16] = 0xa0;
		uint8_t *p = name_buf + 16;
		while (*p-- == 0xa0) { }
		p[2] = 0;

		// Read file length and type
		int num_blocks, last_block;
		char type_char;
		fscanf(f, "\x0d%d\x0d%c\x0d%d\x0d", &num_blocks, &type_char, &last_block);
		size_t size = (num_blocks - 1) * 254 + last_block - 1;

		int type;
		switch (type_char) {
			case 'S':
				type = FTYPE_SEQ;
				break;
			case 'U':
				type = FTYPE_USR;
				break;
			case 'R':
				type = FTYPE_REL;
				break;
			default:
				type = FTYPE_PRG;
				break;
		}

		// Read start address
		long here = ftell(f);
		uint8_t sa_lo, sa_hi;
		fseek(f, cur_offset, SEEK_SET);
		fread(&sa_lo, 1, 1, f);
		fread(&sa_hi, 1, 1, f);
		fseek(f, here, SEEK_SET);

		// Add entry
		vec.push_back(c64_dir_entry(name_buf, type, false, false, size, cur_offset, sa_lo, sa_hi));

		cur_offset += num_blocks * 254;
	}

	return true;
}

static bool parse_p00_file(FILE *f, std::vector<c64_dir_entry> &vec, char *dir_title)
{
	// Dummy directory title
	strcpy(dir_title, ".P00 FILE       ");

	// Contains only one file
	vec.reserve(1);

	// Read file name and start address
	uint8_t name_buf[17];
	fseek(f, 8, SEEK_SET);
	fread(name_buf, 17, 1, f);
	name_buf[16] = 0;
	uint8_t sa_lo, sa_hi;
	fseek(f, 26, SEEK_SET);
	fread(&sa_lo, 1, 1, f);
	fread(&sa_hi, 1, 1, f);

	// Get file size
	fseek(f, 0, SEEK_END);
	long size = ftell(f) - 26;

	// Add entry
	vec.push_back(c64_dir_entry(name_buf, FTYPE_PRG, false, false, size, 26, sa_lo, sa_hi));
	return true;
}

bool ReadArchDirectory(const std::string & path, std::vector<c64_dir_entry> &vec)
{
	// Open file
	FILE *f = fopen(path.c_str(), "rb");
	if (f) {

		// Read header
		uint8_t header[64];
		fread(header, 1, sizeof(header), f);

		// Determine archive type and parse archive
		bool result = false;
		char dir_title[16];
		if (is_t64_header(header)) {
			result = parse_t64_file(f, vec, dir_title);
		} else if (is_lynx_header(header)) {
			result = parse_lynx_file(f, vec, dir_title);
		} else if (is_p00_header(header)) {
			result = parse_p00_file(f, vec, dir_title);
		}

		fclose(f);
		return result;
	} else {
		return false;
	}
}
