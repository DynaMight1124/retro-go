/*
 *  Display.cpp - C64 graphics display, emulator window handling
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

#include "Display.h"
#include "C64.h"
#include "Cartridge.h"
#include "IEC.h"
#include "Prefs.h"
#include "Version.h"

#include <SDL.h>

#include <filesystem>
#include <format>
namespace fs = std::filesystem;
namespace chrono = std::chrono;

#include <stdlib.h>


// Drive LED display states
enum {
	LED_OFF = DRVLED_OFF,				// LED and icon off
	LED_ON = DRVLED_ON,					// LED on (green)
	LED_ERROR_OFF = DRVLED_ERROR_OFF,	// LED off, drive icon visible
	LED_ERROR_ON = DRVLED_ERROR_ON,		// LED on (red)
	LED_FLASH_ON = DRVLED_ERROR_FLASH,	// LED flashing, currently on (red)
	LED_FLASH_OFF						// LED flashing, currently off
};

// Period of LED error flashing
constexpr uint32_t PULSE_ms = 138;

// Notification timeout
constexpr int NOTIFICATION_TIMEOUT_ms = 4000;

// Drive LED image
static const char * led_image[8] = {
    "  XXX   ",
    " X.O.X  ",
    "X.O...X ",
    "X.O...X ",
    "X.....X ",
    " X...X  ",
    "  XXX   ",
    "        ",
};

// Menu font
#include "MenuFont.h"

#define MCHAR_DRIVE_L "\x07"
#define MCHAR_DRIVE_R "\x08"
#define MCHAR_PLAY "\x0a"
#define MCHAR_REWIND "\x0b"
#define MCHAR_FORWARD "\x0c"
#define MCHAR_PAUSE "\x0d"
#define MCHAR_RECORD "\x0e"
#define MCHAR_TAPE "\x0f"


// C64 color palettes based on measurements by Philip "Pepto" Timmermann <pepto@pepto.de>
// (see http://www.pepto.de/projects/colorvic/)

// Original "Pepto" palette
static const uint8_t palette_pepto_red[16] = {
	0x00, 0xff, 0x86, 0x4c, 0x88, 0x35, 0x20, 0xcf, 0x88, 0x40, 0xcb, 0x34, 0x68, 0x8b, 0x68, 0xa1
};

static const uint8_t palette_pepto_green[16] = {
	0x00, 0xff, 0x19, 0xc1, 0x17, 0xac, 0x07, 0xf2, 0x3e, 0x2a, 0x55, 0x34, 0x68, 0xff, 0x4a, 0xa1
};

static const uint8_t palette_pepto_blue[16] = {
	0x00, 0xff, 0x01, 0xe3, 0xbd, 0x0a, 0xc0, 0x2d, 0x00, 0x00, 0x37, 0x34, 0x68, 0x59, 0xff, 0xa1
};

// "Colodore" palette
static const uint8_t palette_colodore_red[16] = {
	0x00, 0xff, 0x81, 0x75, 0x8e, 0x56, 0x2e, 0xed, 0x8e, 0x55, 0xc4, 0x4a, 0x7b, 0xa9, 0x70, 0xb2
};

static const uint8_t palette_colodore_green[16] = {
	0x00, 0xff, 0x33, 0xce, 0x3c, 0xac, 0x2c, 0xf1, 0x50, 0x38, 0x6c, 0x4a, 0x7b, 0xff, 0x6d, 0xb2
};

static const uint8_t palette_colodore_blue[16] = {
	0x00, 0xff, 0x38, 0xc8, 0x97, 0x4d, 0x9b, 0x71, 0x29, 0x00, 0x71, 0x4a, 0x7b, 0x9f, 0xeb, 0xb2
};


// Colors for speedometer/drive LEDs
enum {
	black = 0,
	white = 1,
	fill_gray = 16,
	shine_gray = 17,
	shadow_gray = 18,
	red = 19,
	dark_red = 20,
	green = 21,
};


/*
 *  Display constructor
 */

