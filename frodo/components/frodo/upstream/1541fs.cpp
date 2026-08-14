/*
 *  1541fs.cpp - 1541 emulation in host file system
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
 * Notes:
 * ------
 *
 *  - If the directory is opened (file name "$"), a temporary file
 *    with the structure of a 1541 directory file is created and
 *    opened. It can then be accessed in the same way as all other
 *    files.
 *
 * Incompatibilities:
 * ------------------
 *
 *  - No "raw" directory reading
 *  - No relative/sequential/user files
 *  - Only "I" and "UJ" commands implemented
 */

#include "sysdeps.h"

#include "1541fs.h"
#include "IEC.h"
#include "main.h"
#include "Prefs.h"

#include <algorithm>
#include <filesystem>
#include <vector>
namespace fs = std::filesystem;


// Prototypes
static bool match(const char *p, const char *n);


/*
 *  Constructor: Prepare emulation
 */

FSDrive::FSDrive(IEC *iec, const std::string & path) : Drive(iec)
{
	if (change_dir(path)) {
		for (unsigned i = 0; i < 16; ++i) {
			file[i] = nullptr;
		}

		Reset();

		Ready = true;
	}
}


/*
 *  Destructor
 */

FSDrive::~FSDrive()
{
	if (Ready) {
		close_all_channels();
		Ready = false;
	}
}


/*
 *  Change emulation directory
 */

bool FSDrive::change_dir(const std::string & path)
{
	if (fs::is_directory(path)) {
		dir_path = path;

		auto dir_name = dir_path.filename().string();
		memset(dir_title, ' ', sizeof(dir_title));
		for (unsigned i = 0; i < dir_name.length(); ++i) {
			dir_title[i] = ascii2petscii(dir_name[i]);
		}

		return true;

	} else {

		dir_path.clear();
		return false;
	}
}


/*
 *  Open channel
 */

uint8_t FSDrive::Open(int channel, const uint8_t *name, int name_len)
{
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

	if (name[0] == '$')
		return open_directory(channel, name + 1, name_len - 1);

	return open_file(channel, name, name_len);
}


/*
 *  Open file
 */

uint8_t FSDrive::open_file(int channel, const uint8_t *name, int name_len)
{
	char plain_name[NAMEBUF_LENGTH];
	int plain_name_len;
	int mode = FMODE_READ;
	int type = FTYPE_PRG;
	int rec_len = 0;
	parse_file_name(name, name_len, (uint8_t *)plain_name, plain_name_len, mode, type, rec_len, true);

	// Channel 0 is READ, channel 1 is WRITE
	if (channel == 0 || channel == 1) {
		mode = channel ? FMODE_WRITE : FMODE_READ;
		if (type == FTYPE_DEL) {
			type = FTYPE_PRG;
		}
	}

	bool writing = (mode == FMODE_WRITE || mode == FMODE_APPEND);

	// Expand wildcards (only allowed on reading)
	if (strchr(plain_name, '*') || strchr(plain_name, '?')) {
		if (writing) {
			set_error(ERR_SYNTAX33);
			return ST_OK;
		} else {
			find_first_file(plain_name);
		}
	}

	// Relative files are not supported
	if (type == FTYPE_REL) {
		set_error(ERR_UNIMPLEMENTED);
		return ST_OK;
	}

	// Select fopen() mode according to file mode
	const char *mode_str = "rb";
	switch (mode) {
		case FMODE_WRITE:
			mode_str = "wb";
			break;
		case FMODE_APPEND:
			mode_str = "ab";
			break;
	}

	// Open file
	auto file_path = dir_path / plain_name;

	if (! fs::is_directory(dir_path)) {
		set_error(ERR_NOTREADY);
	} else if ((file[channel] = fopen(file_path.string().c_str(), mode_str)) != nullptr) {
		if (mode == FMODE_READ || mode == FMODE_M) {	// Read and buffer first byte
			read_char[channel] = fgetc(file[channel]);
		}
	} else {
		set_error(ERR_FILENOTFOUND);
	}

	return ST_OK;
}


/*
 *  Scan directory for entries matching a pattern and return a sorted list
 *  of entry names
 */

static std::vector<std::string> scan_directory(const fs::path & dir_path, const char *pattern)
{
	// Scan directory for matching entries
	std::vector<std::string> entries;
	for (const auto & entry : fs::directory_iterator{dir_path}) {
		auto file_name = entry.path().filename();
		if (match(pattern, file_name.string().c_str())) {
			entries.push_back(file_name.string());
		}
	}

	// Sort entries by name
	std::sort(entries.begin(), entries.end());

	return entries;
}


/*
 *  Find first file matching wildcard pattern and get its real name
 */

// Return true if name 'n' matches pattern 'p'
static bool match(const char *p, const char *n)
{
	while (*p != '\0') {
		if (*p == '*') {	// Wildcard '*' matches all following characters
			return true;
		} else if (*p != *n) {
			if ((*n == '\0') || (*p != '?'))	// Wildcard '?' matches single character
				return false;
		}
		p++; n++;
	}

	return *n == '\0';
}

void FSDrive::find_first_file(char *pattern)
{
	if (! fs::is_directory(dir_path)) {
		return;
	}

	// Scan directory for matching entries
	auto entries = scan_directory(dir_path, pattern);

	// Return first match which is a file
	for (const auto & entry : entries) {
		if (fs::is_regular_file(dir_path / entry)) {
			strncpy(pattern, entry.c_str(), NAMEBUF_LENGTH);
			return;
		}
	}
}


