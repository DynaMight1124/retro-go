/*
 *  IEC.h - IEC bus routines, 1541 emulation (DOS level)
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

#ifndef IEC_H
#define IEC_H

#include <string>
#include <vector>


/*
 *  Definitions
 */

// Maximum length of file names
const int NAMEBUF_LENGTH = 256;

// C64 status codes
enum {
	ST_OK = 0,				// No error
	ST_READ_TIMEOUT	= 0x02,	// Timeout on reading
	ST_TIMEOUT = 0x03,		// Timeout
	ST_EOF = 0x40,			// End of file
	ST_NOTPRESENT = 0x80	// Device not present
};

// 1541 error codes
enum {
	ERR_OK,				// 00 OK
	ERR_SCRATCHED,		// 01 FILES SCRATCHED
	ERR_UNIMPLEMENTED,	// 03 UNIMPLEMENTED
	ERR_READ20,			// 20 READ ERROR (block header not found)
	ERR_READ21,			// 21 READ ERROR (no sync character)
	ERR_READ22,			// 22 READ ERROR (data block not present)
	ERR_READ23,			// 23 READ ERROR (checksum error in data block)
	ERR_READ24,			// 24 READ ERROR (byte decoding error)
	ERR_WRITE25,		// 25 WRITE ERROR (write-verify error)
	ERR_WRITEPROTECT,	// 26 WRITE PROTECT ON
	ERR_READ27,			// 27 READ ERROR (checksum error in header)
	ERR_WRITE28,		// 28 WRITE ERROR (long data block)
	ERR_DISKID,			// 29 DISK ID MISMATCH
	ERR_SYNTAX30,		// 30 SYNTAX ERROR (general syntax)
	ERR_SYNTAX31,		// 31 SYNTAX ERROR (invalid command)
	ERR_SYNTAX32,		// 32 SYNTAX ERROR (command too long)
	ERR_SYNTAX33,		// 33 SYNTAX ERROR (wildcards on writing)
	ERR_SYNTAX34,		// 34 SYNTAX ERROR (missing file name)
	ERR_WRITEFILEOPEN,	// 60 WRITE FILE OPEN
	ERR_FILENOTOPEN,	// 61 FILE NOT OPEN
	ERR_FILENOTFOUND,	// 62 FILE NOT FOUND
	ERR_FILEEXISTS,		// 63 FILE EXISTS
	ERR_FILETYPE,		// 64 FILE TYPE MISMATCH
	ERR_NOBLOCK,		// 65 NO BLOCK
	ERR_ILLEGALTS,		// 66 ILLEGAL TRACK OR SECTOR
	ERR_NOCHANNEL,		// 70 NO CHANNEL
	ERR_DIRERROR,		// 71 DIR ERROR
	ERR_DISKFULL,		// 72 DISK FULL
	ERR_STARTUP,		// 73 Power-up message
	ERR_NOTREADY		// 74 DRIVE NOT READY
};

// Mountable file types
enum {
	FILE_DISK_IMAGE,	// Disk image file, handled by ImageDrive
	FILE_GCR_IMAGE,		// GCR disk image file, requires full 1541 emulation
	FILE_TAPE_IMAGE,	// Tape image file, handled by Datasette emulation
	FILE_ARCH			// Archive file, handled by ArchDrive
};

// 1541 file types
enum {
	FTYPE_DEL,			// Deleted
	FTYPE_SEQ,			// Sequential
	FTYPE_PRG,			// Program
	FTYPE_USR,			// User
	FTYPE_REL,			// Relative
	FTYPE_UNKNOWN
};

static const char ftype_char[9] = "DSPUL   ";

// 1541 file access modes
enum {
	FMODE_READ,			// Read
	FMODE_WRITE,		// Write
	FMODE_APPEND,		// Append
	FMODE_M				// Read open file
};

// Drive LED states
enum {
	DRVLED_OFF,			// Inactive, LED off
	DRVLED_ON,			// Active, LED on
	DRVLED_ERROR_OFF,	// Error, LED off
	DRVLED_ERROR_ON,	// Error, LED on
	DRVLED_ERROR_FLASH	// Error, flash LED
};

// Information about file in disk image/archive file
struct c64_dir_entry {
	c64_dir_entry(const uint8_t *n, int t, bool o, bool p, size_t s, off_t ofs = 0, uint8_t sal = 0, uint8_t sah = 0)
		: type(t), is_open(o), is_protected(p), size(s), offset(ofs), sa_lo(sal), sa_hi(sah)
	{
		strncpy((char *)name, (const char *)n, 17);
		name[16] = 0;
	}

	// Basic information
	uint8_t name[17];		// File name (C64 charset, null-terminated)
	int type;				// File type (see defines above)
	bool is_open;			// Flag: file open
	bool is_protected;		// Flag: file protected
	size_t size;			// File size (may be approximated)

	// Special information
	off_t offset;			// Offset of file in archive file
	uint8_t sa_lo, sa_hi;	// C64 start address
};


class C64;
class Drive;
class Prefs;

