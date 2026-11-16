# GTA Vice City for Retro-Go

A Retro-Go port of the GTA Vice City demake, based on the C translation of
[aidencantcode's GTA Vice City J2ME project](https://github.com/aidencantcode/gtavcj2me).
Explore Vice City, drive vehicles and play missions on your handheld.

This is a demake, rather than the original PC or console game. The port includes
Retro-Go menus, audio, screenshots, checkpoint saves and optional streamed radio.
It has been tested on ESP32-S3 and ESP32-P4 handhelds; performance varies by device
and game settings.

## Setup

1. Install Retro-Go firmware for your device that includes the GTA Vice City port.
2. On your SD card, create `roms/gtavc/`.
3. Copy the **contents** of [Files to SD](Files%20to%20SD/) into that folder.
4. In the Retro-Go launcher, open GTA Vice City and select `GTA Vice City.gta`.

Your SD card should look like this:

```text
roms/
  gtavc/
    GTA Vice City.gta
    assets/
      mapidx.bin
      anim.bin
      ...remaining supplied files and folders...
```

Keep all supplied asset files and their folder structure. The `.gta` file is
just a launcher entry: it can be empty and you can rename it, keeping the `.gta`
extension. You do not need to copy the source code or converter onto the SD card.

Existing installations with assets directly inside `roms/gtavc/` also work;
select an existing `.bin` file to launch as before.

## Menus and saves

- **Tap MENU** to open the game's menu; **hold MENU** for the Retro-Go game menu.
- **Tap OPTION** to switch weapons; **hold OPTION** for Retro-Go options.
- Use the game's options for resolution, draw distance, HUD and radio settings.
  Retro-Go options provide display scaling, volume and frameskip settings.
- Saves record game progress as checkpoints, rather than an exact frozen frame.
  Native Continue and Retro-Go save slots are supported.

Keep using the same launcher entry for your Retro-Go save slots: renaming the
entry or switching from a `.bin` entry to `.gta` gives it a separate set of slots.

## Optional radio stations

The game works without radio files. To add stations, use the included
[Radio Converter](tools/radio_converter/README.MD) on Windows, Linux or macOS.
You need Python 3.9 or newer, FFmpeg and your original Vice City `.adf` radio files.

1. Put the `.adf` files in `tools/radio_converter/import/`.
2. Open a terminal in `tools/radio_converter/` and run:

   ```sh
   python convert_radio.py
   ```

   Use `python3` on Linux/macOS if needed.
3. Copy the numbered WAV files from `export/` into `roms/gtavc/assets/radio/`.
4. Enable radio in the game's options and enter a vehicle.

The default conversion creates up to **30 minutes per station**, in the supported
8 kHz mono WAV format. Up to **15 stations** are supported, named `01.wav` through
`15.wav`. You can use fewer stations, choose another duration or convert full-length
tracks; see the converter's README for setup and options. Radio files are streamed
from the SD card, so longer tracks primarily need more storage space.

## Credits

Thanks to aidencantcode for the original J2ME demake, the developers of its C port,
and the Retro-Go contributors for the handheld platform.
