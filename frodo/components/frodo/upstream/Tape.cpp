/*
 *  Tape.cpp - Emulation of Datasette tape drive
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

#include "sysdeps.h"

#include "Tape.h"
#include "CIA.h"
#include "IEC.h"
#include "Prefs.h"

#include <filesystem>
namespace fs = std::filesystem;


// Size of TAP image header in bytes
constexpr unsigned TAP_HEADER_SIZE = 20;


/*
 *  Constructor: Open tape image file
 */

Tape::Tape(MOS6526 * cia) : the_cia(cia), the_file(nullptr)
{
	header_size = data_size = 0;
	current_pos = 0;
	write_protected = true;
	file_extended = false;

	motor_on = false;
	button_state = TapeState::Stop;
	drive_state = TapeState::Stop;

	read_pulse_length = -1;
	write_cycle = 0;
	first_write_pulse = true;

	open_image_file(ThePrefs.TapePath);
	Rewind();
}


/*
 *  Destructor: Close tape image file
 */

Tape::~Tape()
{
	close_image_file();
}


/*
 *  Reset tape emulation
 */

void Tape::Reset()
{
	SetMotor(false);
	SetButtons(TapeState::Stop);
}


/*
 *  Preferences may have changed
 */

void Tape::NewPrefs(const Prefs * prefs)
{
	// Image file name changed?
	if (ThePrefs.TapePath != prefs->TapePath) {

		// Swap tape
		close_image_file();
		open_image_file(prefs->TapePath);
		Rewind();
	}
}


/*
 *  Change tape motor control state
 */

void Tape::SetMotor(bool on)
{
	if (motor_on != on) {
		motor_on = on;

		set_drive_state();
		schedule_read_pulse();
		first_write_pulse = true;
	}
}


/*
 *  Change tape button state
 */

void Tape::SetButtons(TapeState pressed)
{
	if (button_state != pressed) {
		if (pressed == TapeState::Record && write_protected) {
			pressed = TapeState::Stop;
		}
		button_state = pressed;

		set_drive_state();
		schedule_read_pulse();
		first_write_pulse = true;
	}
}


/*
 *  Set tape drive mechanism state from motor and button state
 */

void Tape::set_drive_state()
{
	if (the_file != nullptr && motor_on && button_state == TapeState::Play) {
		drive_state = TapeState::Play;
	} else if (the_file != nullptr && motor_on && button_state == TapeState::Record) {
		drive_state = TapeState::Record;
	} else {
		drive_state = TapeState::Stop;
	}
}


/*
 *  Rewind tape to start position
 */

void Tape::Rewind()
{
	if (the_file != nullptr) {
		current_pos = header_size;
		fseek(the_file, current_pos, SEEK_SET);
	}

	SetButtons(TapeState::Stop);	// Stop after rewind

	read_pulse_length = -1;
}


/*
 *  Forward tape to end position
 */

void Tape::Forward()
{
	if (the_file != nullptr) {
		current_pos = header_size + data_size;
		fseek(the_file, current_pos, SEEK_SET);
	}

	SetButtons(TapeState::Stop);	// Stop after forwarding

	read_pulse_length = -1;
}


/*
 *  Return tape position in percent
 */

int Tape::TapePosition() const
{
	if (data_size == 0) {
		return 100;
	} else {
		return (current_pos - header_size) * 100 / data_size;
	}
}


/*
 *  Check whether file with given header (64 bytes) and size looks like a
 *  tape image file
 */

bool IsTapeImageFile(const std::string & path, const uint8_t * header, long size)
{
	return memcmp(header, "C64-TAPE-RAW", 12) == 0;
}


/*
 *  Open tape image file
 */

void Tape::open_image_file(const std::string & filepath)
{
#ifdef FRODO_SC
	// Check file type
	int type;
	if (! IsMountableFile(filepath, type))
		return;
	if (type != FILE_TAPE_IMAGE)
		return;

	// Try opening the file for reading/writing first, then for reading only
	bool read_only = false;
	the_file = fopen(filepath.c_str(), "rb+");
	if (the_file == nullptr) {
		read_only = true;
		the_file = fopen(filepath.c_str(), "rb");
	}

	if (the_file == nullptr)
		return;

	// Get version and data size
	uint8_t header[TAP_HEADER_SIZE];
	if (fread(header, 1, sizeof(header), the_file) != sizeof(header))
		goto error;

	tap_version = header[12];
	if (tap_version != 0 && tap_version != 1)
		goto error;

	header_size = sizeof(header);
	data_size = (header[19] << 24)
	          | (header[18] << 16)
	          | (header[17] <<  8)
	          | (header[16] <<  0);
	write_protected = read_only;
	file_extended = false;
	return;

error:
	fclose(the_file);
#endif // def FRODO_SC
	the_file = nullptr;
}


/*
 *  Close tape image file
 */