Display::Display(C64 * c64) : the_c64(c64)
{
	speedometer_string[0] = '\0';

	// Create window and renderer
	uint32_t flags = (ThePrefs.DisplayType == DISPTYPE_SCREEN) ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_RESIZABLE;

	if (ThePrefs.TestBench) {
		flags |= SDL_WINDOW_HIDDEN;	// Hide window in regression test mode
	}

	int result = SDL_CreateWindowAndRenderer(
		DISPLAY_X * ThePrefs.ScalingNumerator / ThePrefs.ScalingDenominator,
		DISPLAY_Y * ThePrefs.ScalingNumerator / ThePrefs.ScalingDenominator,
		flags, &the_window, &the_renderer
	);
	if (result < 0) {
		error_and_quit(std::format("Couldn't initialize video output ({})\n", SDL_GetError()));
	}

	SDL_SetWindowTitle(the_window, VERSION_STRING);
	SDL_SetWindowPosition(the_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_SetWindowMinimumSize(the_window, DISPLAY_X, DISPLAY_Y);
	SDL_RenderSetLogicalSize(the_renderer, DISPLAY_X, DISPLAY_Y);

	// Clear screen to black
	SDL_SetRenderDrawColor(the_renderer, 0, 0, 0, 255);
	SDL_RenderClear(the_renderer);

	// Create 32-bit display texture
	the_texture = SDL_CreateTexture(the_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, DISPLAY_X, DISPLAY_Y);
	if (! the_texture) {
		error_and_quit(std::format("Couldn't create SDL texture ({})\n", SDL_GetError()));
	}

	// Create 8-bit indexed pixel buffer for VIC to draw into
	vic_pixels = new uint8_t[DISPLAY_X * DISPLAY_Y];
	memset(vic_pixels, 0, DISPLAY_X * DISPLAY_Y);

	// Init color palette for pixel buffer
	init_colors(ThePrefs.Palette);

	// Hide mouse pointer in fullscreen mode
	if (ThePrefs.DisplayType == DISPTYPE_SCREEN) {
		SDL_ShowCursor(SDL_DISABLE);
	}

	// LEDs off
	for (unsigned i = 0; i < 4; ++i) {
		led_state[i] = LED_OFF;
	}

	// Create LED images
	for (unsigned type = 0; type < 3; ++type) {
		unsigned color;
		switch (type) {
			case 0:  color = green; break;		// LED_ON
			case 1:  color = red; break;		// LED_ERROR_ON
			default: color = dark_red; break;	// LED_ERROR_OFF
		}
		uint8_t * p = led_pixmap[type];
		for (unsigned y = 0; y < 8; ++y) {
			for (unsigned x = 0; x < 8; ++x) {
				switch (led_image[y][x]) {
					case '.': p[x] = color; break;
					case 'X': p[x] = shadow_gray; break;
					case 'O': p[x] = shine_gray; break;
					default:  p[x] = 0; break;
				}
			}
			p += 8;
		}
	}

	// Start timer for LED error flashing
	pulse_timer = SDL_AddTimer(PULSE_ms, pulse_handler_static, this);
	if (pulse_timer == 0) {
		error_and_quit(std::format("Couldn't create SDL pulse timer ({})\n", SDL_GetError()));
	}

	// Get controller button mapping
	button_mapping = ThePrefs.SelectedButtonMapping();

	// Clear notifications
	for (unsigned i = 0; i < NUM_NOTIFICATIONS; ++i) {
		notes[i].active = false;
	}
	next_note = 0;

	// Show greeting
	if (! ThePrefs.AutoStart) {
		ShowNotification("Welcome to Frodo, press F10 for settings");
	}
}


/*
 *  Display destructor
 */

Display::~Display()
{
	if (pulse_timer) {
		SDL_RemoveTimer(pulse_timer);
	}

	delete[] vic_pixels;

	if (the_renderer) {
		SDL_DestroyRenderer(the_renderer);
	}
	if (the_window) {
		SDL_DestroyWindow(the_window);
	}
}


/*
 *  Show an error message and quit
 */

void Display::error_and_quit(const std::string & msg) const
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, VERSION_STRING, msg.c_str(), the_window);
	SDL_Quit();
	exit(1);
}


/*
 *  Pause display: Exit fullscreen mode
 */

void Display::Pause()
{
	if (ThePrefs.DisplayType == DISPTYPE_SCREEN) {
		toggle_fullscreen(false);
	}
}


/*
 *  Resume display: Re-enter fullscreen mode
 */

void Display::Resume()
{
	if (ThePrefs.DisplayType == DISPTYPE_SCREEN) {
		toggle_fullscreen(true);
	}
}


/*
 *  Prefs may have changed, recalculate palette
 */

void Display::NewPrefs(const Prefs *prefs)
{
	if (prefs->Palette != ThePrefs.Palette) {
		init_colors(prefs->Palette);
	}

	button_mapping = prefs->SelectedButtonMapping();
}


