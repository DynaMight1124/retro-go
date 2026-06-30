=============================================================================
MAME4ALL GCW0 1.2 (January 04, 2014) by Alekmaul (alekmaul@portabledev.com)
=============================================================================
Official web page for news, additional information :
http://www.portabledev.com/pages/dingoo/jeuxdev.-perso/mame4all_od.php

Based on : MAME4ALL 2.7 (April 30, 2012) by Franxis (franxism@gmail.com)

1. INTRODUCTION
---------------
It is a port of MAME 0.37b5 emulator by Nicola Salmoria for GP2X, WIZ and CAANOO 
portable consoles.
To see MAME license see the end of this document (chapter 14).
It emulates all arcade games supported by original MAME 0.37b5 plus some additional
games from newer MAME versions.
This version emulates more than 2200 different romsets.


2. CONTROLS
-----------
The emulator controls are the next ones:
* Standard in-game MAME controls:
  - D-Pad and Analog Pad: UP, DOWN, LEFT and RIGHT.
  - Buttons A,B,X,Y,L,R: MAME buttons 1,2,3,4,5,6.
  - Button SELECT: Insert credit.
  - Button START: Start game.
* Extended controls in game (to access menus and options)
  - Buttons START+SELECT : Mame menu.
  - Buttons START+L+R: Esc (exit to mame frontend).
  - Buttons L+R: Pause.     
  - Buttons SELECT+R: Show FPS.
  - Buttons START+L: Show profiler.
  - Buttons L+B: Snapshot.

* The Game Selector will search for MAME ROMs found in the roms directory. 
  - Use UP and DOWN to select a game.
  - Use LEFT and RIGHT to page up and page down.
  - Press A to select the game to play.
  - Press L+R to exit.
  - Press SELECT to regenerate cache file.
  - If you add new games you will need to regenerate the cache file.

* After selecting a game, configuration options are available:
  - Use UP and DOWN to select the option you want to change.
  - Use LEFT and RIGHT to change the option.
  - Press A to start the game (and save the configured options).
  - Press B to go back to the game list.

* The "MAME menu" (see above) can be used to:
  - Redefine DEFAULT keys for all games.
  - Redefine keys for the current game.
  - Display game information.
  - Set Auto-fire options.
  - Enable cheats.

* Controller Notes:
  - All keys can be redefined, including the "shifted" SELECT keys.
    In some cases this is essential. ie. Robotron.
  - To type OK when MAME requires it, press LEFT and then RIGHT.


3. EMULATION OPTIONS
--------------------




4. INSTALLATION
---------------
The first time you run mame4all, you need to select a correct rom directory 
(with games supported by mame4all). When the directory is correct, press START to
select it.

Also, mame4all will create a .mame4all directory in your home directory with the 
sub directories :
  frontend/   -> Frontend configuration files
  nvram/      -> NVRAM files directory
  hi/         -> High Scores directory 
  cfg/        -> MAME configuration files directory
  memcard/    -> Memory card files directory
  snap/       -> Screen snapshots directory

You can delete the .mame4all directory to create a brand new configuration.


5. SUPPORTED GAMES
------------------
There are more than 2270 different supported romsets. For more details, see 
"gamelist.txt" and "gameromlist.txt" files.
Games have to be copied into the roms folder you selected when running mame4all 
the first time.


6. ROM NAMES
------------
Folder names or ZIP file names are listed on "gamelist.txt" file.
Romsets have to be MAME 0.37b5 ones (July 2000).
Additionaly there are additional romsets from newer MAME versions.
See "gameromlist.txt" for correct rom files into ZIP files.
NOTE: File and directory names in Linux are case-sensitive. Put all file and 
directory names using low case!.
Also, neogeo games needs a bios file named neogeo.zip with files :
 neo-geo.rom (crc=0x9036d879), ng-sfix.rom (crc=0x354029fc), ng-sm1.rom (crc=0x97cf998b).


7. SOUND SAMPLES
----------------
The sound samples are used to get complete sound in some games.
The sound samples collection is now included.


8. ARTWORK
----------
Artwork is used to improve the visualization for some games. 
The artwork collection is now included.