/*
 *  Open directory, create temporary file
 */

uint8_t FSDrive::open_directory(int channel, const uint8_t *pattern, int pattern_len)
{
	// Special treatment for "$0"
	if (pattern[0] == '0' && pattern[1] == 0) {
		pattern++;
		pattern_len--;
	}

	// Look for file name pattern after ':'
	char ascii_pattern[NAMEBUF_LENGTH] = "*";

	const uint8_t *t = (uint8_t *) memchr(pattern, ':', pattern_len);
	if (t) {
		petscii2ascii(ascii_pattern, t + 1, NAMEBUF_LENGTH);
	}

	// Scan directory for matching entries
	if (! fs::is_directory(dir_path)) {
		set_error(ERR_NOTREADY);
		return ST_OK;
	}

	auto entries = scan_directory(dir_path, ascii_pattern);

	// Create temporary file
	if ((file[channel] = tmpfile()) == nullptr) {
		return ST_OK;
	}

	// Create directory title
	uint8_t buf[32] = "\x01\x04\x01\x01\0\0\x12\x22                \x22 FR 2A";
	memcpy(buf + 8, dir_title, sizeof(dir_title));
	fwrite(buf, 1, sizeof(buf), file[channel]);

	// Create and write one line for every directory entry
	for (const auto & file_name : entries) {

		// Get file statistics
		auto file_path = dir_path / file_name;

		// Clear line with spaces and terminate with null byte
		memset(buf, ' ', sizeof(buf));
		buf[sizeof(buf) - 1] = 0;

		uint8_t *p = buf;
		*p++ = 0x01;	// Dummy line link
		*p++ = 0x01;

		// Calculate size in blocks (of 254 bytes each)
		std::uintmax_t num_blocks;
		if (fs::is_directory(file_path)) {
			num_blocks = 0;
		} else {
			num_blocks = (fs::file_size(file_path) + 254) / 254;
			if (num_blocks > 0xffff) {
				num_blocks = 0xffff;
			}
		}
		*p++ = num_blocks & 0xff;
		*p++ = (num_blocks >> 8) & 0xff;

		p++;
		if (num_blocks < 10) p++;	// Less than 10: add one space
		if (num_blocks < 100) p++;	// Less than 100: add another space

		// Convert and insert file name
		*p++ = '\"';
		uint8_t *q = p;
		for (unsigned i = 0; i < 16 && i < file_name.length(); ++i) {
			uint8_t c = ascii2petscii(file_name[i]);
			if (ThePrefs.MapSlash) {
				if (c == '/') {
					c = '\\';
				} else if (c == '\\') {
					c = '/';
				}
			}
			*q++ = c;
		}
		*q++ = '\"';
		p += 18;

		// File type
		if (fs::is_directory(file_path)) {
			*p++ = 'D';
			*p++ = 'I';
			*p++ = 'R';
		} else {
			*p++ = 'P';
			*p++ = 'R';
			*p++ = 'G';
		}

		// Write line
		fwrite(buf, 1, sizeof(buf), file[channel]);
	}

	// Final line (664 blocks free)
	fwrite("\x01\x01\x98\x02" "BLOCKS FREE.             \0\0", 1, 32, file[channel]);

	// Rewind file for reading and read first byte
	rewind(file[channel]);
	read_char[channel] = fgetc(file[channel]);

	return ST_OK;
}


/*
 *  Close channel
 */

uint8_t FSDrive::Close(int channel)
{
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

void FSDrive::close_all_channels()
{
	for (unsigned i = 0; i < 15; ++i) {
		Close(i);
	}

	cmd_len = 0;
}


/*
 *  Read from channel
 */

uint8_t FSDrive::Read(int channel, uint8_t &byte)
{
	// Channel 15: Error channel
	if (channel == 15) {
		byte = *error_ptr++;

		if (byte != '\r') {
			return ST_OK;
		} else {	// End of message
			set_error(ERR_OK);
			return ST_EOF;
		}
	}

	if (!file[channel]) return ST_READ_TIMEOUT;

	// Read one byte
	byte = read_char[channel];
	int c = fgetc(file[channel]);
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

uint8_t FSDrive::Write(int channel, uint8_t byte, bool eoi)
{
	// Channel 15: Collect chars and execute command on EOI
	if (channel == 15) {
		if (cmd_len >= 58)
			return ST_TIMEOUT;
		
		cmd_buf[cmd_len++] = byte;

		if (eoi) {
			execute_cmd(cmd_buf, cmd_len);
			cmd_len = 0;
		}
		return ST_OK;
	}

	if (!file[channel]) {
		set_error(ERR_FILENOTOPEN);
		return ST_TIMEOUT;
	}

	if (putc(byte, file[channel]) == EOF) {
		set_error(ERR_WRITE25);
		return ST_TIMEOUT;
	}

	return ST_OK;
}


/*
 *  Execute drive commands
 */

// INITIALIZE
void FSDrive::initialize_cmd()
{
	close_all_channels();
}

// VALIDATE
void FSDrive::validate_cmd()
{
	// Nothing to do
}


/*
 *  Reset drive
 */

void FSDrive::Reset()
{
	close_all_channels();
	cmd_len = 0;	
	set_error(ERR_STARTUP);
}
