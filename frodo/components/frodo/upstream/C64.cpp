/*
 *  C64.cpp - Put the pieces together
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

#include "C64.h"
#include "1541gcr.h"
#include "CIA.h"
#include "CPUC64.h"
#include "CPU1541.h"
#include "Display.h"
#include "IEC.h"
#include "main.h"
#include "Prefs.h"
#include "REU.h"
#include "SID.h"
#include "Tape.h"
#include "VIC.h"

#include <SDL.h>

#include <chrono>
#include <memory>
#include <thread>
#include <utility>
namespace chrono = std::chrono;


#ifdef FRODO_SC
bool IsFrodoSC = true;
#else
bool IsFrodoSC = false;
#endif


// Builtin ROMs
#include "Basic_ROM.h"
#include "Kernal_ROM.h"
#include "Char_ROM.h"
#include "1541_ROM.h"


// Snapshot magic header
#define SNAPSHOT_HEADER "FrodoSnapshot4\x01\0"

// Snapshot flags
#define SNAPSHOT_FLAG_1541_PROC 1

// Snapshot data structure
struct Snapshot {
	uint8_t magic[16];
	uint16_t flags;

	char drive8Path[256];

	uint8_t ram[C64_RAM_SIZE];
	uint8_t color[COLOR_RAM_SIZE];

	uint8_t driveRam[DRIVE_RAM_SIZE];

	uint32_t cycleCounter;

	MOS6510State cpu;
	MOS6569State vic;
	MOS6581State sid;
	MOS6526State cia1;
	MOS6526State cia2;

	MOS6502State driveCPU;
	GCRDiskState driveGCR;

	TapeSaveState tape;

	// TODO: REU state is not saved
};


// Length of rewind buffer
constexpr size_t REWIND_LENGTH = SCREEN_FREQ * 30;  // 30 seconds


// For speed limiting to 50/60 fps
constexpr int FRAME_TIME_us = 1000000 / SCREEN_FREQ;	// 20 ms for 50 fps (PAL)
constexpr int FORWARD_SCALE = 4;	// Fast-forward is four times faster


// Joystick dead zone around center (+/-), and hysteresis to prevent jitter
constexpr int JOYSTICK_DEAD_ZONE = 12000;
constexpr int JOYSTICK_HYSTERESIS = 1000;


/*
 *  Constructor: Allocate objects and memory
 */

C64::C64() : quit_requested(false), prefs_editor_requested(false), load_snapshot_requested(false)
{
	// Allocate RAM/ROM memory
	RAM = new uint8_t[C64_RAM_SIZE];
	Basic = new uint8_t[BASIC_ROM_SIZE];
	Kernal = new uint8_t[KERNAL_ROM_SIZE];
	Char = new uint8_t[CHAR_ROM_SIZE];
	Color = new uint8_t[COLOR_RAM_SIZE];
	RAM1541 = new uint8_t[DRIVE_RAM_SIZE];
	ROM1541 = new uint8_t[DRIVE_ROM_SIZE];

	// Open display
	TheDisplay = new Display(this);

	// Initialize memory
	init_memory();

	// Load ROM files
	load_rom_files(ThePrefs.SelectedROMPaths());

	// Patch ROMs for IEC routines and fast reset
	patch_roms(ThePrefs.FastReset, ThePrefs.Emul1541Proc, ThePrefs.AutoStart);

	// Create the chips
	TheCPU = new MOS6510(this, RAM, Basic, Kernal, Char, Color);

	TheGCRDisk = new GCRDisk(RAM1541);
	TheCPU1541 = new MOS6502_1541(this, TheGCRDisk, RAM1541, ROM1541);
	TheGCRDisk->SetCPU(TheCPU1541);

	TheVIC = new MOS6569(this, TheDisplay, TheCPU, RAM, Char, Color);
	TheSID = new MOS6581;
	TheCIA1 = new MOS6526_1(TheCPU, TheVIC);
	TheCIA2 = TheCPU1541->TheCIA2 = new MOS6526_2(TheCPU, TheVIC, TheCPU1541);
	TheIEC = new IEC(this);
	TheTape = new Tape(TheCIA1);

	TheCart = new NoCartridge;
	swap_cartridge(REU_NONE, "", ThePrefs.REUType, ThePrefs.CartridgePath);

	TheCPU->SetChips(TheVIC, TheSID, TheCIA1, TheCIA2, TheCart, TheIEC, TheTape);

	// Initialize joystick variables
	joy_minx[0] = joy_miny[0] = -JOYSTICK_DEAD_ZONE;
	joy_maxx[0] = joy_maxy[0] = +JOYSTICK_DEAD_ZONE;
	joy_maxtrigl[0] = joy_maxtrigr[0] = +JOYSTICK_DEAD_ZONE;
	joy_trigl_on[0] = joy_trigr_on[0] = false;

	joy_minx[1] = joy_miny[1] = -JOYSTICK_DEAD_ZONE;
	joy_maxx[1] = joy_maxy[1] = +JOYSTICK_DEAD_ZONE;
	joy_maxtrigl[1] = joy_maxtrigr[1] = +JOYSTICK_DEAD_ZONE;
	joy_trigl_on[1] = joy_trigr_on[1] = false;

	// Open joystick drivers if required
	open_close_joysticks(0, 0, ThePrefs.Joystick1Port, ThePrefs.Joystick2Port);
	joykey = 0xff;

	// Allocate buffer for rewinding
	rewind_buffer = new Snapshot[REWIND_LENGTH];
}


/*
 *  Destructor: Delete all objects
 */

C64::~C64()
{
	open_close_joysticks(ThePrefs.Joystick1Port, ThePrefs.Joystick2Port, 0, 0);

	delete TheTape;
	delete TheGCRDisk;
	delete TheCart;
	delete TheIEC;
	delete TheCIA2;
	delete TheCIA1;
	delete TheSID;
	delete TheVIC;
	delete TheCPU1541;
	delete TheCPU;
	delete TheDisplay;

	delete[] RAM;
	delete[] Basic;
	delete[] Kernal;
	delete[] Char;
	delete[] Color;
	delete[] RAM1541;
	delete[] ROM1541;

	delete[] rewind_buffer;
}


/*
 *  Load ROM files
 */

