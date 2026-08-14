/*
 *  main.h - Main program
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

#ifndef MAIN_H
#define MAIN_H

#include <filesystem>
#include <vector>


class C64;


/*
 *  Main application object
 */

class Frodo {
public:
	Frodo() { }

	void ProcessArgs(int argc, char ** argv);
	int ReadyToRun();

	bool RunPrefsEditor();

private:
	void save_test_screenshot(const std::string & path);

	std::filesystem::path prefs_path;		// Pathname of current preferences file
	std::filesystem::path snapshot_path;	// Directory for saving snapshots

	std::vector<std::string> prefs_override;	// Preferences items overridden on command line
};


// Global application object
extern Frodo * TheApp;

// Global C64 object
extern C64 * TheC64;


#endif // ndef MAIN_H