/*
 *  Set drive LED display (display is deferred until Update())
 */

void Display::SetLEDs(int l0, int l1, int l2, int l3)
{
	led_state[0] = l0;
	led_state[1] = l1;
	led_state[2] = l2;
	led_state[3] = l3;
}


/*
 *  Show notification to user (display is deferred until Update())
 */

void Display::ShowNotification(std::string s)
{
	// Copy message
	unsigned size = s.length();
	if (size >= sizeof(Notification::text)) {
		size = sizeof(Notification::text) - 1;
	}
	for (unsigned i = 0; i < size; ++i) {
		notes[next_note].text[i] = s[i];
	}
	notes[next_note].text[size] = '\0';

	// Remember notification time
	notes[next_note].time = chrono::steady_clock::now();

	// Make notification active
	notes[next_note].active = true;
	next_note = (next_note + 1) % NUM_NOTIFICATIONS;
}


/*
 *  LED error flashing
 */

uint32_t Display::pulse_handler_static(uint32_t interval, void * arg)
{
	Display * disp = static_cast<Display *>(arg);
	disp->pulse_handler();
	return interval;
}

void Display::pulse_handler()
{
	for (unsigned i = 0; i < 4; ++i) {
		switch (led_state[i]) {
			case LED_FLASH_ON:
				led_state[i] = LED_FLASH_OFF;
				break;
			case LED_FLASH_OFF:
				led_state[i] = LED_FLASH_ON;
				break;
		}
	}
}


/*
 *  Set speedometer display (deferred until Update())
 */

void Display::SetSpeedometer(int speed)
{
	static int delay = 0;

	if (delay >= 20) {
		delay = 0;
		if (speed == 100) {
			speedometer_string[0] = '\0';  // hide if speed = 100%
		} else {
			snprintf(speedometer_string, sizeof(speedometer_string), "%d%%", speed);
		}
	} else {
		delay++;
	}
}


/*
 *  Show VIC bitmap and user interface elements on the display
 */

void Display::draw_overlays()
{
	// Update and draw notifications
	auto now = chrono::steady_clock::now();

	unsigned i = next_note;
	unsigned y_pos = 3;
	do {
		if (notes[i].active) {
			int elapsed_ms = chrono::duration_cast<chrono::milliseconds>(now - notes[i].time).count();
			if (elapsed_ms > NOTIFICATION_TIMEOUT_ms) {
				notes[i].active = false;
			} else {
				draw_string(5, y_pos + 1, notes[i].text, shadow_gray);
				draw_string(4, y_pos    , notes[i].text, shine_gray);
				y_pos += 8;
			}
		}
		i = (i + 1) % NUM_NOTIFICATIONS;
	} while (i != next_note);

	if (ThePrefs.ShowLEDs) {

		// Draw speedometer
		draw_string(5, DISPLAY_Y - 8, speedometer_string, shadow_gray);
		draw_string(4, DISPLAY_Y - 9, speedometer_string, shine_gray);

		// Draw disk drive LEDs
		for (unsigned i = 0; i < 4; ++i) {
			if (led_state[i] != LED_OFF) {
				static const char * drive_str[4] = {
					MCHAR_DRIVE_L MCHAR_DRIVE_R "8",
					MCHAR_DRIVE_L MCHAR_DRIVE_R "9",
					MCHAR_DRIVE_L MCHAR_DRIVE_R "10",
					MCHAR_DRIVE_L MCHAR_DRIVE_R "11",
				};

				draw_string(DISPLAY_X * (i+1) / 7 + 1, DISPLAY_Y - 8, drive_str[i], shadow_gray);
				draw_string(DISPLAY_X * (i+1) / 7,     DISPLAY_Y - 9, drive_str[i], shine_gray);

				uint8_t * p = vic_pixels + (DISPLAY_X * (i+1) / 7 + (i < 2 ? 24 : 31)) + DISPLAY_X * (DISPLAY_Y - 9);

				const uint8_t * q;
				switch (led_state[i]) {
					case LED_ERROR_ON:
					case LED_FLASH_ON:  q = led_pixmap[1]; break;
					case LED_ERROR_OFF:
					case LED_FLASH_OFF: q = led_pixmap[2]; break;
					default:            q = led_pixmap[0]; break;
				}

				for (unsigned y = 0; y < 8; ++y) {
					for (unsigned x = 0; x < 8; ++x) {
						uint8_t c = q[x];
						if (c) {	// 0 = transparent
							p[x] = c;
						}
					}
					p += DISPLAY_X;
					q += 8;
				}
			}
		}

		// Draw tape indicator
		TapeState tape_state = the_c64->TapeDriveState();
		if (tape_state != TapeState::Stop) {
			draw_string(DISPLAY_X - 80, DISPLAY_Y -  9, MCHAR_TAPE, shadow_gray);
			draw_string(DISPLAY_X - 81, DISPLAY_Y - 10, MCHAR_TAPE, shine_gray);

			int x_pos;
			if (tape_state == TapeState::Record) {
				draw_string(DISPLAY_X - 68, DISPLAY_Y -  9, MCHAR_RECORD, shadow_gray);
				draw_string(DISPLAY_X - 69, DISPLAY_Y - 10, MCHAR_RECORD, red);
				x_pos = 56;
			} else {
				draw_string(DISPLAY_X - 68, DISPLAY_Y -  9, MCHAR_PLAY, shadow_gray);
				draw_string(DISPLAY_X - 69, DISPLAY_Y - 10, MCHAR_PLAY, green);
				x_pos = 58;
			}

			char str[16];
			int pos = the_c64->TapePosition();
			if (pos == 100) {
				strcpy(str, "end");
			} else {
				snprintf(str, sizeof(str), "%d%%", pos);
			}

			draw_string(DISPLAY_X - x_pos,     DISPLAY_Y -  9, str, shadow_gray);
			draw_string(DISPLAY_X - x_pos - 1, DISPLAY_Y - 10, str, shine_gray);
		}

		// Draw play mode indicator
		PlayMode mode = the_c64->GetPlayMode();
		if (mode != PlayMode::Play) {
			const char * str;
			switch (mode) {
				case PlayMode::Rewind:
					str = MCHAR_REWIND;
					break;
				case PlayMode::Forward:
					str = MCHAR_FORWARD;
					break;
				case PlayMode::Pause:
					str = MCHAR_PAUSE;
					break;
				default:
					str = nullptr;
					break;
			};
			if (str) {
				draw_string(DISPLAY_X - 11, DISPLAY_Y -  9, str, shadow_gray);
				draw_string(DISPLAY_X - 12, DISPLAY_Y - 10, str, shine_gray);
			}
		}
	}

}