void C64::load_rom(const std::string & which, const std::string & path, uint8_t * where, size_t size, const uint8_t * builtin)
{
	if (! path.empty()) {
		FILE * f = fopen(path.c_str(), "rb");
		if (f) {
			fseek(f, 0, SEEK_END);
			long file_size = ftell(f);
			fseek(f, 0, SEEK_SET);

			if (file_size == size) {
				size_t actual = fread(where, 1, size, f);
				if (actual == size) {
					fclose(f);
					return;
				}
			}

			fclose(f);
		}

		fprintf(stderr, "WARNING: Cannot load %s ROM file '%s', using built-in\n", which.c_str(), path.c_str());
	}

	// Use builtin ROM
	memcpy(where, builtin, size);
}

void C64::load_rom_files(const ROMPaths & p)
{
	load_rom("Basic", p.BasicROMPath, Basic, BASIC_ROM_SIZE, BuiltinBasicROM);
	load_rom("Kernal", p.KernalROMPath, Kernal, KERNAL_ROM_SIZE, BuiltinKernalROM);
	load_rom("Char", p.CharROMPath, Char, CHAR_ROM_SIZE, BuiltinCharROM);
	load_rom("1541", p.DriveROMPath, ROM1541, DRIVE_ROM_SIZE, BuiltinDriveROM);
}


/*
 *  Initialize emulation memory
 */

void C64::init_memory()
{
	// Initialize RAM with powerup pattern
	// Sampled from a PAL C64 (Assy 250425) with Fujitsu MB8264A-15 DRAM chips
	uint8_t *p = RAM;
	for (unsigned i = 0; i < 512; ++i) {
		for (unsigned j = 0; j < 64; ++j) {
			if (j == 4 || j == 5) {
				*p++ = (i & 1) ? 0x03 : 0x01;	// Unstable
			} else if (j == 7) {
				*p++ = 0x07;					// Unstable
			} else if (j == 32 || j == 57 || j == 58) {
				*p++ = 0xff;
			} else if (j == 55) {
				*p++ = (i & 1) ? 0x07 : 0x05;	// Unstable
			} else if (j == 56) {
				*p++ = (i & 1) ? 0x2f : 0x27;
			} else if (j == 59) {
				*p++ = 0x10;
			} else if (j == 60) {
				*p++ = 0x05;
			} else {
				*p++ = 0x00;
			}
		}
		for (unsigned j = 0; j < 64; ++j) {
			if (j == 36) {
				*p++ = 0xfb;
			} else if (j == 63) {
				*p++ = (i & 1) ? 0xff : 0x7c;	// Unstable
			} else {
				*p++ = 0xff;
			}
		}
	}

	// Initialize color RAM with random values
	p = Color;
	for (unsigned i = 0; i < COLOR_RAM_SIZE; ++i) {
		*p++ = rand() & 0x0f;
	}

	// Clear 1541 RAM
	memset(RAM1541, 0, DRIVE_RAM_SIZE);
}


/*
 *  Start main emulation loop, returns with exit code
 */

int C64::Run()
{
	cycle_counter = 0;

	// Reset chips
	TheCPU->Reset();
	TheSID->Reset();
	TheCIA1->Reset();
	TheCIA2->Reset();
	TheCPU1541->Reset();
	TheGCRDisk->Reset();
	TheTape->Reset();

	// Remember start time of first frame
	frame_start = chrono::steady_clock::now();
	frame_skip_factor = 1;
	frame_skip_counter = 1;

	// Enter main loop
	return main_loop();
}


/*
 *  Request emulator to quit with given exit code
 */

void C64::RequestQuit(int exit_code)
{
	main_loop_exit_code = exit_code;
	quit_requested = true;
}


/*
 *  Request emulator to show prefs editor at next VBlank
 */

void C64::RequestPrefsEditor()
{
	prefs_editor_requested = true;
}


/*
 *  Request emulator to load snapshot at next VBlank
 */

void C64::RequestLoadSnapshot(const std::string & path)
{
	requested_snapshot = path;
	load_snapshot_requested = true;
}


/*
 *  Reset C64
 */

void C64::Reset(bool clear_memory)
{
	TheCPU->AsyncReset();
	TheCPU1541->AsyncReset();
	TheGCRDisk->Reset();
	TheTape->Reset();
	TheSID->Reset();
	TheCIA1->Reset();
	TheCIA2->Reset();
	TheIEC->Reset();
	TheCart->Reset();
	// Note: VIC has no reset input

	if (clear_memory) {
		init_memory();
	}

	reset_play_mode();
}


/*
 *  Reset C64 and auto-start from drive 8
 */

void C64::ResetAndAutoStart()
{
	patch_roms(ThePrefs.FastReset, ThePrefs.Emul1541Proc, true);

	Reset(true);
}


/*
 *  NMI C64
 */

void C64::NMI()
{
	TheCPU->AsyncNMI();
}


/*
 *  The preferences have changed. prefs is a pointer to the new
 *  preferences, ThePrefs still holds the previous ones.
 */

void C64::NewPrefs(const Prefs *prefs)
{
	open_close_joysticks(ThePrefs.Joystick1Port, ThePrefs.Joystick2Port, prefs->Joystick1Port, prefs->Joystick2Port);

	TheDisplay->NewPrefs(prefs);

	TheIEC->NewPrefs(prefs);
	TheGCRDisk->NewPrefs(prefs);
	TheTape->NewPrefs(prefs);

	TheSID->NewPrefs(prefs);

	auto old_roms = ThePrefs.SelectedROMPaths();
	auto new_roms = prefs->SelectedROMPaths();
	if (old_roms != new_roms) {
		load_rom_files(new_roms);
		Reset(true);	// Reset C64 if ROMs have changed
	}

	patch_roms(prefs->FastReset, prefs->Emul1541Proc, prefs->AutoStart);

	if (prefs->AutoStart) {
		Reset(true);	// Reset C64 if auto-start requested
	}

	swap_cartridge(ThePrefs.REUType, ThePrefs.CartridgePath, prefs->REUType, prefs->CartridgePath);
	TheCPU->SetChips(TheVIC, TheSID, TheCIA1, TheCIA2, TheCart, TheIEC, TheTape);

	// Reset 1541 processor if turned on or off (to bring IEC lines back to sane state)
	if (ThePrefs.Emul1541Proc != prefs->Emul1541Proc) {
		TheCPU1541->AsyncReset();
	}

	reset_play_mode();
}


/*
 *  Turn 1541 processor emulation on or off and set disk drive path
 */

