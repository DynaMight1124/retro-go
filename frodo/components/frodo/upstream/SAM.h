/*
 *  SAM.h - Simple Assembler and Monitor With Integrated System Explorer
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

#ifndef SAM_H
#define SAM_H

#include <string>


class C64;

// Exported functions
extern void SAM_GetState(C64 *the_c64);
extern void SAM_SetState(C64 *the_c64);

extern std::string SAM_GetStartupMessage();
extern std::string SAM_GetPrompt();
extern void SAM_Exec(std::string line, std::string & retOutput, std::string & retError);

extern void SAM(C64 *the_c64);


#endif // ndef SAM_H
