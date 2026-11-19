/*
    VeMUlator - A Dreamcast Visual Memory Unit emulator for libretro
    Copyright (C) 2018  Mahmoud Jaoune

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>
#include <inttypes.h>

#define FPS 60
#define SAMPLE_RATE 32768
#define SCREEN_WIDTH 48
#define SCREEN_HEIGHT 32

/* A real VMU has four icons in a strip under the dot matrix -- a file, a
   spade for game mode, a clock, and the flash-write marker. They are
   separate LCD segments driven from XRAM bank 2, not part of the 48x32
   picture, so showing them makes the frame taller. */
#define ICON_ROWS 12
#define ICON_COUNT 4
#define ICON_WIDTH (SCREEN_WIDTH / ICON_COUNT)
#define SCREEN_HEIGHT_ICONS (SCREEN_HEIGHT + ICON_ROWS)

typedef unsigned char byte;

#endif // _COMMON_H_
