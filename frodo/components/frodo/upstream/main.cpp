/*
 *  main.cpp - Main program
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

#include "main.h"
#include "C64.h"
#include "Cartridge.h"
#include "Display.h"
#include "IEC.h"
#include "Prefs.h"
#include "Version.h"

#ifdef HAVE_GTK
#include <gtk/gtk.h>
#endif

#include <SDL.h>

#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <memory>
namespace fs = std::filesystem;


// Global variables
Frodo * TheApp = nullptr;	// Application object
C64 * TheC64 = nullptr;		// Global C64 object


/*
 *  Process command line arguments
 */

void Frodo::ProcessArgs(int argc, char ** argv)
{
#ifdef HAVE_GTK
	gchar * config_file = nullptr;
	gchar ** remaining_args = nullptr;

	static GOptionEntry entries[] = {
		{ "config", 'c', G_OPTION_FLAG_NONE, G_OPTION_ARG_FILENAME, &config_file, "Run with a different configuration file than the default", "FILE" },
		{ G_OPTION_REMAINING, 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING_ARRAY, &remaining_args, nullptr, "[ITEM=VALUE…] [image or program file]" },
		{ nullptr, 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, nullptr, nullptr, nullptr }
	};

	GOptionContext * context = g_option_context_new("- Commodore 64 emulator");
	g_option_context_add_main_entries(context, entries, nullptr);

	GError * error = nullptr;
	if (! g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("Error on command line: %s\n", error->message);
		SDL_Quit();
		exit(1);
	}

	if (config_file) {
		prefs_path = config_file;
		g_free(config_file);
	}

	if (remaining_args) {
		bool have_filepath = false;

		for (auto arg = remaining_args; *arg != nullptr; ++arg) {

			// One remaining arg can be a file to auto-start
			if (! have_filepath) {
				int type;

				if (IsMountableFile(*arg, type)) {

					have_filepath = true;
					if (type == FILE_DISK_IMAGE || type == FILE_GCR_IMAGE) {
						prefs_override.push_back(std::string("DrivePath8=") + *arg);
						prefs_override.push_back("Emul1541Proc=true");
					} else if (type == FILE_ARCH) {
						prefs_override.push_back(std::string("DrivePath8=") + *arg);
						prefs_override.push_back("Emul1541Proc=false");
					} else if (type == FILE_TAPE_IMAGE) {
						prefs_override.push_back("DrivePath8=");	// No disk
						prefs_override.push_back(std::string("TapePath=") + *arg);
					}
					prefs_override.push_back("Cartridge=");			// No cartridge
					prefs_override.push_back("AutoStart=true");
					continue;

				} else if (IsCartridgeFile(*arg)) {

					have_filepath = true;
					prefs_override.push_back(std::string("Cartridge=") + *arg);
					prefs_override.push_back("DrivePath8=");	// No disk
					prefs_override.push_back("AutoStart=true");
					continue;

				} else if (IsBASICProgram(*arg)) {

					have_filepath = true;
					prefs_override.push_back(std::string("LoadProgram=") + *arg);
					prefs_override.push_back("Cartridge=");		// No cartridge
					prefs_override.push_back("AutoStart=true");
					continue;
				}
			}

			prefs_override.push_back(*arg);
		}

		g_strfreev(remaining_args);
	}
#endif
}


/*
 *  Arguments processed, run emulation
 */

int Frodo::ReadyToRun()
{
	// Load preferences
	if (prefs_path.empty()) {
		auto path = SDL_GetPrefPath("cebix", "Frodo");
		prefs_path = fs::path(path) / "config";
		snapshot_path = fs::path(path) / "snapshots";

		// Create snapshot directory if it doesn't exist
		if (! fs::exists(snapshot_path)) {
			fs::create_directories(snapshot_path);
		}

		SDL_free(path);
	}
	ThePrefs.Load(prefs_path);

	// Override preferences items given on command line
	for (auto & item : prefs_override) {
		ThePrefs.ParseItem(item);
	}

#ifdef HAVE_GTK
	// Show preferences editor
	if (! ThePrefs.AutoStart) {
		if (! ThePrefs.ShowEditor(true, prefs_path, snapshot_path))
			return 0;  // "Quit" clicked
	}
#endif

	// Create and start C64
	TheC64 = new C64;
	int exit_code = TheC64->Run();

	// Save test screenshot on exit if requested
	if (! ThePrefs.TestScreenshotPath.empty()) {
		save_test_screenshot(ThePrefs.TestScreenshotPath);
	}

	// Shutdown
	delete TheC64;

	// Save preferences
	ThePrefs.Save(prefs_path);

	return exit_code;
}


