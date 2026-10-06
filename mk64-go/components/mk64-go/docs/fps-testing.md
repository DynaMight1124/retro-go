# MK64 Portable -- "fps-optimized" test build (for testers)

This is an experimental build, separate from the ad hoc test build.  It installs
next to your normal copy and does not touch it or its save.

EBOOT.PBP MD5 `1be2d5b35816d3fcd9350e7bc88410c1` (build 2).

Build 2 (2026-09-20) over build 1: a rare freeze of two to three seconds in the
middle of a race is addressed (the game no longer does any I/O while a race
runs; the log is written out at a pause or at the end of the race), and if a
freeze ever happens again the log says where the time went (`stall: ...`).

## What is new

- **60 fps in 1P races** (Grand Prix and Time Trial).  The game still simulates
  exactly what it did; it now shows a picture after every simulation step instead
  of every second one.  2P-4P, the menus and the results screen stay at 30 fps.
  When a stretch of track is too heavy the game drops to 30 fps for a few seconds
  and comes back by itself, rather than running slow.
- **Sound mixing on the PSP's second processor** (the Media Engine), which frees
  the main CPU for the game.  This needs `mk64k.prx` next to the EBOOT -- a small
  kernel helper.  Without it the game still runs and mixes on the main CPU.
- A much faster renderer (clipping, vertex work, how triangles reach the GE).
- The save now keeps a backup copy (`eeprom.bak`), so an interrupted save cannot
  wipe your progress.

## Installing

1. Copy the `MK64PortableFPS` folder to `ms0:/PSP/GAME/`.  It sits next to
   `MK64Portable`; nothing is overwritten.
2. Copy your Mario Kart 64 (USA) ROM into `MK64PortableFPS` as well.
3. Start it from the XMB (it shows as a second "MK64 Portable").  The first start
   builds the game data from the ROM once, as after any install, and starts with
   an empty save.

Ad hoc play: this build only plays against the **same** build.  A PSP on the
other test build is refused ("A PSP HAS ANOTHER GAME VERSION").

## What to look at

1. Hold SELECT for 3 seconds to show the FPS counter.  In a 1P race it should
   read about 60, with occasional drops to 30 on the heaviest stretches (race
   starts with all eight karts, DK's Jungle Parkway near the ferry).
2. Does the picture look right everywhere?  Things we had to fix on real
   hardware, so worth a look: the ground right under and beside the kart, the
   edges of the screen, the results screen after a Grand Prix race (two replays
   and two score panels), Lakitu when he shows the lap, distant scenery popping
   in or out.
3. Does the sound play at normal speed and cleanly -- music, engine, items, the
   announcer?
4. Does anything freeze?  A freeze at the Nintendo logo or when you press HOME
   would point at the Media Engine helper: delete `mk64k.prx` (or create an empty
   file `data/nome`) and try again.

## If something is wrong

Send `data/log.txt` and `data/log_prev.txt` from the `MK64PortableFPS` folder,
and say which course and roughly where.  Finish or pause the race before you
exit: the log is only written out then.

These empty files in `MK64PortableFPS/data/` switch parts off, to narrow a
problem down:

| File | Effect |
| --- | --- |
| `fps30` | no 60 fps: the game runs as before |
| `nome` | sound mixing on the main CPU |
| `nodirect` | the renderer's older, slower path to the GE |
| `novfpuoc` | the renderer's vertex tests in plain C |
