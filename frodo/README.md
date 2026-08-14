# Commodore 64 (Frodo) for Retro-Go

Play Commodore 64 software on Retro-Go handhelds using an embedded port of
[Frodo V4](https://github.com/cebix/frodo4). The port uses Frodo Lite with
precise CPU and CIA timing, full 1541 drive emulation where needed, PAL video,
SID audio, cartridge banking, save states and a handheld-friendly virtual
keyboard.

[Retro-Go](https://github.com/ducalex/retro-go) provides the launcher and the
common menus, display, sound, input, screenshots and save-state interface. The
same Frodo application runs on original ESP32, ESP32-S3 and ESP32-P4 devices.

## Game formats

The formats normally shown by the Retro-Go launcher are:

| Format | Support | Recommendation |
|---|---|---|
| `.prg` | Supported | Usually the quickest and most efficient choice. Programs load directly without emulating a running 1541 drive. |
| `.crt` | Supported | Also an excellent choice. Cartridges start quickly and avoid drive overhead; common simple and banked cartridge types are supported. |
| `.t64` | Supported | Fast direct loading like PRG, although tape archives often contain cracktros, menus or documentation that require keyboard input. |
| `.d64` | Supported | Best when a game needs its original disk layout or custom loader. Loading is authentically slower and full 1541 emulation uses more CPU. |
| `.zip` | Supported | Convenience wrapper for one supported game image. Put the game itself first in the archive, not a folder entry. Performance after extraction is the same as the contained format. |

If the same release is available as a clean PRG or CRT, prefer that version.
Use D64 when the program depends on disk loading, multiple files or disk writes.
Different releases may contain different cracks or game code, so one format is
not guaranteed to behave identically to another.

X64 and G64 images are accepted by the disk core, including from ZIP files,
but are secondary, less-tested formats and are not normally advertised by the
launcher. TAP is deliberately unsupported. An unsupported file inside a ZIP
produces an error rather than being treated as another format.

D64 and X64 files are copied to a writable cache when launched. Games can
write to this working disk, while the original image on the SD card remains
unchanged.

## Controls

| Handheld control | C64 action |
|---|---|
| D-pad | Joystick directions |
| A or B | Joystick fire |
| Short START | F1 |
| SELECT | Space |
| Y or L | F1 on devices with those buttons |
| X or R | Space on devices with those buttons |
| START + SELECT | RUN/STOP |
| START + A | Y |
| START + B | N |
| SELECT + A | Return |
| Hold START for 700 ms | Open the virtual C64 keyboard |
| MENU | Retro-Go game menu |
| OPTION | Retro-Go options menu |

Some C64 games use joystick port 1 and others use port 2. Change **Joystick**
under Retro-Go's emulator options if a title screen works but the game does
not respond. Port 2 is the default.

The key chords take priority over joystick fire, preventing the accompanying
button from also reaching the game. Short START sends F1 only when START is
released, so opening the virtual keyboard does not accidentally press F1.

## Virtual keyboard

Hold START until the keyboard appears. Move with the D-pad, press A to type the
highlighted character, and press B to close it. The keyboard stays open after
each character and remembers the current position during that session, making
words and repeated letters easier to enter.

It contains digits, letters, space, comma, period and question mark. The direct
shortcuts above remain useful for common cracktro questions and title screens.

## Retro-Go features

The game menu provides save/load slots, automatic resume, screenshots, soft
reset, hard reset and the normal Retro-Go display, audio and system options.

- Save states cover PRG, T64, CRT and disk sessions, including cartridge bank
  state and the complete writable D64/X64 working image.
- A soft reset resets the C64 hardware while preserving RAM, similar to a warm
  machine reset.
- A hard reset recreates the machine and launches the selected game again.
- Screenshots capture the most recently completed C64 frame.

Save states belong to the exact game image and Frodo engine version that made
them. A state from a different image, an incompatible build or a damaged file
is rejected instead of being partially restored.

## Compatibility notes

Frodo Lite is used on every ESP32 family because the cycle-exact Frodo SC
engine cannot maintain full PAL speed even on the tested ESP32-P4. Lite runs at
the intended 50 Hz on supported hardware and gives the best practical balance
of compatibility and performance. A small number of games with demanding
mid-scanline raster effects may still show graphical imperfections.

C64 software varies enormously, especially cracked and modified releases. If
a title fails, try another clean release or another supported format before
assuming the original game is incompatible.

## Thanks and licensing

Many thanks to Christian Bauer and all Frodo contributors for the C64 emulator,
and to the Retro-Go authors and contributors for the ESP32 platform, launcher
and device support. Thanks also to everyone testing games and hardware across
the ESP32, ESP32-S3 and ESP32-P4 families.

Frodo's upstream source snapshot and provenance are under
[`components/frodo/upstream/`](components/frodo/upstream/README.retro-go.md),
with its license in
[`components/frodo/upstream/COPYING`](components/frodo/upstream/COPYING).