void C64::MountDrive8(bool emul_1541_proc, const char * path)
{
	auto prefs = std::make_unique<Prefs>(ThePrefs);
	prefs->DrivePath[0] = path;
	prefs->Emul1541Proc = emul_1541_proc;
	NewPrefs(prefs.get());
	ThePrefs = *prefs;
}


/*
 *  Set tape drive path
 */

void C64::MountDrive1(const char * path)
{
	auto prefs = std::make_unique<Prefs>(ThePrefs);
	prefs->TapePath = path;
	NewPrefs(prefs.get());
	ThePrefs = *prefs;
}


/*
 *  Insert cartridge
 */

void C64::InsertCartridge(const std::string & path)
{
	auto prefs = std::make_unique<Prefs>(ThePrefs);
	prefs->CartridgePath = path;
	if (! path.empty()) {
		prefs->REUType = REU_NONE;	// Cartridge overrides REU
	}
	NewPrefs(prefs.get());
	ThePrefs = *prefs;
}


/*
 *  Patch kernal ROM reset and IEC routines
 */

static void apply_patch(bool apply, uint8_t * rom, const uint8_t * builtin, uint16_t offset, unsigned size, const uint8_t * patch)
{
	if (apply) {

		// Only apply patch if original data is present
		if (memcmp(rom + offset, builtin + offset, size) == 0) {
			memcpy(rom + offset, patch, size);
		}

	} else {

		// Only undo patch if patched data is present
		if (memcmp(rom + offset, patch, size) == 0) {
			memcpy(rom + offset, builtin + offset, size);
		}
	}
}

void C64::patch_roms(bool fast_reset, bool emul_1541_proc, bool auto_start)
{
	// Fast reset
	static const uint8_t fast_reset_patch[] = { 0xa0, 0x00 };
	static const uint8_t fast_reset_drive_patch_1[] = { 0xfb, 0x4c, 0xc9, 0xea };	// Skip zero page test, just clear
	static const uint8_t fast_reset_drive_patch_2[] = { 0x4c, 0xea, 0xea };			// Skip ROM test
	static const uint8_t fast_reset_drive_patch_3[] = { 0x4c, 0x22, 0xeb };			// Skip RAM test...
	static const uint8_t fast_reset_drive_patch_4[] = { 0xea, 0xea, 0xa9, 0x00 };	// ...just clear

	apply_patch(fast_reset, Kernal, BuiltinKernalROM, 0x1d84, sizeof(fast_reset_patch), fast_reset_patch);
	apply_patch(fast_reset, ROM1541, BuiltinDriveROM, 0x2ab1, sizeof(fast_reset_drive_patch_1), fast_reset_drive_patch_1);
	apply_patch(fast_reset, ROM1541, BuiltinDriveROM, 0x2ad1, sizeof(fast_reset_drive_patch_2), fast_reset_drive_patch_2);
	apply_patch(fast_reset, ROM1541, BuiltinDriveROM, 0x2b00, sizeof(fast_reset_drive_patch_3), fast_reset_drive_patch_3);
	apply_patch(fast_reset, ROM1541, BuiltinDriveROM, 0x2af2, sizeof(fast_reset_drive_patch_4), fast_reset_drive_patch_4);

	// IEC
	static const uint8_t iec_patch_1[] = { 0xf2, 0x00 };	// IECOut
	static const uint8_t iec_patch_2[] = { 0xf2, 0x01 };	// IECOutATN
	static const uint8_t iec_patch_3[] = { 0xf2, 0x02 };	// IECOutSec
	static const uint8_t iec_patch_4[] = { 0xf2, 0x03 };	// IECIn
	static const uint8_t iec_patch_5[] = { 0xf2, 0x04 };	// IECSetATN
	static const uint8_t iec_patch_6[] = { 0xf2, 0x05 };	// IECRelATN
	static const uint8_t iec_patch_7[] = { 0xf2, 0x06 };	// IECTurnaround
	static const uint8_t iec_patch_8[] = { 0xf2, 0x07 };	// IECRelease

	apply_patch(!emul_1541_proc, Kernal, BuiltinKernalROM, 0x0d40, sizeof(iec_patch_1), iec_patch_1);
	apply_patch(!emul_1541_proc, Kernal, BuiltinKernalROM, 0x0d23, sizeof(iec_patch_2), iec_patch_2);
	apply_patch(!emul_1541_proc, Kernal, BuiltinKernalROM, 0x0d36, sizeof(iec_patch_3), iec_patch_3);
	apply_patch(!emul_1541_proc, Kernal, BuiltinKernalROM, 0x0e13, sizeof(iec_patch_4), iec_patch_4);
	apply_patch(!emul_1541_proc, Kernal, BuiltinKernalROM, 0x0def, sizeof(iec_patch_5), iec_patch_5);
	apply_patch(!emul_1541_proc, Kernal, BuiltinKernalROM, 0x0dbe, sizeof(iec_patch_6), iec_patch_6);
	apply_patch(!emul_1541_proc, Kernal, BuiltinKernalROM, 0x0dcc, sizeof(iec_patch_7), iec_patch_7);
	apply_patch(!emul_1541_proc, Kernal, BuiltinKernalROM, 0x0e03, sizeof(iec_patch_8), iec_patch_8);

	// Auto start after reset
	static const uint8_t auto_start_patch[] = { 0xf2, 0x10 };	// BASIC interactive input loop

	apply_patch(auto_start, Basic, BuiltinBasicROM, 0x0560, sizeof(auto_start_patch), auto_start_patch);

	// 1541
	static const uint8_t drive_patch_1[] = { 0xea, 0xea };						// Don't check ROM checksum
	static const uint8_t drive_patch_2[] = { 0xf2, 0x00 };						// DOS idle loop
	static const uint8_t drive_patch_3[] = { 0x20, 0xf2, 0xf5, 0xf2, 0x01 };	// Write sector
	static const uint8_t drive_patch_4[] = { 0xf2, 0x02 };						// Format track

	apply_patch(true, ROM1541, BuiltinDriveROM, 0x2ae4, sizeof(drive_patch_1), drive_patch_1);
	apply_patch(true, ROM1541, BuiltinDriveROM, 0x2ae8, sizeof(drive_patch_1), drive_patch_1);
	apply_patch(true, ROM1541, BuiltinDriveROM, 0x2c9b, sizeof(drive_patch_2), drive_patch_2);
	apply_patch(true, ROM1541, BuiltinDriveROM, 0x3594, sizeof(drive_patch_3), drive_patch_3);
	apply_patch(true, ROM1541, BuiltinDriveROM, 0x3b0c, sizeof(drive_patch_4), drive_patch_4);
}


