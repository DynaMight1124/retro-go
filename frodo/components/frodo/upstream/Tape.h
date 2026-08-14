/*
 *  Tape.h - Emulation of Datasette tape drive
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

#ifndef TAPE_H
#define TAPE_H

#include <string>


// Tape button/mechanism state
enum class TapeState {
	Stop,
	Play,
	Record,
};


class MOS6526;
class Prefs;
struct TapeSaveState;


// Datasette emulation
class Tape {
public:
	Tape(MOS6526 * cia);
	~Tape();

	void Reset();

	void GetState(TapeSaveState * s) const;
	void SetState(const TapeSaveState * s);

	void NewPrefs(const Prefs * prefs);

	void EmulateCycle();

	void SetMotor(bool on);
	void SetButtons(TapeState pressed);
	void Rewind();
	void Forward();

	bool MotorOn() const { return motor_on; }
	TapeState ButtonState() const { return button_state; }
	TapeState DriveState() const { return drive_state; }
	int TapePosition() const;

	void WritePulse(uint32_t cycle);

private:
	void set_drive_state();

	void open_image_file(const std::string & filepath);
	void close_image_file();

	void schedule_read_pulse();
	void trigger_read_pulse();

	MOS6526 * the_cia;		// Pointer to CIA object

	FILE * the_file;		// File pointer for image file
	unsigned tap_version;	// TAP file version
	uint32_t header_size;	// TAP header size
	uint32_t data_size;		// TAP data size
	bool write_protected;	// Flag: Image file write protected
	bool file_extended;		// Flag: Image file grew larger during writing, correct data size in header when closing

	uint32_t current_pos;	// Current position in image file

	bool motor_on;			// Flag: Tape motor on
	TapeState button_state;	// Tape button state
	TapeState drive_state;	// Tape drive mechanism state

	int read_pulse_length;	// Remaining number of cycles in next read pulse (-1 = no pulse pending)
	uint32_t write_cycle;	// Cycle of last write pulse
	bool first_write_pulse;	// Flag: Waiting for first write pulse to determine length
};


// Datasette state
struct TapeSaveState {
	uint32_t current_pos;
	int32_t read_pulse_length;
	uint32_t write_cycle;
	bool first_write_pulse;
	TapeState button_state;
	// Motor state comes from the CPU
};


/*
 *  Check for tape pulse
 */

inline void Tape::EmulateCycle()
{
	if (read_pulse_length < 0)
		return;

	--read_pulse_length;
	if (read_pulse_length == 0) {
		trigger_read_pulse();
	}
}


/*
 *  Functions
 */

// Check whether file with given header (64 bytes) and size looks like a
// tape image file
extern bool IsTapeImageFile(const std::string & path, const uint8_t * header, long size);

// Create new blank tape image file
extern bool CreateTapeImageFile(const std::string & path);


#endif // ndef TAPE_H
