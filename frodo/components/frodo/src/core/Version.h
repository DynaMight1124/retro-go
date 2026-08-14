/*
 *  Version.h - Version information
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

#ifndef VERSION_H
#define VERSION_H

// Version/revision
constexpr int FRODO_VERSION = 4;
constexpr int FRODO_REVISION = 5;

#ifdef FRODO_SC
const char VERSION_STRING[] = "Frodo V4.5";
#else
const char VERSION_STRING[] = "Frodo Lite V4.5";
#endif

#define DRIVE_ID_STRING "FRODO V4.5"

#endif // ndef VERSION_H