// Class for complete IEC bus system with drives 8..11
class IEC {
public:
	IEC(C64 * c64);
	~IEC();

	void Reset();
	void NewPrefs(const Prefs *prefs);
	void UpdateLEDs();

	uint8_t Out(uint8_t byte, bool eoi);
	uint8_t OutATN(uint8_t byte);
	uint8_t OutSec(uint8_t byte);
	uint8_t In(uint8_t &byte);
	void SetATN();
	void RelATN();
	void Turnaround();
	void Release();

private:
	Drive *create_drive(unsigned num, const std::string & path);

	uint8_t listen(int device);
	uint8_t talk(int device);
	uint8_t unlisten();
	uint8_t untalk();
	uint8_t sec_listen();
	uint8_t sec_talk();
	uint8_t open_out(uint8_t byte, bool eoi);
	uint8_t data_out(uint8_t byte, bool eoi);
	uint8_t data_in(uint8_t &byte);

	C64 * the_c64;	// Pointer to C64 object (for drive LEDs)

	uint8_t name_buf[NAMEBUF_LENGTH];	// Buffer for file names and command strings
	uint8_t *name_ptr;		// Pointer for reception of file name
	int name_len;			// Received length of file name

	Drive *drive[4];		// 4 drives (8..11)

	Drive *listener;		// Pointer to active listener
	Drive *talker;			// Pointer to active talker

	bool listener_active;	// Listener selected, listener_data is valid
	bool talker_active;		// Talker selected, talker_data is valid
	bool listening;			// Last ATN was listen (to decide between sec_listen/sec_talk)

	uint8_t received_cmd;	// Received command code ($x0)
	uint8_t sec_addr;		// Received secondary address ($0x)
};

// Abstract superclass for individual drives
class Drive {
public:
	Drive(IEC *iec);
	virtual ~Drive() {}

	virtual uint8_t Open(int channel, const uint8_t *name, int name_len) = 0;
	virtual uint8_t Close(int channel) = 0;
	virtual uint8_t Read(int channel, uint8_t &byte) = 0;
	virtual uint8_t Write(int channel, uint8_t byte, bool eoi) = 0;
	virtual void Reset() = 0;

	int LED;			// Drive LED state
	bool Ready;			// Drive is ready for operation

protected:
	void set_error(int error, int track = 0, int sector = 0);

	void parse_file_name(const uint8_t *src, int src_len, uint8_t *dest, int &dest_len, int &mode, int &type, int &rec_len, bool convert_charset = false);

	void execute_cmd(const uint8_t *cmd, int cmd_len);
	virtual void block_read_cmd(int channel, int track, int sector, bool user_cmd = false);
	virtual void block_write_cmd(int channel, int track, int sector, bool user_cmd = false);
	virtual void block_execute_cmd(int channel, int track, int sector);
	virtual void block_allocate_cmd(int track, int sector);
	virtual void block_free_cmd(int track, int sector);
	virtual void buffer_pointer_cmd(int channel, int pos);
	virtual void mem_read_cmd(uint16_t adr, uint8_t len);
	virtual void mem_write_cmd(uint16_t adr, uint8_t len, uint8_t *p);
	virtual void mem_execute_cmd(uint16_t adr);
	virtual void copy_cmd(const uint8_t *new_file, int new_file_len, const uint8_t *old_files, int old_files_len);
	virtual void rename_cmd(const uint8_t *new_file, int new_file_len, const uint8_t *old_file, int old_file_len);
	virtual void scratch_cmd(const uint8_t *files, int files_len);
	virtual void position_cmd(const uint8_t *cmd, int cmd_len);
	virtual void initialize_cmd();
	virtual void new_cmd(const uint8_t *name, int name_len, const uint8_t *comma);
	virtual void validate_cmd();
	void unsupp_cmd();

	char error_buf[256];	// Buffer with current error message
	char *error_ptr;		// Pointer within error message	
	int error_len;			// Remaining length of error message
	int current_error;		// Number of current error

	uint8_t cmd_buf[64];	// Buffer for incoming command strings
	int cmd_len;			// Length of received command

private:
	IEC *the_iec;			// Pointer to IEC object
};


/*
 *  Functions
 */

// Convert ASCII character to PETSCII character
extern uint8_t ascii2petscii(char c);

// Convert ASCII string to PETSCII string
extern void ascii2petscii(uint8_t *dest, const char *src, int max);

// Convert PETSCII character to ASCII character
extern char petscii2ascii(uint8_t c);

// Convert PETSCII string to ASCII string
extern void petscii2ascii(char *dest, const uint8_t *src, int max);

// Check whether file is a mountable disk/tape image or archive file, return type
extern bool IsMountableFile(const std::string & path, int & ret_type);

// Read directory of mountable disk image or archive file into c64_dir_entry vector
extern bool ReadDirectory(const std::string & path, int type, std::vector<c64_dir_entry> &vec);

// Check whether file is likely to be a BASIC program
extern bool IsBASICProgram(const std::string & path);


#endif // ndef IEC_H
