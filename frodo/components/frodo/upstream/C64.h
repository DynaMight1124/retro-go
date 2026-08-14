/*
 *  C64.h - Put the pieces together
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

#ifndef C64_H
#define C64_H

#include <SDL_joystick.h>
#include <SDL_gamecontroller.h>

#include <chrono>
#include <string>

#include "Tape.h"


// Sizes of memory areas
constexpr unsigned C64_RAM_SIZE = 0x10000;
constexpr unsigned COLOR_RAM_SIZE = 0x400;
constexpr unsigned BASIC_ROM_SIZE = 0x2000;
constexpr unsigned KERNAL_ROM_SIZE = 0x2000;
constexpr unsigned CHAR_ROM_SIZE = 0x1000;
constexpr unsigned DRIVE_RAM_SIZE = 0x800;
constexpr unsigned DRIVE_ROM_SIZE = 0x4000;

#ifdef NTSC
// Screen refresh frequency (NTSC)
constexpr unsigned SCREEN_FREQ = 60;

// Clock cycles per raster line (NTSC)
constexpr unsigned CYCLES_PER_LINE = 65;
#else
// Screen refresh frequency (PAL)
constexpr unsigned SCREEN_FREQ = 50;

// Clock cycles per raster line (PAL)
constexpr unsigned CYCLES_PER_LINE = 63;
#endif


// false: Frodo, true: FrodoSC
extern bool IsFrodoSC;


enum class PlayMode {
	Play,
	Rewind,
	Forward,
	RequestPause,	// Pause on next VBlank
	Pause,
	RewindFrame,
	ForwardFrame,
};


class Prefs;
class ROMPaths;
class Display;
class MOS6510;
class MOS6569;
class MOS6581;
class MOS6526_1;
class MOS6526_2;
class IEC;
class Cartridge;
class MOS6502_1541;
class GCRDisk;
class Tape;
struct Snapshot;


// Main C64 emulator object
class C64 {
public:
	C64();
	~C64();

	int Run();

	void RequestQuit(int exit_code = 0);
	void RequestPrefsEditor();
	void RequestLoadSnapshot(const std::string & path);

	void Reset(bool clear_memory = false);
	void ResetAndAutoStart();
	void NMI();

	uint32_t CycleCounter() const { return cycle_counter; }

	void NewPrefs(const Prefs *prefs);
	void MountDrive8(bool emul_1541_proc, const char * path);
	void MountDrive1(const char * path = nullptr);
	void InsertCartridge(const std::string & path);

	void MakeSnapshot(Snapshot * s, bool instruction_boundary = false);
	void RestoreSnapshot(const Snapshot * s);
	bool SaveSnapshot(const std::string & filename, std::string & ret_error_msg);
	bool LoadSnapshot(const std::string & filename, Prefs * prefs, std::string & ret_error_msg);

	bool DMALoad(const std::string & filename, std::string & ret_error_msg);
	void AutoStartOp();

	void SetPlayMode(PlayMode mode);
	PlayMode GetPlayMode() const { return play_mode; }

	void JoystickAdded(int32_t index);
	void JoystickRemoved(int32_t instance_id);

	void SetTapeButtons(TapeState pressed);
	void SetTapeControllerButton(bool pressed);
	void RewindTape();
	void ForwardTape();
	TapeState TapeButtonState() const;
	TapeState TapeDriveState() const;
	int TapePosition() const;

	void SetDriveLEDs(int l0, int l1, int l2, int l3);
	void ShowNotification(std::string s);

	uint8_t * RAM;				// C64 memories
	uint8_t * Basic;
	uint8_t * Kernal;
	uint8_t * Char;
	uint8_t * Color;

	uint8_t * RAM1541;			// 1541 memories
	uint8_t * ROM1541;

	Display * TheDisplay;		// Display object

	MOS6510 * TheCPU;			// C64 chip objects
	MOS6569 * TheVIC;
	MOS6581 * TheSID;
	MOS6526_1 * TheCIA1;
	MOS6526_2 * TheCIA2;
	IEC * TheIEC;
	Cartridge * TheCart;		// Inserted cartridge

	MOS6502_1541 * TheCPU1541;	// 1541 objects
	GCRDisk * TheGCRDisk;

	Tape * TheTape;				// Datasette object

	// Builtin ROM data
	static const uint8_t BuiltinBasicROM[BASIC_ROM_SIZE];
	static const uint8_t BuiltinKernalROM[KERNAL_ROM_SIZE];
	static const uint8_t BuiltinCharROM[CHAR_ROM_SIZE];
	static const uint8_t BuiltinDriveROM[DRIVE_ROM_SIZE];

private:
	void load_rom(const std::string & which, const std::string & path, uint8_t * where, size_t size, const uint8_t * builtin);
	void load_rom_files(const ROMPaths & p);
	void init_memory();

	void pause();
	void resume();

	void patch_roms(bool fast_reset, bool emul_1541_proc, bool auto_start);

	void write_to_screen(const char * str);
	void set_keyboard_buffer(const char * str);

#ifdef FRODO_SC
	bool emulate_c64_cycle();
	void emulate_1541_cycle();
#endif

	void swap_cartridge(int oldreu, const std::string & oldcart, int newreu, const std::string & newcart);

	void open_close_joystick(int port, int oldjoy, int newjoy);
	void open_close_joysticks(int oldjoy1, int oldjoy2, int newjoy1, int newjoy2);
	uint8_t poll_joystick(int port);

	int main_loop();
	void poll_input();
	void vblank();
	void handle_rewind();
	void reset_play_mode();

	bool quit_requested;			// Emulator shall quit
	int main_loop_exit_code = 0;	// Exit code to return from main loop

	bool prefs_editor_requested;	// Emulator shall show prefs editor
	bool load_snapshot_requested;	// Emulator shall load snapshot
	std::string requested_snapshot;

	uint32_t cycle_counter;			// Cycle counter

	SDL_Joystick * joy[2] = { nullptr, nullptr };				// SDL joystick devices
	SDL_GameController * controller[2] = { nullptr, nullptr };	// SDL game controller devices

	int joy_minx[2], joy_maxx[2], joy_miny[2], joy_maxy[2]; 	// For joystick debouncing
	int joy_maxtrigl[2], joy_maxtrigr[2];
	bool joy_trigl_on[2], joy_trigr_on[2];
	uint8_t joykey;				// Joystick keyboard emulation mask value

	std::chrono::time_point<std::chrono::steady_clock> frame_start;	// Start time of last frame (for speed control)
	unsigned frame_skip_factor;				// For display update limiting
	unsigned frame_skip_counter;			// For display update limiting

	PlayMode play_mode = PlayMode::Play;	// Current play mode
	Snapshot * rewind_buffer = nullptr;		// Snapshot buffer for rewinding
	size_t rewind_start = 0;				// Index of first recorded snapshot
	size_t rewind_fill = 0;					// Number of recorded snapshots
};


/*
 *  Functions
 */

// Check whether file is a snapshot file
extern bool IsSnapshotFile(const char * filename);

enum {
	KEYCODE_PLAY_ON_TAPE = 64,
	NUM_C64_KEYCODES = 65,
};

// Obtain C64 keycode from key name (<0: invalid key)
extern int KeycodeFromString(const std::string & s);

// Obtain key name from C64 keycode
extern const char * StringForKeycode(unsigned kc);


#endif // ndef C64_H