void Display::Update()
{
	// Draw user interface elements (but keep regression test screenshot clean)
	if (ThePrefs.TestScreenshotPath.empty()) {
		draw_overlays();
	}

	// Convert 8-bit pixel buffer to 32-bit texture
	uint32_t * texture_buffer;
	int texture_pitch;

	SDL_LockTexture(the_texture, nullptr, (void **) &texture_buffer, &texture_pitch);

	uint8_t  * inPixel  = vic_pixels;
	uint32_t * outPixel = texture_buffer;

	for (unsigned y = 0; y < DISPLAY_Y; ++y) {
		for (unsigned x = 0; x < DISPLAY_X; ++x) {
			outPixel[x] = palette[inPixel[x]];
		}
		inPixel  += DISPLAY_X;
		outPixel += texture_pitch / sizeof(uint32_t);
	}

	SDL_UnlockTexture(the_texture);

	// Update display
	SDL_RenderClear(the_renderer);
	SDL_RenderCopy(the_renderer, the_texture, nullptr, nullptr);
	SDL_RenderPresent(the_renderer);
}


/*
 *  Draw string into pixel buffer using the C64 lower-case ROM font
 */

void Display::draw_string(unsigned x, unsigned y, const char *str, uint8_t front_color) const
{
	uint8_t *pb = vic_pixels + DISPLAY_X*y + x;

	unsigned char c;
	while ((c = *str++) != 0) {
		if (c >= 0x80) {
			c = 0x7f;	// Replacement character
		}
		const uint8_t * q = menu_font + c*8;
		uint8_t * p = pb;
		for (unsigned y = 0; y < 8; y++) {
			uint8_t v = *q++;
			for (unsigned x = 0; x < menu_char_width[c]; ++x) {
				if (v & 0x80) { p[x] = front_color; }
				v <<= 1;
			}
			p += DISPLAY_X;
		}
		pb += menu_char_width[c];
	}
}


/*
 *  Return pointer to VIC bitmap data
 */

uint8_t * Display::BitmapBase()
{
	return vic_pixels;
}


/*
 *  Return number of VIC bitmap bytes per row
 */

int Display::BitmapXMod()
{
	return DISPLAY_X;
}


/*
 *  Toggle fullscreen mode
 */

