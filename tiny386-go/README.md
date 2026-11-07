# Tiny386-Go

Play classic DOS PC games on Retro-Go handhelds.

Tiny386-Go brings [Tiny386](https://github.com/hchunhui/tiny386), a PC emulator
by Chunhui He, to Retro-Go. Each game boots from its own DOS disk image, with
handheld buttons mapped to keyboard keys and an on-screen keyboard for occasional
typing. It is best suited to older, less demanding DOS games; compatibility and
speed vary by game and device.

## Features

- Support tested on ESP32, ESP32-S3 and ESP32-P4.
- Native 320x200 output for supported EGA/VGA modes, with Retro-Go display scaling.
- AdLib music/sound, PC speaker and Sound Blaster digital audio support.
- Per-game control remapping, an on-screen keyboard and a menu for special keys.
- Retro-Go menus, screenshots and saved preferences.
- A desktop image builder that adds FreeDOS and starts your chosen game automatically.
- Built-in SeaBIOS and SeaVGABIOS: no separate BIOS download is needed.

## Getting started

You need a handheld running a Retro-Go build that includes Tiny386-Go, a microSD
card and your own installed or unpacked DOS game files. Use your device's Retro-Go
installation instructions to install compatible firmware; this application is part
of that firmware, rather than a program you copy onto the SD card.

1. Open the **[Tiny386 DOS Image Builder](tools/image_builder/README.md)** on your
   computer. Windows users can open `tools/Tiny386-Image-Builder.exe`; Python sources are
   available for Windows, Linux and macOS.
2. Select your game's folder and startup file, then create a game image. The
   default 4 MiB disk is a starting point for small DOS games; increase it if needed.
3. Copy the matching `.img` and `.ini` files to `roms/tiny386/` on your SD card.
   Keep them together with their original names. You can use a subfolder per game.
4. Open **Tiny 386** in the Retro-Go launcher and select the game's `.ini` file.

For example:

```text
roms/tiny386/MyGame/
  MyGame.ini
  MyGame.img
```

The `.ini` describes the emulated PC and points to the disk image. A game `.exe`
on its own cannot boot the emulator. The builder supplies FreeDOS; you do not
need to find an MS-DOS boot disk. Game files are not included.

## Controls

These are the defaults; individual games may need different keys:

| Handheld control | PC key / action |
| --- | --- |
| D-pad | Arrow keys |
| A / B | Ctrl / Alt |
| Start / Select | Enter / Escape |
| Menu / Option | Retro-Go game menu / options |
| Hold Start alone | Show the on-screen keyboard |

In **Emulator Options**:

- **Show Keyboard**: D-pad moves, A types, B closes, Select toggles case and
  Start sends Enter. The game pauses while you choose a key.
- **Remap Controls**: change keyboard bindings for this game, or reset to its
  INI defaults. Changes are saved separately from your game image.
- **Special keys**: send keys such as F1-F12, Tab or Ctrl+C.
- **Audio**: choose AdLib (default) or PC Speaker. PC Speaker can improve speed;
  relaunch the game so it can detect the selected sound hardware. Some games
  also need their own sound setup changed.
- **Render limit**: leave Auto for normal use, or try a lower display update
  ceiling. Lower values can reduce rendering work but do not guarantee faster play.

Retro-Go's display options let you change scaling; fullscreen is the default.

## Saves, memory and compatibility

Use the game's own save feature. Saves are written into its disk image, so keep
a backup of the `.img` before editing or replacing it. VM snapshots/save states
are not supported.

The builder defaults to 2 MiB of guest RAM, XMS enabled and EMS disabled. Disk
capacity and guest RAM are separate settings. Games that need more memory may
work with 4 MiB guest RAM on a device with enough PSRAM; see the builder's
[memory guide](tools/image_builder/README.md#memory-settings). More RAM does not
necessarily make a game faster.

Original ESP32 devices have tighter memory and performance limits. Games such
as Wolfenstein 3D can be slow even when they boot successfully. DOS text screens
may also be hard to read on small displays. Games requiring a mouse, PC joystick
or FPU are outside the current supported feature set.

## More information

- [Image builder guide](tools/image_builder/README.md): create and update game disks.
- [DOS boot files, sources and licences](tools/image_builder/assets/README.md).

## Thanks and credits

Thanks to **Chunhui He (hchunhui)** for creating and maintaining
[Tiny386](https://github.com/hchunhui/tiny386), which makes this port possible.
Thanks also to the [Retro-Go project](https://github.com/ducalex/retro-go) and its
contributors, and to the FreeDOS, FreeCOM, HIMEMX, Jemm and SeaBIOS projects.

Tiny386's [upstream licence](components/tiny386/LICENSE) and the notices for
bundled third-party software are retained in this repository. Please keep the
applicable licences and source distributions when redistributing builds.
