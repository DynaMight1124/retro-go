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

#ifndef _VIDEO_H_
#define _VIDEO_H_

#include "ram.h"

///This keeps track of XRAM and draws on the canvas when refresh rate occurs.
class VE_VMS_VIDEO
{
public:
    VE_VMS_VIDEO(VE_VMS_RAM *_ram);
    ~VE_VMS_VIDEO();

    void drawFrame(uint16_t *buffer);

    /* The icon strip under the picture. Written straight after the 48x32
       frame, so the buffer needs SCREEN_HEIGHT_ICONS rows. */
    void drawIcons(uint16_t *buffer);
    
private:
	VE_VMS_RAM *ram;


};

#endif // _VIDEO_H_