/*
 *  Change attached cartridge according to old and new preferences
 */

void C64::swap_cartridge(int oldreu, const std::string & oldpath, int newreu, const std::string & newpath)
{
	if (oldreu == newreu && oldpath == newpath)
		return;

	Cartridge * new_cart = nullptr;

	// Create new cartridge object
	if (newreu == REU_NONE) {

		if (! newpath.empty()) {
			std::string error;
			new_cart = Cartridge::FromFile(newpath, error);

			if (new_cart) {
				ShowNotification("Cartridge inserted");	// This message is good to have on screen even on startup
				Reset();					// Reset C64 if new cartridge inserted
			} else {
				ShowNotification(error);	// Keep old cartridge on error
			}

		} else {

			new_cart = new NoCartridge;
			if (oldreu == REU_NONE) {
				ShowNotification("Cartridge removed");
			}
		}

	} else if (newreu == REU_GEORAM) {
		new_cart = new GeoRAM;
	} else {
		new_cart = new REU(TheCPU, newreu);
	}

	// Swap cartridge object if successful
	if (new_cart) {
		delete TheCart;
		TheCart = new_cart;
	}
}


#ifdef FRODO_SC

/*
 *  Emulate one cycle of the C64.
 *  Returns true if a new video frame has started.
 */

bool C64::emulate_c64_cycle()
{
	// The order of calls is important here
	unsigned flags = TheVIC->EmulateCycle();
	if (flags & VIC_HBLANK) {
		TheSID->EmulateLine();
	}
	TheCIA1->EmulateCycle();
	TheCIA2->EmulateCycle();
	TheCPU->EmulateCycle();
	TheTape->EmulateCycle();

	++cycle_counter;

	return flags & VIC_VBLANK;
}


/*
 *  Emulate one cycle of the 1541.
 */

void C64::emulate_1541_cycle()
{
	TheCPU1541->EmulateVIACycle();
	if (!TheCPU1541->Idle) {
		TheCPU1541->EmulateCPUCycle();
	}
}

#endif // def FRODO_SC


/*
 *  Pause emulator
 */

void C64::pause()
{
	TheSID->PauseSound();
	TheDisplay->Pause();
}


/*
 *  Resume emulator
 */

void C64::resume()
{
	TheDisplay->Resume();
	TheSID->ResumeSound();

	// Flush event queue
	SDL_PumpEvents();
	SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

	// Restart frame timing
	frame_start = chrono::steady_clock::now();
}


/*
 *  Vertical blank: Poll input devices, update display
 */

void C64::vblank()
{
	// Handle single-frame controls
	if (play_mode == PlayMode::RequestPause) {
		play_mode = PlayMode::Pause;
	} else if (play_mode == PlayMode::RewindFrame || play_mode == PlayMode::ForwardFrame) {
		play_mode = PlayMode::Pause;
	}

	// Poll keyboard and joysticks
	poll_input();

	// Handle request for prefs editor
	if (prefs_editor_requested) {
		pause();
		if (! TheApp->RunPrefsEditor()) {
			quit_requested = true;
			return;
		}
		resume();
		prefs_editor_requested = false;
	}

	// Handle request for snapshot loading
	if (load_snapshot_requested) {
		std::string error;
		if (LoadSnapshot(requested_snapshot, &ThePrefs, error)) {
			ShowNotification("Snapshot loaded");
		} else {
			ShowNotification(error);
		}
		load_snapshot_requested = false;
	}

	// Count TOD clocks
	if (play_mode != PlayMode::Pause) {
		TheCIA1->CountTOD();
		TheCIA2->CountTOD();
	}

	// Update window if needed
	--frame_skip_counter;
	if (frame_skip_counter == 0) {
		frame_skip_counter = frame_skip_factor;
	}
	if (frame_skip_counter == 1) {
    	TheDisplay->Update();
	}

	// Handle rewind feature
	handle_rewind();

	// Calculate time between frames, display speedometer
	chrono::time_point<chrono::steady_clock> now = chrono::steady_clock::now();

	int elapsed_us = chrono::duration_cast<chrono::microseconds>(now - frame_start).count();
	int speed_index = FRAME_TIME_us / double(elapsed_us + 1) * 100;

	// Limit speed to 100% (and FPS to 50 Hz) if desired
	if ((elapsed_us < FRAME_TIME_us) && ThePrefs.LimitSpeed) {
		std::this_thread::sleep_until(frame_start);
		if (play_mode == PlayMode::Forward) {
			frame_start += chrono::microseconds(FRAME_TIME_us / FORWARD_SCALE);
			frame_skip_factor = FORWARD_SCALE;
		} else {
			frame_start += chrono::microseconds(FRAME_TIME_us);
			frame_skip_factor = frame_skip_counter = 1;
		}
		speed_index = 100;	// Hide speed display even in fast-forwarding mode
	} else {
		frame_start = now;
		if (speed_index > 100) {
			frame_skip_factor = speed_index / 100;
		} else {
			frame_skip_factor = frame_skip_counter = 1;
		}
	}

	TheDisplay->SetSpeedometer(speed_index);
}


/*
 *  The emulation's main loop
 */

