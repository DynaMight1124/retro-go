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

#include "video.h"

/* The four segments along the bottom of a VMU screen: file, game, clock and
   the flash-write warning. They are etched into the glass rather than drawn
   on a grid, so each is a single shape that is either lit or not, and they
   are drawn here at the size they are shown -- one bit a pixel, 12 wide and
   ICON_ROWS tall, most significant bit leftmost.
 *
 * Shrinking a detailed drawing of one down to this size does not work: the
 * artwork the original carried is 24x32 with one pixel lines, and halving it
 * turns those into mush. The four also sat at different places in that grid,
 * so a single crop caught the middle of the write icon and rendered it as a
 * solid block. */
static const byte FILE_ICON[ICON_ROWS * 2] =
{
   0x00, 0x00,
   0x03, 0xFC,
   0x02, 0x04,
   0x02, 0xF4,
   0x02, 0x04,
   0x02, 0xF4,
   0x02, 0x04,
   0x02, 0xF4,
   0x02, 0x04,
   0x02, 0x04,
   0x03, 0xFC,
   0x00, 0x00
};

static const byte GAME_ICON[ICON_ROWS * 2] =
{
   0x00, 0x00,
   0x00, 0x60,
   0x00, 0xF0,
   0x01, 0xF8,
   0x03, 0xFC,
   0x07, 0xFE,
   0x07, 0xFE,
   0x07, 0xFE,
   0x03, 0x8E,
   0x00, 0x60,
   0x00, 0xF0,
   0x00, 0x00
};

static const byte CLOCK_ICON[ICON_ROWS * 2] =
{
   0x00, 0x00,
   0x00, 0xF0,
   0x03, 0x0C,
   0x04, 0x42,
   0x04, 0x42,
   0x04, 0x42,
   0x04, 0x7A,
   0x04, 0x02,
   0x03, 0x0C,
   0x00, 0xF0,
   0x00, 0x00,
   0x00, 0x00
};

static const byte WRITE_ICON[ICON_ROWS * 2] =
{
   0x00, 0x00,
   0x07, 0xFE,
   0x07, 0x9E,
   0x07, 0x9E,
   0x07, 0x9E,
   0x07, 0x9E,
   0x07, 0x9E,
   0x07, 0xFE,
   0x07, 0x9E,
   0x07, 0xFE,
   0x07, 0xFE,
   0x00, 0x00
};

static const byte *const ICONS[4] =
   { FILE_ICON, GAME_ICON, CLOCK_ICON, WRITE_ICON };


VE_VMS_VIDEO::VE_VMS_VIDEO(VE_VMS_RAM *_ram)
{
	ram = _ram;
}

VE_VMS_VIDEO::~VE_VMS_VIDEO()
{
}

void VE_VMS_VIDEO::drawFrame(uint16_t *buffer)
{
   int i, c;
   size_t XRAMAddress;
   byte MSB = 0x80;	//10000000b

   //Read XRAM buffer from RAM (WRAM starts at 180), each 6 bytes make 1 horizontal line on-screen
   //Each bit declares whether the pixel is on or off
   //There is a 4-byte empty space between each two lines (96 bytes) of XRAM buffer.

   if((ram->readByte_RAW(MCR) & 8) == 0)
      return;

   //Draw pixels (48x32) for banks 0 and 1 ("i" selects bank)
   for(i = 0, c = 0; i < 2; ++i) 
   {
      XRAMAddress = 0x180;
      for (int y = 0; y < 16; y++) 
      {
         byte pixelLine[6];

         if((y % 2 == 0) && (y > 0)) XRAMAddress += 4;

         for(int p = 0; p < 6; p++)
            pixelLine[p] = ram->readByteXRAM(XRAMAddress++, i);

         //Big-Endian
         for (int x = 0; x < 6; x++)
            for (int j = 0; j < 8; j++)
               buffer[c++] = ((pixelLine[x] & (MSB >> j)) != 0) ? 0 : 0xFFFF;


      }
   }

}

/* Called separately from drawFrame, not by it: this writes past the 48x32
   picture, so only a caller that asked for the taller frame should call it.
 *
 * XRAM bank 2 holds one byte per segment at 0x181, set by the BIOS. Nothing
 * writes that bank under high level emulation, so the strip stays blank. */
void VE_VMS_VIDEO::drawIcons(uint16_t *buffer)
{
   int i, ox, oy;
   uint16_t *strip = buffer + (SCREEN_WIDTH * SCREEN_HEIGHT);

   for(i = 0; i < SCREEN_WIDTH * ICON_ROWS; i++)
      strip[i] = 0xFFFF;

   for(i = 0; i < ICON_COUNT; i++)
   {
      const byte *icon = ICONS[i];
      int left = i * ICON_WIDTH;

      if(ram->readByteXRAM(0x181 + i, 2) == 0x00)
         continue;

      for(oy = 0; oy < ICON_ROWS; oy++)
      {
         unsigned bits = (icon[oy * 2] << 8) | icon[oy * 2 + 1];

         for(ox = 0; ox < ICON_WIDTH; ox++)
            if(bits & (0x800 >> ox))
               strip[oy * SCREEN_WIDTH + left + ox] = 0;
      }
   }
}