void Display::toggle_fullscreen(bool full)
{
	if (full) {
		SDL_SetWindowFullscreen(the_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		SDL_ShowCursor(SDL_DISABLE);
	} else {
		SDL_SetWindowFullscreen(the_window, 0);
		SDL_ShowCursor(SDL_ENABLE);
	}
}


/*
 *  Poll the keyboard
 */

/*
  C64 keyboard matrix:

    Bit 7   6   5   4   3   2   1   0
  0    CUD  F5  F3  F1  F7 CLR RET DEL
  1    SHL  E   S   Z   4   A   W   3
  2     X   T   F   C   6   D   R   5
  3     V   U   H   B   8   G   Y   7
  4     N   O   K   M   0   J   I   9
  5     ,   @   :   .   -   L   P   +
  6     /   ↑   =  SHR HOM  ;   *   £
  7    R/S  Q   C= SPC  2  CTL  ←   1
*/

#define MATRIX(a,b) (((a) << 3) | (b))

static void translate_key(SDL_Scancode key, bool key_up, uint8_t *key_matrix, uint8_t *rev_matrix, uint8_t *joystick)
{
	int c64_key = -1;
	switch (key) {
		case SDL_SCANCODE_A: c64_key = MATRIX(1,2); break;
		case SDL_SCANCODE_B: c64_key = MATRIX(3,4); break;
		case SDL_SCANCODE_C: c64_key = MATRIX(2,4); break;
		case SDL_SCANCODE_D: c64_key = MATRIX(2,2); break;
		case SDL_SCANCODE_E: c64_key = MATRIX(1,6); break;
		case SDL_SCANCODE_F: c64_key = MATRIX(2,5); break;
		case SDL_SCANCODE_G: c64_key = MATRIX(3,2); break;
		case SDL_SCANCODE_H: c64_key = MATRIX(3,5); break;
		case SDL_SCANCODE_I: c64_key = MATRIX(4,1); break;
		case SDL_SCANCODE_J: c64_key = MATRIX(4,2); break;
		case SDL_SCANCODE_K: c64_key = MATRIX(4,5); break;
		case SDL_SCANCODE_L: c64_key = MATRIX(5,2); break;
		case SDL_SCANCODE_M: c64_key = MATRIX(4,4); break;
		case SDL_SCANCODE_N: c64_key = MATRIX(4,7); break;
		case SDL_SCANCODE_O: c64_key = MATRIX(4,6); break;
		case SDL_SCANCODE_P: c64_key = MATRIX(5,1); break;
		case SDL_SCANCODE_Q: c64_key = MATRIX(7,6); break;
		case SDL_SCANCODE_R: c64_key = MATRIX(2,1); break;
		case SDL_SCANCODE_S: c64_key = MATRIX(1,5); break;
		case SDL_SCANCODE_T: c64_key = MATRIX(2,6); break;
		case SDL_SCANCODE_U: c64_key = MATRIX(3,6); break;
		case SDL_SCANCODE_V: c64_key = MATRIX(3,7); break;
		case SDL_SCANCODE_W: c64_key = MATRIX(1,1); break;
		case SDL_SCANCODE_X: c64_key = MATRIX(2,7); break;
		case SDL_SCANCODE_Y: c64_key = MATRIX(3,1); break;
		case SDL_SCANCODE_Z: c64_key = MATRIX(1,4); break;

		case SDL_SCANCODE_0: c64_key = MATRIX(4,3); break;
		case SDL_SCANCODE_1: c64_key = MATRIX(7,0); break;
		case SDL_SCANCODE_2: c64_key = MATRIX(7,3); break;
		case SDL_SCANCODE_3: c64_key = MATRIX(1,0); break;
		case SDL_SCANCODE_4: c64_key = MATRIX(1,3); break;
		case SDL_SCANCODE_5: c64_key = MATRIX(2,0); break;
		case SDL_SCANCODE_6: c64_key = MATRIX(2,3); break;
		case SDL_SCANCODE_7: c64_key = MATRIX(3,0); break;
		case SDL_SCANCODE_8: c64_key = MATRIX(3,3); break;
		case SDL_SCANCODE_9: c64_key = MATRIX(4,0); break;

		case SDL_SCANCODE_SPACE: c64_key = MATRIX(7,4); break;
		case SDL_SCANCODE_GRAVE: c64_key = MATRIX(7,1); break;			// ←
		case SDL_SCANCODE_BACKSLASH: c64_key = MATRIX(6,6); break;		// ↑
		case SDL_SCANCODE_COMMA: c64_key = MATRIX(5,7); break;
		case SDL_SCANCODE_PERIOD: c64_key = MATRIX(5,4); break;
		case SDL_SCANCODE_MINUS: c64_key = MATRIX(5,0); break;			// +
		case SDL_SCANCODE_EQUALS: c64_key = MATRIX(5,3); break;			// -
		case SDL_SCANCODE_LEFTBRACKET: c64_key = MATRIX(5,6); break;	// @
		case SDL_SCANCODE_RIGHTBRACKET: c64_key = MATRIX(6,1); break;	// *
		case SDL_SCANCODE_SEMICOLON: c64_key = MATRIX(5,5); break;		// :
		case SDL_SCANCODE_APOSTROPHE: c64_key = MATRIX(6,2); break;		// ;
		case SDL_SCANCODE_SLASH: c64_key = MATRIX(6,7); break;

		case SDL_SCANCODE_ESCAPE: c64_key = MATRIX(7,7); break;			// RUN/STOP
		case SDL_SCANCODE_RETURN: c64_key = MATRIX(0,1); break;
		case SDL_SCANCODE_BACKSPACE:
		case SDL_SCANCODE_DELETE: c64_key = MATRIX(0,0); break;			// INS/DEL
		case SDL_SCANCODE_INSERT: c64_key = MATRIX(0,0) | 0x80; break;
		case SDL_SCANCODE_HOME: c64_key = MATRIX(6,3); break;			// CLR/HOME
		case SDL_SCANCODE_END: c64_key = MATRIX(6,0); break;			// £
		case SDL_SCANCODE_PAGEUP: c64_key = MATRIX(6,6); break;			// ↑
		case SDL_SCANCODE_PAGEDOWN: c64_key = MATRIX(6,5); break;		// =

		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_TAB:
		case SDL_SCANCODE_RCTRL: c64_key = MATRIX(7,2); break;
		case SDL_SCANCODE_LSHIFT: c64_key = MATRIX(1,7); break;
		case SDL_SCANCODE_RSHIFT: c64_key = MATRIX(6,4); break;
		case SDL_SCANCODE_LALT: c64_key = MATRIX(7,5); break;			// C=
		case SDL_SCANCODE_RALT: c64_key = MATRIX(7,5); break;			// C=

		case SDL_SCANCODE_UP: c64_key = MATRIX(0,7)| 0x80; break;
		case SDL_SCANCODE_DOWN: c64_key = MATRIX(0,7); break;
		case SDL_SCANCODE_LEFT: c64_key = MATRIX(0,2) | 0x80; break;
		case SDL_SCANCODE_RIGHT: c64_key = MATRIX(0,2); break;

		case SDL_SCANCODE_F1: c64_key = MATRIX(0,4); break;
		case SDL_SCANCODE_F2: c64_key = MATRIX(0,4) | 0x80; break;
		case SDL_SCANCODE_F3: c64_key = MATRIX(0,5); break;
		case SDL_SCANCODE_F4: c64_key = MATRIX(0,5) | 0x80; break;
		case SDL_SCANCODE_F5: c64_key = MATRIX(0,6); break;
		case SDL_SCANCODE_F6: c64_key = MATRIX(0,6) | 0x80; break;
		case SDL_SCANCODE_F7: c64_key = MATRIX(0,3); break;
		case SDL_SCANCODE_F8: c64_key = MATRIX(0,3) | 0x80; break;

		case SDL_SCANCODE_KP_0:
		case SDL_SCANCODE_KP_5: c64_key = 0x10 | 0x40; break;
		case SDL_SCANCODE_KP_1: c64_key = 0x06 | 0x40; break;
		case SDL_SCANCODE_KP_2: c64_key = 0x02 | 0x40; break;
		case SDL_SCANCODE_KP_3: c64_key = 0x0a | 0x40; break;
		case SDL_SCANCODE_KP_4: c64_key = 0x04 | 0x40; break;
		case SDL_SCANCODE_KP_6: c64_key = 0x08 | 0x40; break;
		case SDL_SCANCODE_KP_7: c64_key = 0x05 | 0x40; break;
		case SDL_SCANCODE_KP_8: c64_key = 0x01 | 0x40; break;
		case SDL_SCANCODE_KP_9: c64_key = 0x09 | 0x40; break;

		default: break;
	}

	if (c64_key < 0)
		return;

	// Handle joystick emulation
	if (c64_key & 0x40) {
		c64_key &= 0x1f;
		if (key_up) {
			*joystick |= c64_key;
		} else {
			*joystick &= ~c64_key;
		}
		return;
	}

	// Handle other keys
	bool shifted = c64_key & 0x80;
	int c64_byte = (c64_key >> 3) & 7;
	int c64_bit = c64_key & 7;
	if (key_up) {
		if (shifted) {
			key_matrix[6] |= 0x10;
			rev_matrix[4] |= 0x40;
		}
		key_matrix[c64_byte] |= (1 << c64_bit);
		rev_matrix[c64_bit] |= (1 << c64_byte);
	} else {
		if (shifted) {
			key_matrix[6] &= 0xef;
			rev_matrix[4] &= 0xbf;
		}
		key_matrix[c64_byte] &= ~(1 << c64_bit);
		rev_matrix[c64_bit] &= ~(1 << c64_byte);
	}
}

void Display::PollKeyboard(uint8_t *key_matrix, uint8_t *rev_matrix, uint8_t *joystick)
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {

			// Key pressed
			case SDL_KEYDOWN:
				if (event.key.repeat) {
					break;
				}

				switch (event.key.keysym.scancode) {

					case SDL_SCANCODE_F10:	// F10: Prefs/Quit
						the_c64->RequestPrefsEditor();
						break;

					case SDL_SCANCODE_F11:	// F11: NMI (Restore)
						the_c64->NMI();
						break;

					case SDL_SCANCODE_F12:	// F12: Reset (hold Shift to clear memory, Ctrl to auto-start)
						if (SDL_GetModState() & KMOD_CTRL) {
							the_c64->ResetAndAutoStart();
						} else {
							the_c64->Reset(SDL_GetModState() & KMOD_SHIFT);
						}
						break;

					case SDL_SCANCODE_NUMLOCKCLEAR:
						num_locked = !num_locked;
						break;

					case SDL_SCANCODE_KP_ENTER:	// Enter on keypad: Toggle fullscreen
						if (ThePrefs.DisplayType == DISPTYPE_WINDOW) {
							ThePrefs.DisplayType = DISPTYPE_SCREEN;
							toggle_fullscreen(true);
						} else {
							ThePrefs.DisplayType = DISPTYPE_WINDOW;
							toggle_fullscreen(false);
						}
						break;

					case SDL_SCANCODE_KP_PLUS:	// Plus on keypad: Fast-forward while pressed
						if (the_c64->GetPlayMode() == PlayMode::Play) {
							the_c64->SetPlayMode(PlayMode::Forward);
						} else if (the_c64->GetPlayMode() == PlayMode::Pause) {
							the_c64->SetPlayMode(PlayMode::ForwardFrame);
						}
						break;

					case SDL_SCANCODE_KP_MINUS:	// Minus on keypad: Rewind while pressed
						if (the_c64->GetPlayMode() == PlayMode::Play) {
							the_c64->SetPlayMode(PlayMode::Rewind);
						} else if (the_c64->GetPlayMode() == PlayMode::Pause) {
							the_c64->SetPlayMode(PlayMode::RewindFrame);
						}
						break;

#undef DEBUG
#ifdef DEBUG
					case SDL_SCANCODE_PAUSE:
						if (the_c64->GetPlayMode() == PlayMode::Play) {
							the_c64->SetPlayMode(PlayMode::RequestPause);
						} else if (the_c64->GetPlayMode() == PlayMode::Pause) {
							the_c64->SetPlayMode(PlayMode::Play);
						}
						break;
#endif

					default:
						translate_key(event.key.keysym.scancode, false, key_matrix, rev_matrix, joystick);
						break;
				}
				break;

			// Key released
			case SDL_KEYUP:
				if (event.key.keysym.scancode == SDL_SCANCODE_KP_PLUS) {
					if (the_c64->GetPlayMode() == PlayMode::Forward) {
						the_c64->SetPlayMode(PlayMode::Play);
					}
				} else if (event.key.keysym.scancode == SDL_SCANCODE_KP_MINUS) {
					if (the_c64->GetPlayMode() == PlayMode::Rewind) {
						the_c64->SetPlayMode(PlayMode::Play);
					}
				} else {
					translate_key(event.key.keysym.scancode, true, key_matrix, rev_matrix, joystick);
				}
				break;

			// File dropped
			case SDL_DROPFILE: {
				char * filename = event.drop.file;
				int type;

				if (fs::is_directory(filename)) {

					// Turn off 1541 processor emulation and mount directory
					the_c64->MountDrive8(false, filename);
					ShowNotification("Directory mounted in drive 8");

				} else if (IsMountableFile(filename, type)) {

					// Mount disk image file
					if (type == FILE_DISK_IMAGE) {
						the_c64->MountDrive8(ThePrefs.Emul1541Proc, filename);
						ShowNotification("Disk image file mounted in drive 8");
					} else if (type == FILE_GCR_IMAGE) {
						the_c64->MountDrive8(true, filename);
						ShowNotification("Disk image file mounted in drive 8");
					} else if (type == FILE_TAPE_IMAGE) {
						the_c64->MountDrive1(filename);
						ShowNotification("Tape image file mounted in drive 1");
					} else if (type == FILE_ARCH) {
						the_c64->MountDrive8(false, filename);
						ShowNotification("Archive file mounted in drive 8");
					}

				} else if (IsSnapshotFile(filename)) {

					// Load snapshot
					the_c64->RequestLoadSnapshot(filename);

				} else if (IsCartridgeFile(filename)) {

					// Insert cartridge
					the_c64->InsertCartridge(filename);

				} else if (IsBASICProgram(filename)) {

					// Load BASIC file directly into RAM
					std::string message;
					if (the_c64->DMALoad(filename, message)) {
						message = "Program loaded, type RUN to start";
					}
					ShowNotification(message);
				}

				SDL_free(filename);
				break;
			}

			// Map controller buttons to keyboard
			// Note: 'A' button is handled in C64::poll_joystick() separately for each controller
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP: {
				SDL_GameControllerButton button = (SDL_GameControllerButton) event.cbutton.button;
				if (button_mapping.count(button) > 0) {
					unsigned keycode = button_mapping[button];
					if (keycode < 64) {
						if (event.type == SDL_CONTROLLERBUTTONUP) {
							key_matrix[keycode >> 3] |= 1 << (keycode & 7);
							rev_matrix[keycode & 7] |= 1 << (keycode >> 3);
						} else {
							key_matrix[keycode >> 3] &= ~(1 << (keycode & 7));
							rev_matrix[keycode & 7] &= ~(1 << (keycode >> 3));
						}
					} else if (keycode == KEYCODE_PLAY_ON_TAPE) {
						if (event.type == SDL_CONTROLLERBUTTONUP) {
							the_c64->SetTapeControllerButton(false);
						} else {
							the_c64->SetTapeControllerButton(true);
						}
					}
				}
				break;
			}

			// Controller attached/removed
			case SDL_CONTROLLERDEVICEADDED:
				if (! ThePrefs.TestBench) {
					the_c64->JoystickAdded(event.cdevice.which);
				}
				break;

			case SDL_CONTROLLERDEVICEREMOVED:
				if (! ThePrefs.TestBench) {
					the_c64->JoystickRemoved(event.cdevice.which);
				}
				break;

			// Quit Frodo
			case SDL_QUIT:
				the_c64->RequestQuit();
				break;
		}
	}
}