/*
 *  Run preferences editor
 */

bool Frodo::RunPrefsEditor()
{
	auto prefs = std::make_unique<Prefs>(ThePrefs);
	bool result = prefs->ShowEditor(false, prefs_path, snapshot_path);
	if (result) {
		TheC64->NewPrefs(prefs.get());
		ThePrefs = *prefs;
	}
	return result;
}


/*
 *  Save screenshot of VIC display for regression tests
 */

void Frodo::save_test_screenshot(const std::string & path)
{
	const uint8_t * bitmap = TheC64->TheDisplay->BitmapBase();
	const int xmod = TheC64->TheDisplay->BitmapXMod();

	const uint32_t width = DISPLAY_X;
	const uint32_t height = DISPLAY_Y;

	FILE * f = fopen(path.c_str(), "wb");
	if (! f) {
		fprintf(stderr, "Cannot create screenshot file '%s'\n", path.c_str());
		return;
	}

	// Write BMP file
	const uint32_t bitmap_size = width * height;
	const uint32_t file_size = 14 + 40 + 16*4 + bitmap_size;

	uint8_t header[14 + 40] = {

		// File header
		'B', 'M',
		file_size & 0xff, (file_size >> 8) & 0xff, (file_size >> 16) & 0xff, (file_size >> 24) & 0xff,
		0x00, 0x00, 0x00, 0x00,
		0x76, 0x00, 0x00, 0x00,	// Bitmap offset

		// Info header
		0x28, 0x00, 0x00, 0x00,	// Info header size
		width & 0xff, (width >> 8) & 0xff, (width >> 16) & 0xff, (width >> 24) & 0xff,
		height & 0xff, (height >> 8) & 0xff, (height >> 16) & 0xff, (height >> 24) & 0xff,
		0x01, 0x00, 0x08, 0x00,	// 1 plane, 8 bpp
		0x00, 0x00, 0x00, 0x00,
		bitmap_size & 0xff, (bitmap_size >> 8) & 0xff, (bitmap_size >> 16) & 0xff, (bitmap_size >> 24) & 0xff,
		0x22, 0x0b, 0x00, 0x00,	// X DPI
		0x22, 0x0b, 0x00, 0x00,	// Y DPI
		0x10, 0x00, 0x00, 0x00,	// 16 colors
		0x00, 0x00, 0x00, 0x00
	};

	fwrite(header, sizeof(header), 1, f);

	uint8_t palette[16 * 4] = {	// Very drab "Pepto" palette for VICE testbench
		0x00, 0x00, 0x00, 0x00,	// BGRA
		0xff, 0xff, 0xff, 0x00,
		0x2b, 0x37, 0x68, 0x00,
		0xb2, 0xa4, 0x70, 0x00,
		0x86, 0x3d, 0x6f, 0x00,
		0x43, 0x8d, 0x58, 0x00,
		0x79, 0x28, 0x35, 0x00,
		0x6f, 0xc7, 0xb8, 0x00,
		0x25, 0x4f, 0x6f, 0x00,
		0x00, 0x39, 0x43, 0x00,
		0x59, 0x67, 0x9a, 0x00,
		0x44, 0x44, 0x44, 0x00,
		0x6c, 0x6c, 0x6c, 0x00,
		0x84, 0xd2, 0x9a, 0x00,
		0xb5, 0x5e, 0x6c, 0x00,
		0x95, 0x95, 0x95, 0x00
	};

	fwrite(palette, sizeof(palette), 1, f);

	for (int y = height - 1; y >= 0; --y) {
		fwrite(bitmap + y * xmod, width, 1, f);
	}

	fclose(f);
}


/*
 *  Create application object and start it
 */

int main(int argc, char ** argv)
{
#ifdef HAVE_GTK
	gtk_init(&argc, &argv);
#else
	printf(
		"%s Copyright (C) Christian Bauer\n"
		"This is free software with ABSOLUTELY NO WARRANTY.\n"
		, VERSION_STRING
	);
	fflush(stdout);
#endif

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0) {
		fprintf(stderr, "Cannot initialize SDL: %s\n", SDL_GetError());
		return 1;
	}

	// Seed RNG
	std::srand(std::time(0));

	// Run Frodo application
	TheApp = new Frodo();
	TheApp->ProcessArgs(argc, argv);
	int exit_code = TheApp->ReadyToRun();

	// Shutdown
	delete TheApp;
	SDL_Quit();

	return exit_code;
}
