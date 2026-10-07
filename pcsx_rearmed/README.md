# PCSX-ReARMed for Retro-Go

A PlayStation emulator port for Retro-Go handhelds, based on PCSX-ReARMed.
The ESP32-P4 version uses dynamic recompilation and can run both 2D and 3D
games. Performance and compatibility vary by game; full-speed emulation is
not guaranteed.

## Hardware support

- **ESP32-P4:** the primary supported platform. Lightrec, using GNU Lightning
  with an RV32 backend, is the default CPU core. An experimental RV32 port of
  `new_dynarec` is also selectable in Emulator Options.
- **ESP32-S3:** uses the software MIPS interpreter automatically. It has been
  tested to compile and boot games, but performance is generally too low for
  practical gameplay.


## Features

- Software GPU rendering at the game's original resolution, without enhanced
  2x rendering. Output larger than 320x240 is reduced to fit the port's display
  surface; Retro-Go handles display scaling and aspect ratio.
- NTSC and PAL games, with emulation pacing based on the detected video rate.
- Built-in HLE BIOS; no external BIOS file is required.
- Optional stereo sound through Retro-Go, with Fast and Accurate settings.
- Retro-Go save states, cold resume, reset, and screenshots.
- A shared 128 KiB memory card in slot 1, used by all games.
- Standard digital PlayStation controls and Retro-Go menus.

## Build

Run these commands from the Retro-Go repository root with the ESP-IDF
environment configured. Substitute the target name for your device.

Build the P4 application:

```bash
python rg_tool.py --target gb300-p4 build pcsx_rearmed
```

Build the S3 application:

```bash
python rg_tool.py --target crokpocket build pcsx_rearmed
```

To package the launcher and emulator into an image:

```bash
python rg_tool.py --target gb300-p4 build-img launcher pcsx_rearmed
```

Use Retro-Go's normal flashing workflow for the resulting firmware. An
application-only build is useful when building or updating the launcher
separately.

## Games and storage

Place games in `roms/psx/` on the SD card and launch them from Retro-Go's
Playstation list. The current launcher lists these formats:

- **CHD (`.chd`):** compressed disc images; the main format used for hardware
  testing.
- **BIN (`.bin`):** raw disc images. Keep the matching CUE sheet and all track
  files together. The core looks for a companion CUE sheet when opening a BIN;
  select the BIN from the launcher.
- **PBP (`.pbp`):** compressed PlayStation disc images supported by the core.
  Compatibility testing is less extensive than for CHD.

The frontend does not currently provide a disc-swap menu or M3U playlist
support. Multi-disc games requiring a disc change are therefore not fully
supported.

Use complete, valid disc images when investigating compatibility problems.
Modified or "slim" images with video or audio removed can behave differently
from the original game.

### BIOS

All supported targets currently boot using the built-in HLE BIOS. External
BIOS files are ignored, and there is no BIOS selection option. HLE
compatibility varies by game.

The real-BIOS lookup code is retained in `main/main.c` behind the disabled
`PCSX_REAL_BIOS_ENABLED` guard for development. It uses `RG_BASE_PATH_BIOS`
(`retro-go/bios/` on the SD card). Real BIOS operation is not a supported user
mode in the current build.

### Memory card and save states

The emulator creates a formatted slot-1 card if it does not already exist:

```text
retro-go/saves/psx/pcsx-card1.mcd
```

Every game uses this same card, like sharing one physical PlayStation memory
card. It has 15 usable blocks in total. Slot 2 is disabled. Back up the card
before replacing or deleting it, and avoid powering off while a game saves.

In-game memory-card saves are separate from Retro-Go save states. Use the
Retro-Go menu for save/load and save-and-continue operations. Save states are
not guaranteed to remain compatible across emulator builds or CPU-core
changes; memory-card saves are preferable for long-term progress.

Unreadable or incomplete memory-card images are disabled with an error
message instead of being overwritten. Restore storage access and relaunch
before saving again. Save-state loading validates the complete file before
applying it; real-BIOS states cannot be loaded in this HLE-only build. Loading
temporarily needs roughly 4 MiB of free PSRAM and fails safely if unavailable.

## Emulator options

Options are saved between launches.

| Option | Default | Behavior |
| --- | --- | --- |
| Sound | Off | Enables or disables audio processing/output. Off reduces audio work rather than simply muting the output. |
| Sound quality | Fast | Fast disables SPU interpolation and reverb. Accurate enables them at a higher CPU cost. Output remains 44.1 kHz stereo. |
| CPU cycle multiplier | 4.0 | Selectable from 2.0 to 4.0 in 0.5 steps. Higher values underclock the emulated CPU and reduce host work. |
| CPU core (next launch) | Lightrec | P4 only. Selects Lightrec or the experimental RV32 dynarec; restart the game to apply it. |

The CPU cycle multiplier trades emulated CPU timing for speed. **4.0 is a
performance-oriented default, not a compatibility setting.** If a game has
physics, animation, or timing problems, try 2.0 first. For example,
Carmageddon's vehicle physics can break at higher multipliers. Lower values
can substantially reduce performance.

Lightrec is the recommended P4 core. The RV32 dynarec runs games but remains
experimental, with game-dependent performance and compatibility. Hold
**SELECT while launching** to restore Lightrec if an RV32 selection prevents
you from reaching the menu.

## Controls

| Retro-Go input | PlayStation input |
| --- | --- |
| D-pad | D-pad |
| A / B | Circle / Cross |
| X / Y | Triangle / Square |
| L / R | L1 / R1 |
| START / SELECT | Start / Select |
| MENU | Retro-Go game menu |
| OPTION | Retro-Go options menu |

The current mapping does not expose L2/R2 or analog-stick controls. Games
requiring those inputs may need additional frontend support.

## Performance and limitations

Performance depends on the game's CPU, geometry, GPU, and audio workload.
Turning sound off and increasing the CPU cycle multiplier can help, but the
multiplier may affect game behavior. Frameskip reduces rendered frames while
the emulator continues advancing; a high reported total frame rate does not
necessarily mean that the same number of distinct frames reaches the screen.

Expect slowdowns in demanding 3D scenes and uneven audio when emulation cannot
keep up. The port has been tested with games including Ridge Racer,
Destruction Derby, Resident Evil, and PAL Carmageddon, but this is not a
comprehensive compatibility guarantee.

Detailed frame and renderer diagnostics are disabled by default. Developers
can enable `PCSX_PORT_PROFILE` through CMake or use Retro-Go's profiling build
configuration. RV32 development notes are in [NDRC_RV32.md](NDRC_RV32.md).

## Credits and license

Thanks to the PCSX and PCSX-ReARMed contributors, including notaz, the
Lightrec and GNU Lightning projects, and the Retro-Go contributors.

PCSX-ReARMed is distributed under GPLv2; see
[COPYING](components/pcsx_rearmed/COPYING). Bundled dependencies retain their
own licenses and copyright notices.