/*
 *  Check if NumLock is down (for switching the joystick keyboard emulation)
 */

bool Display::NumLock()
{
	return num_locked;
}


/*
 *  Set VIC color palette
 */

void Display::init_colors(int palette_prefs)
{
	// Create palette for indexed-to-ARGB conversion
	memset(palette, 0, sizeof(palette));

	const uint8_t * r, * g, * b;
	if (palette_prefs == PALETTE_COLODORE) {
		r = palette_colodore_red;
		g = palette_colodore_green;
		b = palette_colodore_blue;
	} else {
		r = palette_pepto_red;
		g = palette_pepto_green;
		b = palette_pepto_blue;
	}

	// Colors for VIC
	for (unsigned i = 0; i < 16; ++i) {
		palette[i] = (r[i] << 16) | (g[i] << 8) | (b[i] << 0);
	}

	// Extra colors for UI elements
	palette[fill_gray]   = (0xd0 << 16) | (0xd0 << 8) | (0xd0 << 0);
	palette[shine_gray]  = (0xf0 << 16) | (0xf0 << 8) | (0xf0 << 0);
	palette[shadow_gray] = (0x40 << 16) | (0x40 << 8) | (0x40 << 0);
	palette[red]         = (0xf0 << 16) | (0x00 << 8) | (0x00 << 0);
	palette[dark_red]    = (0x30 << 16) | (0x00 << 8) | (0x00 << 0);
	palette[green]       = (0x00 << 16) | (0xc0 << 8) | (0x00 << 0);
}
