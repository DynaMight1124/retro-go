# gpSP for Retro-Go

A **Game Boy Advance emulator** for [Retro-Go](https://github.com/ducalex/retro-go)
handhelds, based on **gpSP**. Play GBA games through Retro-Go's launcher, with
standard menus for controls, display settings, audio and saving.

## ESP32 support

| Chip | CPU emulation | What to expect |
| --- | --- | --- |
| **ESP32** | Interpreter | Basic GBA support. Performance is limited, and many games run too slowly for comfortable play. |
| **ESP32-S3** | Xtensa dynamic recompiler | Faster emulation, with a per-game interpreter fallback for compatibility. |
| **ESP32-P4** | RISC-V dynamic recompiler | Hardware-specific dynamic recompilation for GBA emulation. Performance varies by game. |

Game speed depends on the game, device configuration, available PSRAM and SD card
performance. The emulator caches ROM data in PSRAM and can load additional pages
from the SD card when a game is larger than the cache.

## Features

- **GBA games from the Retro-Go launcher**, using extracted `.gba` files.
- **In-game battery saves** for SRAM, Flash and EEPROM cartridges, including
  games that require 128 KB Flash.
- **Save states and screenshot previews** through Retro-Go's game menu.
- **Automatic real-time clock support** for recognized games that use it,
  including Pokemon Ruby, Sapphire and Emerald.
- **Stereo audio**, with an option to disable sound emulation.
- **Automatic or manual frameskip** to balance drawing and emulation speed.
- **Optional rendering on the second CPU core** and selectable frame buffering.
- **Optional external GBA BIOS**, with a built-in BIOS available by default.
- **Per-game dynarec recovery on ESP32-S3**, with a Fast CPU option to retry or
  switch between the dynarec and interpreter.

## Getting started

1. Put extracted `.gba` games in Retro-Go's GBA ROM directory.
2. Optionally add a GBA BIOS named `gba_bios.bin` to Retro-Go's BIOS directory.
   It must be exactly **16 KB**. If it is missing or invalid, the built-in BIOS
   is used.
3. Launch a game from the **GBA** section of Retro-Go.
4. Open the game or options menu to adjust settings or manage save states.

A typical SD-card layout is:

```text
roms/
  gba/
    Game.gba
retro-go/
  bios/
    gba_bios.bin          # Optional
  saves/
    gba/                 # Created and managed by Retro-Go
```

These are the standard locations; a Retro-Go build can customize its storage
paths. **ZIP archives are not supported**: extract the `.gba` file before playing.

## Controls

| Action | Handheld key |
| --- | --- |
| GBA D-pad | D-pad |
| GBA A / B | A / B |
| GBA L | L or X |
| GBA R | R or Y |
| GBA Start / Select | Start / Select |
| Retro-Go game menu | Menu |
| Retro-Go options menu | Option |

Available keys depend on the handheld's Retro-Go configuration.

## Saves and the cartridge clock

**Battery saves** are the game's own saved progress, separate from Retro-Go's
save states. Save names are generated automatically from the ROM filename.

| File | Purpose |
| --- | --- |
| `Game.gba.sram` | In-game battery save |
| `*.sav` | Retro-Go save states |
| `*.png` | Save-state screenshot previews |
| `Game.gba.sram.rtc` | Cartridge clock metadata, for games using RTC |
| `Game.gba.sram.jit` | ESP32-S3 per-game dynarec recovery information |

ESP32-S3 and ESP32-P4 can write battery saves in the background. On the original
ESP32, pending saves are written when opening a menu or exiting. Use Retro-Go's
menu to leave a game so pending writes can finish.

**RTC is automatic** and needs no emulator setting. It uses valid device time or
the saved cartridge clock, with an offline clock when device time is unavailable.
The clock advances during play and is saved between sessions. Time spent powered
off can only be accounted for when the device has a reliable clock to provide it.

## Emulator options

| Option | Purpose |
| --- | --- |
| **Audio enable** | Enable or disable sound emulation. |
| **Frameskip** | Use Auto, draw every frame, or select a fixed skipping level. Auto is the recommended starting point. |
| **Renderer** | Choose synchronous rendering or the second-core renderer. |
| **Buffering** | Choose single, double or triple buffering. |
| **Fast CPU (dynarec)** | ESP32-S3 only: enable the dynarec or use the slower interpreter. |

The defaults use **double buffering** on all chips, with **second-core rendering
on ESP32-S3/P4** and **synchronous rendering on ESP32**. Renderer and buffering
changes require a restart. Automatic frameskip skips drawing while continuing
CPU emulation, controls and audio.

## Building

From the Retro-Go repository root, with its ESP-IDF environment configured:

```sh
python rg_tool.py --target YOUR_TARGET build gbsp
```

Replace `YOUR_TARGET` with the Retro-Go target for your handheld. Follow the
firmware project's installation instructions for your device.

## Limitations

- Game compatibility and speed vary; full-speed emulation is not guaranteed.
- Games larger than the ROM cache can experience pauses while reading the SD card.
- Link cable/RFU multiplayer and rumble output are not supported.
- Save-state compatibility between different CPU backends is not guaranteed.

## Credits and thanks

This port builds on the work of the following developers and projects:

- **[pjcau](https://github.com/pjcau)**: the Xtensa gpSP dynarec, `xjit`, the
  scanline renderer and the Retro-Go integration for the ESP32-S3 version.
- **[Irak4t0n / HowBoyAdvance](https://github.com/Irak4t0n/HowBoyAdvance)**:
  the initial ESP32-P4 gpSP dynarec work.
- **[rapha-tech](https://github.com/rapha-tech)**: porting the ESP32-P4 version
  to Retro-Go.
- **gpSP contributors**: Exophase for the original emulator,
  [notaz](https://github.com/notaz/gpsp),
  [davidgfnet](https://github.com/davidgfnet/gpsp), and the libretro contributors.
- **[Retro-Go](https://github.com/ducalex/retro-go)**: ducalex and contributors
  for the handheld firmware platform.

## License

Bundled components retain their respective licenses. See
[gpSP's COPYING](components/gbsp-libretro/COPYING) and
[xjit's COPYING](components/xjit/COPYING).
