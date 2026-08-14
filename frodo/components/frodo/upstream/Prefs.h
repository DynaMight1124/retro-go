/*
 *  Prefs.h - Global preferences
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

#ifndef PREFS_H
#define PREFS_H

#include <filesystem>
#include <map>
#include <string>


// SID types
enum {
	SIDTYPE_NONE,			// SID emulation off
	SIDTYPE_DIGITAL_6581,	// Digital SID emulation (6581)
	SIDTYPE_DIGITAL_8580,	// Digital SID emulation (8580)
	SIDTYPE_SIDCARD			// SID card
};


// RAM expansion types
enum {
	REU_NONE,		// No REU
	REU_128K,		// 128K REU
	REU_256K,		// 256K REU
	REU_512K,		// 512K REU
	REU_GEORAM		// 512K GeoRAM
};


// Display types
enum {
	DISPTYPE_WINDOW,	// Window
	DISPTYPE_SCREEN		// Fullscreen
};


// Color palettes
enum {
	PALETTE_PEPTO,
	PALETTE_COLODORE
};


// Set of fimware ROM paths
struct ROMPaths {
	auto operator<=>(const ROMPaths &) const = default;

	std::string BasicROMPath;	// Path for BASIC ROM
	std::string KernalROMPath;	// Path for Kernal ROM
	std::string CharROMPath;	// Path for Char ROM
	std::string DriveROMPath;	// Path for Drive ROM
};


// Controller button mapping (map from SDL_GameControllerButton to C64 keycode)
using ButtonMapping = std::map<unsigned, unsigned>;


// Preferences data
class Prefs {
public:
	Prefs();

	bool ShowEditor(bool startup, std::filesystem::path prefs_path, std::filesystem::path snapshot_path);
	void Check();
	void Load(std::filesystem::path prefs_path);
	bool Save(std::filesystem::path prefs_path);
	void ParseItem(std::string item);
	ROMPaths SelectedROMPaths() const;
	ButtonMapping SelectedButtonMapping() const;

	int NormalCycles;			// Available CPU cycles in normal raster lines
	int BadLineCycles;			// Available CPU cycles in Bad Lines
	int CIACycles;				// CIA timer ticks per raster line
	int FloppyCycles;			// Available 1541 CPU cycles per line

	std::string DrivePath[4];	// Path for drive 8..11
	std::string TapePath;		// Path for drive 1

	int SIDType;				// SID emulation type
	int REUType;				// Type of RAM expansion
	int DisplayType;			// Display type (windowed or full-screen)
	int Palette;				// Color palette to use
	int Joystick1Port;			// Port that joystick 1 is connected to (0 = no joystick, all other values are system dependant)
	int Joystick2Port;			// Port that joystick 2 is connected to
	int ScalingNumerator;		// Window scaling numerator
	int ScalingDenominator;		// Window scaling denominator
	int TestMaxFrames;			// Maximum number of frames to run in test-bench mode (not saved to preferences file)

	bool SpriteCollisions;		// Sprite collision detection is on
	bool JoystickSwap;			// Swap joysticks 1<->2
	bool TwinStick;				// Twin-stick control
	bool TapeRumble;			// Tape motor controller rumble
	bool LimitSpeed;			// Limit speed to 100%
	bool FastReset;				// Skip RAM test on reset
	bool CIAIRQHack;			// Write to CIA ICR clears IRQ
	bool MapSlash;				// Map '/' in C64 filenames
	bool Emul1541Proc;			// Enable processor-level 1541 emulation
	bool ShowLEDs;				// Show status bar
	bool AutoStart;				// Auto-start from drive 8 after reset (not saved to preferences file)
	bool TestBench;				// Enable features for automatic regression tests (not saved to preferences file)

	std::string LoadProgram;	// BASIC program file to load in conjunction with AutoStart (not saved to preferences file)

	std::map<std::string, ROMPaths> ROMSetDefs;	// Defined ROM sets, indexed by name
	std::string ROMSet;			// Name of selected ROM set (empty = built-in)

	std::map<std::string, ButtonMapping> ButtonMapDefs;	// Defined button mappings, indexed by name
	std::string ButtonMap;		// Name of selected controller button mapping

	std::string CartridgePath;	// Path for cartridge image file

	std::string TestScreenshotPath;	// Path for screenshot to be saved on exit in test-bench mode (not saved to preferences file)
};


// These are the active preferences
extern Prefs ThePrefs;


#endif // ndef PREFS_H