void Tape::close_image_file()
{
	if (the_file != nullptr) {
		if (file_extended) {

			// Write new data size to header
			fseek(the_file, 16, SEEK_SET);
			putc((data_size >>  0) & 0xff, the_file);
			putc((data_size >>  8) & 0xff, the_file);
			putc((data_size >> 16) & 0xff, the_file);
			putc((data_size >> 24) & 0xff, the_file);
		}

		fclose(the_file);
		the_file = nullptr;
	}

	header_size = data_size = 0;
	current_pos = 0;
	write_protected = true;
	file_extended = false;
}


/*
 *  Create new blank tape image file, returns false on error
 */

bool CreateTapeImageFile(const std::string & path)
{
	// Open file for writing
	FILE *f = fopen(path.c_str(), "wb");
	if (f == nullptr)
		return false;

	// Create and write header
	uint8_t header[TAP_HEADER_SIZE];
	memset(header, 0, sizeof(header));
	memcpy(header, "C64-TAPE-RAW", 12);
	header[12] = 1;
	if (fwrite(header, 1, sizeof(header), f) != sizeof(header)) {
		fclose(f);
		fs::remove(path);
		return false;
	}

	// Close file
	fclose(f);
	return true;
}


/*
 *  Schedule next tape read pulse
 */

void Tape::schedule_read_pulse()
{
	// Tape playing?
	if (the_file == nullptr || drive_state != TapeState::Play) {
		read_pulse_length = -1;
		return;
	}

	// Pulse pending?
	if (read_pulse_length > 0) {
		return;
	}

	// Get next pulse from image file
	int byte = getc(the_file);
	if (byte == EOF) {
eot:	SetButtons(TapeState::Stop);	// Stop at end of tape
		return;
	}
	++current_pos;

	if (byte) {

		// Regular short pulse
		read_pulse_length += byte * 8;

	} else if (tap_version == 1) {

		// Long pulse
		int lo = getc(the_file);
		if (lo == EOF)
			goto eot;
		++current_pos;

		int mid = getc(the_file);
		if (mid == EOF)
			goto eot;
		++current_pos;

		int hi = getc(the_file);
		if (hi == EOF)
			goto eot;
		++current_pos;

		read_pulse_length += (hi << 16) | (mid << 8) | lo;

	} else {

		// Overflow pulse
		read_pulse_length += 1024 * 8;
	}

	if (read_pulse_length < 0) {
		read_pulse_length = 0;
	}
}


/*
 *  Trigger CIA and get next pulse length
 */

void Tape::trigger_read_pulse()
{
	the_cia->TriggerFlagLine();
	schedule_read_pulse();
}


/*
 *  Write pulse triggered
 */

void Tape::WritePulse(uint32_t cycle)
{
	// Tape recording?
	if (the_file == nullptr || drive_state != TapeState::Record) {
		return;
	}

	if (first_write_pulse) {

		// First pulse, just record the time
		write_cycle = cycle;
		first_write_pulse = false;

	} else {

		// Calculate pulse length
		uint32_t pulse_length = cycle - write_cycle;
		write_cycle = cycle;

		if (pulse_length < 8)
			return;

		if (pulse_length <= 255 * 8) {

			// Regular short pulse
			if (putc(pulse_length / 8, the_file) == EOF)
				return;
			++current_pos;

		} else {

			// Long pulse
			if (putc(0, the_file) == EOF)
				return;
			++current_pos;

			if (tap_version == 1) {
				if (pulse_length > 0xffffff) {
					pulse_length = 0xffffff;
				}

				if (putc((pulse_length >>  0) & 0xff, the_file) == EOF)
					return;
				++current_pos;
				if (putc((pulse_length >>  8) & 0xff, the_file) == EOF)
					return;
				++current_pos;
				if (putc((pulse_length >> 16) & 0xff, the_file) == EOF)
					return;
				++current_pos;
			}
		}

		if (current_pos > header_size + data_size) {
			data_size = current_pos - header_size;
			file_extended = true;
		}
	}
}


/*
 *  Get state
 */

void Tape::GetState(TapeSaveState * s) const
{
	s->current_pos = current_pos;
	s->read_pulse_length = read_pulse_length;
	s->write_cycle = write_cycle;
	s->first_write_pulse = first_write_pulse;
	s->button_state = button_state;
}


/*
 *  Set state
 */

void Tape::SetState(const TapeSaveState * s)
{
	if (the_file != nullptr) {
		current_pos = s->current_pos;
		if (current_pos < header_size) {
			current_pos = header_size;
		}
		if (current_pos > header_size + data_size) {
			current_pos = header_size + data_size;
		}
		fseek(the_file, current_pos, SEEK_SET);

		read_pulse_length = s->read_pulse_length;
		write_cycle = s->write_cycle;
		first_write_pulse = s->first_write_pulse;

		SetButtons(s->button_state);
	}
}
