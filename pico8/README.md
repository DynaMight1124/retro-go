# PICO-8 for Retro-Go

This application brings a large selection of PICO-8 cartridges to Retro-Go
handhelds. It is a lightweight, community-developed interpreter and emulator;
it is not the official PICO-8 player and perfect compatibility with every
cartridge is not expected.

The port is based on David Ventura's **PicoPico** project and its modified
**z8lua** interpreter. The original PicoPico code established the Lua dialect,
software renderer, sound engine, cartridge support, and embedded ESP32
foundation used here. This Retro-Go version builds on that work with broader
cartridge compatibility, more complete audio and graphics, multicart support,
and native integration with the Retro-Go launcher and menus.

## Using cartridges

The emulator supports:

- `.p8` text cartridges;
- `.p8.png` cartridge images;
- `.zip` archives containing exactly one `.p8` or `.p8.png` cartridge.

Some games use several cartridges. Keep all files for these games together in
an extracted folder and launch the game's starting cartridge. If a required
companion cartridge is absent, the emulator displays its filename instead of
silently returning to the launcher. ZIP archives containing multiple
cartridges are intentionally rejected because PICO-8 multicart packages do not
use a universal starting filename.

Only use cartridges you are entitled to use. Commercial or paid games are not
included with this port.

## What has been improved

The Retro-Go port has received a substantial compatibility pass covering more
than two hundred games and demos. Work includes:

- many PICO-8 Lua syntax, operator, table, iterator, coroutine, and runtime
  behaviour fixes;
- more complete `.p8` and `.p8.png` loading, compressed code, Unicode tokens,
  and companion-cart handling;
- expanded PICO-8 memory and hardware-register behaviour;
- corrected sprites, maps, palettes, transparency, clipping, lines, triangles,
  textured drawing, fill patterns, P8SCII text, and custom fonts;
- improved music, SFX, instruments, effects, noise, and playback sequencing;
- additional PICO-8 APIs and more accurate argument and return-value handling;
- targeted Lua, map, and rendering optimisations which retain compatibility;
- persistent `cartdata()` game data, reset, screenshots, speed-up, About and
  Options menus, and friendly cartridge-loading errors.

Compatibility is now dramatically higher than the original port, although
unusual carts may still expose missing edge cases. Games designed primarily
for a mouse, keyboard, network access, or unsupported host features may not be
playable on a normal Retro-Go handheld.

## Performance

Performance depends heavily on the cartridge. A simple 30 FPS game can require
only a small fraction of the CPU, while a Lua-heavy 60 FPS game, textured 3D
renderer, or busy scene can become CPU-bound. Automatic render skipping helps
simulation and audio remain correctly paced when a game exceeds the available
frame budget.

### Original ESP32

The original dual-core ESP32 performs better than might be expected. Games
such as Celeste Classic run at their full 30 FPS target, and lighter games are
very usable. Demanding 60 FPS, 3D, or effect-heavy games can slow down
substantially, so this target is best suited to lighter cartridges.

### ESP32-S3

The ESP32-S3 is the best-tested target and offers a good balance of speed and
compatibility. Most tested games reach their intended 30 or 60 FPS rate, with
plenty of headroom in lighter titles. A smaller group of especially demanding
games remains CPU-bound or uses visible frame skipping during heavy scenes.

### ESP32-P4

The ESP32-P4 provides excellent headroom for ordinary games: Celeste Classic,
for example, holds 30 FPS at a low measured CPU load, while Snekburd holds
60 FPS comfortably. Very demanding cartridges still do not scale perfectly
with the faster processor because the Lua VM and software renderer are largely
serial workloads. They are considerably faster than on the original ESP32,
but a few can still miss their target frame rate.

These descriptions are intended as general guidance rather than guarantees;
different scenes within the same game can have very different workloads.
Detailed measured results and individual game notes are available in
[`COMPATIBILITY.md`](COMPATIBILITY.md).

## Saving and other limitations

Games using PICO-8's normal `cartdata()` mechanism retain their supported
progress and settings across restarts. Full Retro-Go save states are not yet
available because a reliable state must preserve the complete live Lua object
graph as well as graphics, audio, memory, and multicart state. The Save and
Load menu actions therefore explain that they are unavailable rather than
creating an unsafe partial state.

Mouse and pointer emulation is also not currently provided. See
[`SAVE_STATES.md`](SAVE_STATES.md) for the technical save-state assessment.

## Thanks and acknowledgements

This port would not exist without the work of:

- **David Ventura**, creator of PicoPico and the modified z8lua foundation;
- the **z8lua** contributors, whose Lua interpreter implements the PICO-8
  dialect;
- **TAC08** and its contributors, whose ideas and firmware work inspired parts
  of PicoPico;
- the **Retro-Go** maintainers and contributors, who provide the handheld
  launcher, display, audio, input, storage, and system framework;
- **Lexaloffle and Joseph White**, creators of PICO-8 and the fantasy console
  ecosystem; and
- everyone who created, tested, documented, and shared PICO-8 cartridges.

Thank you to the original developers for making their work available to build
upon, and to the hardware testers whose repeated cross-game testing made this
compatibility work possible.

PICO-8 is a product of Lexaloffle Games. This is an unofficial community port
and is not affiliated with or endorsed by Lexaloffle.
