# PocketSNES for Retro-Go

PocketSNES is an older, speed-focused Snes9x-derived SNES emulator adapted as a
standalone Retro-Go application. The port uses Retro-Go for content selection,
display, audio, input, settings, save slots, screenshots, and lifecycle events.

## Current integration

- Renders directly into two native RGB565 Retro-Go surfaces. Submitted buffers
  are alternated only after a rendered frame, and the last complete surface is
  retained for redraws and screenshots.
- Mixes and submits correctly counted stereo audio on core 1. Audio submission
  provides pacing without being charged to `rg_system_tick()` busy time.
- Drains a silent audio job before opening a blocking Retro-Go menu, preventing
  the audio worker from racing menu mute/deinitialization.
- Polls the gamepad once per emulated tick and suppresses buttons that remain
  held when a menu closes.
- Supports plain ROMs and the first file in a ZIP through Retro-Go storage APIs.
  ROM storage is selected from the actual input size; an image that cannot fit
  fails during initialization instead of being silently truncated.
- Loads battery-backed SRAM on startup, uses the core's dirty/autosave timer,
  and flushes dirty SRAM before menus and shutdown.
- Implements Retro-Go save/load/reset/screenshot handlers and boot-resume state
  loading.
- Allocates large caches, ROM data, and cold state in PSRAM. Timing-sensitive
  CPU, PPU, renderer, DMA, and tile-validity state is explicitly requested in
  internal RAM and logged at startup for verification.
- Allocates the 512 KiB BS-X RAM area only for a detected BS-X cartridge rather
  than reserving it for every game.

## Controls

`MENU` opens the Retro-Go game menu and `OPTION` opens PocketSNES options. The
Controls option selects a mapping appropriate to devices with either two or
four face buttons. On two-button devices, the Type A/B/C profiles expose the
remaining SNES buttons through MENU-modified combinations.

Buttons still held after dismissing a menu are ignored until released.

## Options

| Option | Effect | Default |
| --- | --- | --- |
| Audio enable | Enables emulated audio output; disabled output remains silent but paced | On |
| Sound Echo | Enables SNES DSP echo processing | On |
| Sound Interpolation | Enables interpolated audio mixing | On |
| Transparency | Enables SNES colour math/transparency | On |
| Controls | Selects the SNES-to-Retro-Go key mapping | Target-dependent |

## Measured status

The optimization work is measurement-led. On the original ESP32 target in
the Super Mario World test scene, the initial port measured about 41.6 active
FPS. Moving audio mixing/pacing to core 1, enabling safe little-endian word
access, removing loop unrolling, and retaining component-local jump-table
optimization raised the same run to about 47.8 active FPS. Increasing the audio
work queue to two entries and moving the primary Z buffer into internal RAM
raised the final measured result to about 49.7 FPS. The stock Retro-Go Snes9x
baseline was about 53.7 FPS in that scene.


## Credits

PocketSNES and Snes9x contributors created and maintained the emulator core.
Retro-Go contributors provide the platform, launcher, and device abstraction
used by this application.