int C64::main_loop()
{
	unsigned prev_raster_y = 0;

	while (true) {
		bool new_frame;

		// Stop emulation in pause mode, just update the display
		if (play_mode == PlayMode::Pause) {
			vblank();
			if (quit_requested) {
				break;
			} else {
				continue;
			}
		}

#ifdef FRODO_SC

		new_frame = emulate_c64_cycle();
		if (ThePrefs.Emul1541Proc) {
			emulate_1541_cycle();
		}

#else

		// The order of calls is important here
		int cycles = 0;
		unsigned flags = TheVIC->EmulateLine(cycles);
		new_frame = (flags & VIC_VBLANK);

		TheSID->EmulateLine();
#if !PRECISE_CIA_CYCLES
		TheCIA1->EmulateLine(ThePrefs.CIACycles);
		TheCIA2->EmulateLine(ThePrefs.CIACycles);
#endif

		if (ThePrefs.Emul1541Proc) {
			int cycles_1541 = ThePrefs.FloppyCycles;
			TheCPU1541->CountVIATimers(cycles_1541);

			if (!TheCPU1541->Idle) {
				// 1541 processor active, alternately execute
				//  6502 and 6510 instructions until both have
				//  used up their cycles
				while (cycles >= 0 || cycles_1541 >= 0) {
					if (cycles > cycles_1541) {
						cycles -= TheCPU->EmulateLine(1);
					} else {
						int used = TheCPU1541->EmulateLine(1);
						cycles_1541 -= used;
						cycle_counter += used;	// Needed for GCR timing
					}
				}
			} else {
				TheCPU->EmulateLine(cycles);
				cycle_counter += CYCLES_PER_LINE;
			}
		} else {
			// 1541 processor disabled, only emulate 6510
			TheCPU->EmulateLine(cycles);
			cycle_counter += CYCLES_PER_LINE;
		}

#endif  // def FRODO_SC

		// Poll keyboard and mouse, and delay execution at three points
		// within the frame to reduce input lag. This also helps with the
		// asynchronously running SID emulation.
		if (ThePrefs.LimitSpeed && play_mode == PlayMode::Play) {
			unsigned raster_y = TheVIC->RasterY();
			if (raster_y != prev_raster_y) {
				if (raster_y == TOTAL_RASTERS * 1 / 4) {
					std::this_thread::sleep_until(frame_start - chrono::microseconds(FRAME_TIME_us * 3 / 4));
					poll_input();
				} else if (raster_y == TOTAL_RASTERS * 2 / 4) {
					std::this_thread::sleep_until(frame_start - chrono::microseconds(FRAME_TIME_us * 2 / 4));
					poll_input();
				} else if (raster_y == TOTAL_RASTERS * 3 / 4) {
					std::this_thread::sleep_until(frame_start - chrono::microseconds(FRAME_TIME_us * 1 / 4));
					poll_input();
				}

				prev_raster_y = raster_y;
			}
		}

		// Update display etc. if new frame has started
		if (new_frame) {
			vblank();

			// Exit if requested
			if (quit_requested) {
				break;
			}

			// Exit with code 1 if automated test time has elapsed
			if (ThePrefs.TestMaxFrames > 0) {
				--ThePrefs.TestMaxFrames;
				if (ThePrefs.TestMaxFrames == 0) {
					main_loop_exit_code = 1;
					quit_requested = true;
				}
			}
		}
	}

	return main_loop_exit_code;
}


/*
 *  Poll keyboard and joysticks
 */

void C64::poll_input()
{
	// Poll joysticks
	TheCIA1->Joystick1 = poll_joystick(0);
	TheCIA1->Joystick2 = poll_joystick(1);

	if (ThePrefs.JoystickSwap) {
		std::swap(TheCIA1->Joystick1, TheCIA1->Joystick2);
	}

	// Poll keyboard
	TheDisplay->PollKeyboard(TheCIA1->KeyMatrix, TheCIA1->RevMatrix, &joykey);

	// Joystick keyboard emulation
	if (TheDisplay->NumLock()) {
		TheCIA1->Joystick1 &= joykey;
	} else {
		TheCIA1->Joystick2 &= joykey;
	}
}


/*
 *  Open/close joystick drivers given old and new state of
 *  joystick preferences
 */

void C64::open_close_joystick(int port, int oldjoy, int newjoy)
{
	if (oldjoy != newjoy) {
		if (newjoy > 0) {
			int index = newjoy - 1;
			joy[port] = SDL_JoystickOpen(index);
			if (joy[port] == nullptr) {
				fprintf(stderr, "WARNING: Cannot open joystick %d: %s\n", port + 1, SDL_GetError());
			} else if (SDL_IsGameController(index)) {
				controller[port] = SDL_GameControllerOpen(index);
			}
		} else {
			if (controller[port]) {
				SDL_GameControllerClose(controller[port]);
				controller[port] = nullptr;
			}
			if (joy[port]) {
				SDL_JoystickClose(joy[port]);
				joy[port] = nullptr;
			}
		}
	}
}

void C64::open_close_joysticks(int oldjoy1, int oldjoy2, int newjoy1, int newjoy2)
{
	open_close_joystick(0, oldjoy1, newjoy1);
	open_close_joystick(1, oldjoy2, newjoy2);
}


/*
 *  Game controller added
 */

void C64::JoystickAdded(int32_t index)
{
	// Assign to port 2 first, then to port 1
	if (joy[1] == nullptr && ThePrefs.Joystick1Port != index + 1) {

		ThePrefs.Joystick2Port = index + 1;
		open_close_joystick(1, 0, ThePrefs.Joystick2Port);
		ShowNotification("Controller assigned to port 2");

	} else if (joy[0] == nullptr && ThePrefs.Joystick2Port != index + 1) {

		ThePrefs.Joystick1Port = index + 1;
		open_close_joystick(0, 0, ThePrefs.Joystick1Port);
		ShowNotification("Controller assigned to port 1");
	}
}


/*
 *  Game controller removed
 */

void C64::JoystickRemoved(int32_t instance_id)
{
	if (joy[0] && SDL_JoystickInstanceID(joy[0]) == instance_id) {

		// Unassign joystick port 1
		open_close_joystick(0, ThePrefs.Joystick1Port, 0);
		ThePrefs.Joystick1Port = 0;
		ShowNotification("Controller on port 1 removed");

	} else if (joy[1] && SDL_JoystickInstanceID(joy[1]) == instance_id) {

		// Unassign joystick port 2
		open_close_joystick(1, ThePrefs.Joystick2Port, 0);
		ThePrefs.Joystick2Port = 0;
		ShowNotification("Controller on port 2 removed");
	}
}


/*
 *  Poll joystick port, return CIA mask
 */