9. ORIGINAL CREDITS
-------------------
- MAME 0.37b5 original version by Nicola Salmoria and the MAME Team (http://www.mame.net).
- Z80 emulator Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
- M6502 emulator Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
- Hu6280 Copyright (c) 1999 Bryan McPhail, mish@tendril.force9.net
- I86 emulator by David Hedley, modified by Fabrice Frances (frances@ensica.fr)
- M6809 emulator by John Butler, based on L.C. Benschop's 6809 Simulator V09.
- M6808 based on L.C. Benschop's 6809 Simulator V09.
- M68000 emulator Copyright 1999 Karl Stenerud.  All rights reserved.
- 80x86 M68000 emulator Copyright 1998, Mike Coates, Darren Olafson.
- 8039 emulator by Mirko Buffoni, based on 8048 emulator by Dan Boris.
- T-11 emulator Copyright (C) Aaron Giles 1998
- TMS34010 emulator by Alex Pasadyn and Zsolt Vasvari.
- TMS9900 emulator by Andy Jones, based on original code by Ton Brouwer.
- Cinematronics CPU emulator by Jeff Mitchell, Zonn Moore, Neil Bradley.
- Atari AVG/DVG emulation based on VECSIM by Hedley Rainnie, Eric Smith and Al Kossow.
- TMS5220 emulator by Frank Palazzolo.
- AY-3-8910 emulation based on various code snippets by Ville Hallik, Michael Cuddy,
  Tatsuyuki Satoh, Fabrice Frances, Nicola Salmoria.
- YM-2203, YM-2151, YM3812 emulation by Tatsuyuki Satoh.
- POKEY emulator by Ron Fries (rfries@aol.com). Many thanks to Eric Smith, 
   Hedley Rainnie and Sean Trowbridge.
- NES sound hardware info by Jeremy Chadwick and Hedley Rainne.
- YM2610 emulation by Hiromitsu Shioya.


10. PORT CREDITS
----------------
- Port to GP2X, WIZ and CAANOO by Franxis (franxism@gmail.com) based on source 
  code MAME 0.37b5 (dated on july 2000).
- TheGrimReaper (m_acky@hotmail.com) has colaborated with a lot of
  things since GP32 MAME 1.3, i.e. Vector graphics support, high scores,
  general frontend, frontend improvements, bugfixes, etc. Thank you!!!
- Pepe_Faruk (joserod@ya.com) has colaborated with new screen centering
  code. Also he has added some new supported games. Thank you!!!
- Reesy (drsms_reesy@yahoo.co.uk) has developed DrZ80 (Z80 ASM ARM core) and has 
  helped a lot with core integration into MAME. He has also done several fixes 
  to the Cyclone core.  You are the best!!!
- Flubba (flubba@i-solutions.se) has done some optimizations and improvements to 
  the DrZ80 core. Thank you!!!
- Dave (dev@finalburn.com) has developed Cyclone (M68000 ASM ARM core). Big 
   thanks to him.
- Notaz (notasas@gmail.com) have fixed some bugs in the Cyclone source code. He 
  has also contributed with several useful code to the GP2X scene. Thanks!!!
  http://uosis.mif.vu.lt/~grig2790/Cyclone/
  http://notaz.gp2x.de/
- Chui (sdl_gp32@yahoo.es) has developed MAME4ALL, the MAME GP2X port for Dreamcast,
  Windows and Linux. Also he has done several optimizations aplicable to all targets.
  http://chui.dcemu.co.uk/mame4all.html
- Slaanesh (astaude@hotmail.com) has continued my work on MAME GP32 and he has 
  done several improvements aplicable to all targets.
  http://users.bigpond.net.au/mame/gp32/
- GnoStiC (mustafa.tufan@gmail.com) has done the USB Joypad support using the 
  library created by Puck2099.
- Sean Poyser (seanpoyser@gmail.com) has done interesting improvements in some drivers.
  For example the use of diagonals in Q*Bert or the use of the shoulder buttons in Tron.
- TTYman (ttyman@free.fr) has done the MAME GP2X port for the PSP portable console.
  http://ttyman.free.fr/
- Headoverheels (davega@euskalnet.net) has added some new games to MAME4ALL, and 
  he has also done some optimizations to existing games.


11. DEVELOPMENT
---------------
January 04,2014 :
- Version 1.2 for GCW0 only, bug-fixes.
December 25,2013 :
- Version 1.1 for GCW0 only, optimizations, games added and bug-fixes.
October 23, 2012 :
- Version 1.0 for OpenDingux only, 1st OpenDingux version.
April 30, 2012:
- Version 2.7: Improved MMU-HACK, solved CAANOO lock-ups, added assembler optimizations.
April 9, 2011:
- Version 2.6. Added WIZ TV-Out support. Added CAANOO port.
March 9, 2010:
- Version 2.5. Version for the RetroMadrid 2010 fair.
August 5, 2009:
- Version 2.4. Improvements in the GP2X WIZ port.
July 11, 2009:
- Version 2.3. Improvements in the GP2X WIZ port. More games added. Fixed controls.
June 13, 2009:
- Version 2.2. New timers system based on integers (instead of floats).
May 31, 2009:
- Version 2.1. Improvements in the GP2X WIZ port. Added Discrete Sound System.
May 16, 2009:
- Version 2.0. GP2X WIZ port added.
December 28, 2008:
- Version 1.6. Optimizations, executable for Neo·Geo games, ASM cores selected automatically.
November 15, 2008:
- Version 1.5. Optimizations and bug-fixes.
September 04, 2008:
- Version 1.4. New games added.
April 05, 2008:
- Version 1.3. Cheats and auto-fire.
March 18, 2008:
- Version 1.2. More bug fix.
March 16, 2008:
- Version 1.1. High scores and bug fix.
March 11, 2008:
- Version 1.0. First version.


12. KNOWN PROBLEMS
------------------
- Not perfect sound or incomplete in some games.
- Slow playability in modern games.


13. THANKS TO
-------------
Franxis: Originally porting MAME to GP32, GP2X and Wiz 
  (http://www.talfi.net/gp32_franxis/).
Slaanesh for Dingoo & Dingux port of mame (http://www.slaanesh.net/).
hi-ban for skins pictures
qbertaddict for tests and videos on youtube 
Without the help of the infos from these people, this emulator can't be here


14. MAME LICENSE
----------------
http://www.mame.net
http://www.mamedev.com

Copyright © 1997-2009, Nicola Salmoria and the MAME team. All rights reserved. 

Redistribution and use of this code or any derivative works are permitted provided
that the following conditions are met: 

* Redistributions may not be sold, nor may they be used in a commercial product 
or activity. 

* Redistributions that are modified from the original source must include the 
complete source code, including the source code for all components used by a binary 
built from the modified sources. However, as a special exception, the source code 
distributed need not include anything that is normally distributed (in either source 
or binary form) with the major components (compiler, kernel, and so on) of the 
operating system on which the executable runs, unless that component itself 
accompanies the executable. 

* Redistributions must reproduce the above copyright notice, this list of conditions 
and the following disclaimer in the documentation and/or other materials provided 
with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
OF THE POSSIBILITY OF SUCH DAMAGE. 
