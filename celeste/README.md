# Celeste for Retro-Go

This is a port of [Celeste Classic](https://www.celeste_game.com/classic/) to the [Retro-Go](https://github.com/ducalex/retro-go) ecosystem. It is based on the C-source port `ccleste` and has been optimised for ESP32, ESP32-S3, and ESP32-P4 devices.

## Features

- **Dual Video Modes**: Support for both the original square aspect ratio and a modern widescreen view.
- **Audio Support**: Full SFX and background music support streamed from the SD card.
- **Save States & Auto-Resume**: Full support for Retro-Go save slots and the "Resume" feature from the launcher.
- **Unified Controls**: Fully integrated with Retro-Go's gamepad abstraction.
- **Native Performance**: Optimised triple-buffering and 16-bit rendering for smooth gameplay.
- **Retro-Go Menu**: Standard access to system settings, volume, and quitting via the Retro-Go overlay.

## Installation

To appear in the Retro-Go launcher and have full audio, the following structure is required on your SD card:

1. Create a directory named `celeste` inside your `roms` folder.
2. Create an empty file named `Celeste.p8` inside that folder.
   - Path: `/roms/celeste/Celeste.p8`
3. Create a `data` folder inside `/roms/celeste/` and place the audio assets there. The assets can be obtained from the Github repo.
   - Path: `/roms/celeste/data/`

## Audio Setup

The port expects audio files in **WAV format (22050Hz, 16-bit, Mono)**. This setup allows the flash size to remain small while still supporting audio.

### Required Files
- **SFX**: `snd0.wav` through `snd55.wav`.
- **Music**: `mus0.wav`, `mus10.wav`, `mus20.wav`, `mus30.wav`, `mus40.wav`.

### Converting from OGG
If you have the original OGG music files, you can convert them using FFmpeg with the following command:
```bash
ffmpeg -i input.ogg -ar 22050 -ac 1 -acodec pcm_s16le output.wav
```

## Save States

The port supports the standard Retro-Go save-state system:
- **In-Game**: Use the Retro-Go Menu (Menu Button > Save & x / Load game) to manage multiple slots.
- **From Launcher**: Choosing the "Resume game" option in the Retro-Go launcher will allow you to load one of your saves and put you back into the action instantly.

## Video Modes

You can toggle between video modes in the **Retro-Go Options** menu (Menu > Options > Emulator options):

- **Classic (128x128)**: The original PICO-8 experience. Tighter camera and original difficulty.
- **Widescreen (256x150)**: Based on the "Scrolleste" map. Provides a wider field of view and smooth camera scrolling.

*Note: You must restart the game for Video Mode changes to take effect.*

## Controls

| Action   | Gamepad Button |
|----------|----------------|
| Move     | D-Pad          |
| Jump     | Button A       |
| Dash     | Button B       |
| Pause    | Start          |
| Menu     | Menu Button    |
| Options  | Select/Option  |

## Credits

- **Original Game**: Maddy Thorson & Noel Berry.
- **Original C Port**: [lemon32767/ccleste](https://github.com/lemon32767/ccleste)
- **ESP32 Port**: [valdanylchuk/ccleste](https://github.com/valdanylchuk/ccleste)
- **Retro-Go Integration**: Specialised for the Retro-Go ecosystem.

## License

This port is free software, licensed under CC-BY-SA-4.0.