uint8_t C64::poll_joystick(int port)
{
	uint8_t j = 0xff;

	int x = 0;
	int y = 0;

	if (controller[port]) {

		// Use Game Controller API
		if (SDL_GameControllerGetButton(controller[port], SDL_CONTROLLER_BUTTON_DPAD_LEFT)) {
			j &= 0xfb;							// Left
		}
		if (SDL_GameControllerGetButton(controller[port], SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) {
			j &= 0xf7;							// Right
		}
		if (SDL_GameControllerGetButton(controller[port], SDL_CONTROLLER_BUTTON_DPAD_UP)) {
			j &= 0xfe;							// Up
		}
		if (SDL_GameControllerGetButton(controller[port], SDL_CONTROLLER_BUTTON_DPAD_DOWN)) {
			j &= 0xfd;							// Down
		}
		if (SDL_GameControllerGetButton(controller[port], SDL_CONTROLLER_BUTTON_A)) {
			j &= 0xef;							// Button
		}

		// Left trigger controls rewind
		int trigger = SDL_GameControllerGetAxis(controller[port], SDL_CONTROLLER_AXIS_TRIGGERLEFT);
		if (trigger > joy_maxtrigl[port]) {
			if (! joy_trigl_on[port]) {
				if (GetPlayMode() == PlayMode::Play) {
					SetPlayMode(PlayMode::Rewind);
				}
				joy_trigl_on[port] = true;
			}
			joy_maxtrigl[port] = +(JOYSTICK_DEAD_ZONE - JOYSTICK_HYSTERESIS);
		} else {
			if (joy_trigl_on[port]) {
				if (GetPlayMode() == PlayMode::Rewind) {
					SetPlayMode(PlayMode::Play);
				}
				joy_trigl_on[port] = false;
			}
			joy_maxtrigl[port] = +JOYSTICK_DEAD_ZONE;
		}

		// Right trigger controls fast-forward
		trigger = SDL_GameControllerGetAxis(controller[port], SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
		if (trigger > joy_maxtrigr[port]) {
			if (! joy_trigr_on[port]) {
				if (GetPlayMode() == PlayMode::Play) {
					SetPlayMode(PlayMode::Forward);
				}
				joy_trigr_on[port] = true;
			}
			joy_maxtrigr[port] = +(JOYSTICK_DEAD_ZONE - JOYSTICK_HYSTERESIS);
		} else {
			if (joy_trigr_on[port]) {
				if (GetPlayMode() == PlayMode::Forward) {
					SetPlayMode(PlayMode::Play);
				}
				joy_trigr_on[port] = false;
			}
			joy_maxtrigr[port] = +JOYSTICK_DEAD_ZONE;
		}

		// Left stick is an alternative to D-pad
		x = SDL_GameControllerGetAxis(controller[port], SDL_CONTROLLER_AXIS_LEFTX);
		y = SDL_GameControllerGetAxis(controller[port], SDL_CONTROLLER_AXIS_LEFTY);

		// Control rumble effects
		if (ThePrefs.TapeRumble) {
			if (TheTape->MotorOn()) {
				SDL_GameControllerRumble(controller[port], 0, 0x8000, 1000 / SCREEN_FREQ);
			} else {
				SDL_GameControllerRumble(controller[port], 0, 0, 1000 / SCREEN_FREQ);
			}
		}

	} else if (joy[port]) {

		// Not a Game Controller, use joystick API
		if (SDL_JoystickGetButton(joy[port], 0)) {
			j &= 0xef;							// Button
		}

		x = SDL_JoystickGetAxis(joy[port], 0);
		y = SDL_JoystickGetAxis(joy[port], 1);
	}

	// In twin-stick mode, axes are controlled by right stick on opposite port
	if (ThePrefs.TwinStick) {
		if (controller[port ^ 1]) {
			x = SDL_GameControllerGetAxis(controller[port ^ 1], SDL_CONTROLLER_AXIS_RIGHTX);
			y = SDL_GameControllerGetAxis(controller[port ^ 1], SDL_CONTROLLER_AXIS_RIGHTY);
		}
	}

	// Convert analog axes to digital directions
	if (x < joy_minx[port]) {
		j &= 0xfb;							// Left
		joy_minx[port] = -(JOYSTICK_DEAD_ZONE - JOYSTICK_HYSTERESIS);
	} else {
		joy_minx[port] = -JOYSTICK_DEAD_ZONE;
	}

	if (x > joy_maxx[port]) {
		j &= 0xf7;							// Right
		joy_maxx[port] = +(JOYSTICK_DEAD_ZONE - JOYSTICK_HYSTERESIS);
	} else {
		joy_maxx[port] = +JOYSTICK_DEAD_ZONE;
	}

	if (y < joy_miny[port]) {
		j &= 0xfe;							// Up
		joy_miny[port] = -(JOYSTICK_DEAD_ZONE - JOYSTICK_HYSTERESIS);
	} else {
		joy_miny[port] = -JOYSTICK_DEAD_ZONE;
	}

	if (y > joy_maxy[port]) {
		j &= 0xfd;							// Down
		joy_maxy[port] = +(JOYSTICK_DEAD_ZONE - JOYSTICK_HYSTERESIS);
	} else {
		joy_maxy[port] = +JOYSTICK_DEAD_ZONE;
	}

	return j;
}


/*
 *  Tape button pressed
 */

void C64::SetTapeButtons(TapeState pressed)
{
	TheCPU->SetTapeSense(pressed != TapeState::Stop);
	TheTape->SetButtons(pressed);
}


/*
 *  Tape PLAY button on game controller pressed or released
 */

void C64::SetTapeControllerButton(bool pressed)
{
	TheCPU->SetTapeSense(pressed);
	// Don't mess with the actual Datasette emulation
}


/*
 *  Rewind tape to start
 */

void C64::RewindTape()
{
	TheTape->Rewind();
}


/*
 *  Forward tape to end
 */

void C64::ForwardTape()
{
	TheTape->Forward();
}


/*
 *  Return which tape button is pressed
 */

TapeState C64::TapeButtonState() const
{
	return TheTape->ButtonState();
}


/*
 *  Return whether tape is playing or recording
 */

TapeState C64::TapeDriveState() const
{
	return TheTape->DriveState();
}


/*
 *  Return tape position in percent
 */

int C64::TapePosition() const
{
	return TheTape->TapePosition();
}


/*
 *  Save state to snapshot (emulation must be in VBlank)
 */

void C64::MakeSnapshot(Snapshot * s, bool instruction_boundary)
{
	memset(s, 0, sizeof(*s));

	memcpy(s->magic, SNAPSHOT_HEADER, sizeof(s->magic));

	if (ThePrefs.DrivePath[0].length() < sizeof(s->drive8Path)) {
		strcpy(s->drive8Path, ThePrefs.DrivePath[0].c_str());
	}

#ifdef FRODO_SC
	while (true) {
		TheCPU->GetState(&(s->cpu));

		if (s->cpu.instruction_complete || !instruction_boundary)
			break;

		// Advance C64 state by one cycle
		emulate_c64_cycle();
		if (ThePrefs.Emul1541Proc) {
			emulate_1541_cycle();
		}
	}
#else
	TheCPU->GetState(&(s->cpu));
#endif
	s->cycleCounter = cycle_counter;

	TheVIC->GetState(&(s->vic));
	TheSID->GetState(&(s->sid));
	TheCIA1->GetState(&(s->cia1));
	TheCIA2->GetState(&(s->cia2));

	memcpy(s->ram, RAM, C64_RAM_SIZE);
	memcpy(s->color, Color, COLOR_RAM_SIZE);

	if (ThePrefs.Emul1541Proc) {
		s->flags |= SNAPSHOT_FLAG_1541_PROC;

#ifdef FRODO_SC
		while (true) {
			TheCPU1541->GetState(&(s->driveCPU));

			if (s->driveCPU.idle || s->driveCPU.instruction_complete || !instruction_boundary)
				break;

			// Advance 1541 state by one cycle
			emulate_1541_cycle();
		}
#else
		TheCPU1541->GetState(&(s->driveCPU));
#endif
	}

	TheGCRDisk->GetState(&(s->driveGCR));

	TheTape->GetState(&(s->tape));

	memcpy(s->driveRam, RAM1541, DRIVE_RAM_SIZE);
}


/*
 *  Restore state from snapshot (emulation must be paused and in VBlank)
 *
 *  Note: The magic header is not checked by this function.
 */

void C64::RestoreSnapshot(const Snapshot * s)
{
	// SL CPU64::SetState() overwrites ram[0/1], so we need to restore that
	// first in case we're loading an SC snapshot
	memcpy(RAM, s->ram, C64_RAM_SIZE);
	memcpy(Color, s->color, COLOR_RAM_SIZE);

	cycle_counter = s->cycleCounter;

	TheCPU->SetState(&(s->cpu));
	TheVIC->SetState(&(s->vic));
	TheSID->SetState(&(s->sid));
	TheCIA1->SetState(&(s->cia1));
	TheCIA2->SetState(&(s->cia2));

	if (s->flags & SNAPSHOT_FLAG_1541_PROC) {

		memcpy(RAM1541, s->driveRam, DRIVE_RAM_SIZE);

		TheCPU1541->SetState(&(s->driveCPU));
		TheGCRDisk->SetState(&(s->driveGCR));
	}

	TheTape->SetState(&(s->tape));
}


/*
 *  Save snapshot file (emulation must be paused and in VBlank)
 */

bool C64::SaveSnapshot(const std::string & filename, std::string & ret_error_msg)
{
	FILE * f = fopen(filename.c_str(), "wb");
	if (f == nullptr) {
		ret_error_msg = "Can't create snapshot file";
		return false;
	}

	// To be able to use SC snapshots with SL, the state of the SC C64 and 1541
	// CPUs are not saved in the middle of an instruction. Instead the state is
	// advanced cycle by cycle until the current instruction has finished.
	auto s = std::make_unique<Snapshot>();
	MakeSnapshot(s.get(), true);

	// TODO: Endianess and alignment should be taken care of
	// to make snapshot files portable

	if (fwrite(s.get(), sizeof(Snapshot), 1, f) != 1) {
		ret_error_msg = "Error writing to snapshot file";
		fclose(f);
		return false;
	}

	fclose(f);
	return true;
}


/*
 *  Load snapshot file (emulation must be paused and in VBlank)
 */

bool C64::LoadSnapshot(const std::string & filename, Prefs * prefs, std::string & ret_error_msg)
{
	FILE * f = fopen(filename.c_str(), "rb");
	if (f == nullptr) {
		ret_error_msg = "Can't open snapshot file";
		return false;
	}

	auto s = std::make_unique<Snapshot>();

	if (fread(s.get(), sizeof(Snapshot), 1, f) != 1) {
		ret_error_msg = "Error reading snapshot file";
		fclose(f);
		return false;
	}

	fclose(f);

	if (memcmp(s->magic, SNAPSHOT_HEADER, sizeof(s->magic)) != 0) {
		ret_error_msg = "Not a Frodo snapshot file";
		return false;
	}

	// Restore prefs from snapshot (before restoring state, to avoid
	// spurious 1541 resets after restoring)
	auto new_prefs = std::make_unique<Prefs>(*prefs);
	new_prefs->Emul1541Proc = s->flags & SNAPSHOT_FLAG_1541_PROC;
	new_prefs->DrivePath[0] = s->drive8Path;
	NewPrefs(new_prefs.get());
	ThePrefs = *new_prefs;
	if (prefs != &ThePrefs) {
		prefs->Emul1541Proc = new_prefs->Emul1541Proc;
		prefs->DrivePath[0] = new_prefs->DrivePath[0];
	}

	RestoreSnapshot(s.get());

	reset_play_mode();
	return true;
}


/*
 *  Load C64 program directly into RAM
 */

bool C64::DMALoad(const std::string & filename, std::string & ret_error_msg)
{
	// Open file
	FILE * f = fopen(filename.c_str(), "rb");
	if (f == nullptr) {
		ret_error_msg = "Can't open program file";
		return false;
	}

	// Read load address
	uint8_t header[2];
	if (fread(header, sizeof(header), 1, f) != 1) {
		ret_error_msg = "Error reading program file";
		return false;
	}

	// Load up to end of RAM
	uint16_t load_addr = (header[1] << 8) | header[0];
	uint16_t num_bytes = fread(RAM + load_addr, 1, C64_RAM_SIZE - load_addr, f);
	uint16_t end_addr = load_addr + num_bytes;

	// Set Kernal and BASIC pointers to simulate LOAD command
	RAM[0x90] = 0x40;						// STATUS = end of file
	RAM[0xba] = 8;							// FA, current device number (8 = first disk)
	RAM[0xae] = end_addr & 0xff;			// EAL/EAH, end address of LOAD
	RAM[0xaf] = end_addr >> 8;
	if (load_addr == 0x0801) {
		RAM[0x2d] = end_addr & 0xff;		// VARTAB, start of variable table (= end of program)
		RAM[0x2e] = end_addr >> 8;
		RAM[0x2f] = RAM[0x31] = RAM[0x2d];	// ARYTAB/STREND, start of array storage and free RAM
		RAM[0x30] = RAM[0x32] = RAM[0x2e];
		RAM[0x33] = RAM[0x37];				// FRETOP = MEMSIZ, clear string area
		RAM[0x34] = RAM[0x38];
		RAM[0x7a] = (load_addr - 1) & 0xff;	// TXTPTR, start of BASIC text
		RAM[0x7b] = (load_addr - 1) >> 8;
		RAM[0x41] = RAM[0x7a];				// DATPTR, current DATA item
		RAM[0x42] = RAM[0x7b];
	}

	return true;
}


/*
 *  Auto start first program from drive 8 or program specified by preferences
 *  (called from BASIC interactive input loop)
 */

void C64::AutoStartOp()
{
	// Remove ROM patch to avoid recursion
	patch_roms(ThePrefs.FastReset, ThePrefs.Emul1541Proc, ThePrefs.AutoStart = false);

	if (! ThePrefs.LoadProgram.empty() ) {

		// Load specified program
		std::string error_msg;
		if (! DMALoad(ThePrefs.LoadProgram, error_msg)) {
			fprintf(stderr, "Unable to auto-start: %s\n", error_msg.c_str());
			return;
		}

		// Put RUN <RETURN> into keyboard buffer
		set_keyboard_buffer("RUN\x0d");

	} else if (! ThePrefs.DrivePath[0].empty()) {

		// Starting from drive 8, write LOAD command to screen
		write_to_screen("load\"*\",8,1");

		// Put <RETURN> RUN <RETURN> into keyboard buffer
		set_keyboard_buffer("\x0dRUN\x0d");

	} else if (! ThePrefs.TapePath.empty()) {

		// Starting from drive 1, write LOAD command to screen
		//
		// Some alternative Kernals set the default device number to 8,
		// so we specify it explicitly here.
		write_to_screen("load\"\",1");

		// Put <RETURN> RUN <RETURN> into keyboard buffer
		set_keyboard_buffer("\x0dRUN\x0d");

		// Rewind and press PLAY on tape
		RewindTape();
		SetTapeButtons(TapeState::Play);
	}
}


/*
 *  Write string to C64 screen memory
 */

void C64::write_to_screen(const char * str)
{
	uint16_t pnt = RAM[0xd1] | (RAM[0xd2] << 8);	// Pointer to current screen line

	while (true) {
		uint8_t c = *str++;
		if (c == '\0')
			break;

		// Convert ASCII to screen code
		if (c == '@') {
			c = 0x00;
		} else if ((c >= 'a') && (c <= 'z')) {
			c ^= 0x60;
		}

		RAM[pnt++] = c;
	}
}


/*
 *  Write string (10 characters max.) to C64 keyboard buffer
 */

void C64::set_keyboard_buffer(const char * str)
{
	uint16_t keyd = 0x277;	// Keyboard buffer

	size_t i = 0;
	while (true) {
		uint8_t c = str[i];
		if (c == '\0')
			break;

		RAM[keyd++] = c;
		++i;
	}

	RAM[0xc6] = i;	// Number of characters
}


/*
 *  Stop rewind/forward mode and clear rewind buffer
 */

void C64::reset_play_mode()
{
	SetPlayMode(PlayMode::Play);
	rewind_start = 0;
	rewind_fill = 0;
}


/*
 *  Set rewind/forward mode
 */

void C64::SetPlayMode(PlayMode mode)
{
	play_mode = mode;
}


/*
 *  Handle rewind recording and replay (to be called in VBlank)
 */

void C64::handle_rewind()
{
	if (rewind_buffer != nullptr) {
		if (play_mode == PlayMode::Rewind || play_mode == PlayMode::RewindFrame) {

			// Pop snapshot from ring buffer
			if (rewind_fill > 0) {
				size_t read_index = (rewind_start + rewind_fill - 1) % REWIND_LENGTH;
				RestoreSnapshot(rewind_buffer + read_index);

				// Keep first snapshot in buffer so we can repeat it when
				// reaching the end of the buffer
				if (rewind_fill > 1) {
					--rewind_fill;
				}
			}

		} else if (play_mode == PlayMode::Play || play_mode == PlayMode::Forward || play_mode == PlayMode::ForwardFrame) {

			// Add snapshot to ring buffer
			size_t write_index = (rewind_start + rewind_fill) % REWIND_LENGTH;
			MakeSnapshot(rewind_buffer + write_index);

			if (rewind_fill < REWIND_LENGTH) {
				++rewind_fill;
			} else {
				rewind_start = (rewind_start + 1) % REWIND_LENGTH;
			}
		}
	}
}


/*
 *  Check whether file is a snapshot file
 */

bool IsSnapshotFile(const char * filename)
{
	FILE * f = fopen(filename, "rb");
	if (f == nullptr)
		return false;

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (size != sizeof(Snapshot))
		return false;

	uint8_t magic[sizeof(Snapshot::magic)];
	memset(magic, 0, sizeof(magic));
	fread(magic, sizeof(magic), 1, f);
	fclose(f);

	return memcmp(magic, SNAPSHOT_HEADER, sizeof(magic)) == 0;
}


/*
 *  Set drive LEDs (forward to display)
 */

void C64::SetDriveLEDs(int l0, int l1, int l2, int l3)
{
	TheDisplay->SetLEDs(l0, l1, l2, l3);
}


/*
 *  Show notification to user (forward to display)
 */

void C64::ShowNotification(std::string s)
{
	TheDisplay->ShowNotification(s);
}


/*
 *  Convert C64 keycodes from/to strings
 */

static const char * c64_key_names[NUM_C64_KEYCODES] = {
	"INS/DEL",
	"RETURN",
	"CRSR ←→",
	"F7",
	"F1",
	"F3",
	"F5",
	"CRSR ↑↓",

	"3",
	"W",
	"A",
	"4",
	"Z",
	"S",
	"E",
	"SHIFT (Left)",

	"5",
	"R",
	"D",
	"6",
	"C",
	"F",
	"T",
	"X",

	"7",
	"Y",
	"G",
	"8",
	"B",
	"H",
	"U",
	"V",

	"9",
	"I",
	"J",
	"0",
	"M",
	"K",
	"O",
	"N",

	"+",
	"P",
	"L",
	"-",
	".",
	":",
	"@",
	",",

	"£",
	"*",
	";",
	"CLR/HOME",
	"SHIFT (Right)",
	"=",
	"↑",
	"/",

	"1",
	"←",
	"CONTROL",
	"2",
	"SPACE",
	"C=",
	"Q",
	"RUN/STOP",

	"PLAY",	// KEYCODE_PLAY_ON_TAPE
};

int KeycodeFromString(const std::string & s)
{
	for (int i = 0; i < NUM_C64_KEYCODES; ++i) {
		if (s == c64_key_names[i]) {
			return i;
		}
	}

	return -1;
}

const char * StringForKeycode(unsigned kc)
{
	if (kc < NUM_C64_KEYCODES) {
		return c64_key_names[kc];
	} else {
		return "";
	}
}
