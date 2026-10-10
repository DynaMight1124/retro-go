# Quake II for Retro-Go

`quake2-go` brings classic Quake II to Retro-Go handhelds using the ESP32-P4.
It is based on the standalone **ESP32_QUAKE2** port, which in turn uses
**quake2generic**. This adaptation uses Retro-Go for display, audio, controls,
storage, menus, and returning to the launcher.

The port renders at **320 x 240** using Quake II's software renderer. Development
and hardware testing have been carried out on the **GB300-P4**. ESP32 and
ESP32-S3 are not supported.

## Game files and SD card setup

Game data is **not included**. Use the `baseq2` PAK files from the original
Quake II installation or the original demo. `pak0.pak` is required; also copy
`pak1.pak` and `pak2.pak` if they are present in your installation.

Place the files on your Retro-Go SD card in this layout, starting at the card's
root:

```text
roms/
  quake2/
    pak0.pak
    pak1.pak    (if supplied with your installation)
    pak2.pak    (if supplied with your installation)
```

Alternatively, put `pak0.pak` and any additional PAKs directly in
`roms/quake2/baseq2`; the `baseq2/` folder is optional. The selected PAK
determines which directory is used, so keep all PAKs for that installation
together in the same folder.

Keep the PAK files intact: do not extract their contents or wrap them in ZIP
archives. Windows executables and DLLs are not needed. Data from the remastered
release has not been validated.

Launch Quake II from Retro-Go and select `pak0.pak`.

## Controls

Default gameplay bindings are:

| Button | Action |
| --- | --- |
| D-pad Up / Down | Move forward / backward |
| D-pad Left / Right | Turn left / right |
| A | Fire |
| B | Jump |
| X | Crouch (hold) |
| Y | Use selected inventory item |
| L / R | Strafe left / right |
| Start | Next inventory item |
| Select | Next weapon |
| Menu (short press) | Quake II menu |
| Menu (hold 500 ms) | Retro-Go menu |
| Option, where available | Retro-Go options |

In Quake II menus, use the D-pad to navigate, **A** to confirm, and **B** to go
back. **A** also confirms the game's Quit prompt and returns to the launcher.

## Options and saves

Retro-Go's Emulator options include **Frameskip** (Auto or Off; default Auto)
and **Show FPS** (default Off). Quake II's own options remain available through
its in-game menu.

Use Quake II's built-in **Save Game** and **Load Game** menus. With the default
Retro-Go storage layout, configuration and native save slots are stored at:

```text
/retro-go/config/quake2/config.cfg
/retro-go/saves/quake2/save0/
/retro-go/saves/quake2/save1/
```

`save0/` is the native autosave slot; manual saves use `save1/` and later slots.
The engine also uses `current/` under `/retro-go/saves/quake2/`.
No additional `baseq2/` or `save/` directory
is needed here; `baseq2/` is optional in the game-data layout under `roms/`.

These are native Quake II saves. Retro-Go save-state slots and automatic resume
are not implemented. Save compatibility uses an explicit version marker rather
than rejecting every new firmware build; incompatible save-format changes may
still require a version change.

## Optional music

Music streams from `roms/quake2/music/track02.wav`, `track03.wav`, etc.
For PAKs directly in `roms/quake2/baseq2`, use `roms/quake2/baseq2/music/` instead.
Music always belongs in a `music/` folder beside the selected PAK.
Numeric names such as `02.wav` are also accepted. Use **22,050 Hz stereo,
signed 16-bit PCM WAV**; MP3/OGG files need conversion first. The supplied
[converter and setup instructions](tools/README.md) explain how to prepare them.

The native **CD music** option enables/disables playback. Retro-Go's volume and the port's output trim apply to the combined audio. Missing or unsupported tracks do not prevent playing the game.

The current focus is the original single-player `baseq2` game.
Mission packs, mods, network multiplayer, and USB keyboard/mouse
support are outside the currently validated Retro-Go feature set.

## Building

From the repository root, in an ESP-IDF (5.5.1) environment, build the GB300-P4 package
with:

```text
python rg_tool.py --target gb300-p4 release launcher quake2-go
```


## Credits and thanks

Thank you to the developers whose work made this port possible:

- **id Software and the original Quake II team**, including **John Carmack**,
  for the game, engine, and [Quake II source release](https://github.com/id-Software/Quake-2).
- **Alejandro Villegas Alonso ([alexkid77](https://github.com/alexkid77))**, for
  the **ESP32_QUAKE2** port and its ESP32-P4 optimizations, which form the basis
  of this adaptation. See the [standalone project's README](../ESP32_QUAKE2-main/README.md).
- **ozkl**, for [quake2generic](https://github.com/ozkl/quake2generic), the
  portable engine foundation.
- **ducalex and the Retro-Go contributors**, for
  [Retro-Go](https://github.com/ducalex/retro-go) and its handheld platform APIs.
- **BitsForPeople and ctag-fh-kiel**, for the
  [ESP32-P4 SIMD memory routine reference](https://github.com/ctag-fh-kiel/esp32p4_memcpy_pie_benchmark),
  and **Larry Bank**, for the SIMD guidance acknowledged by the upstream port.
- **Espressif Systems**, for the ESP32-P4 and ESP-IDF.
- **The Quake-Go port in this repository**, for the WAV streamer and music
  conversion tool adapted for Quake II.

The Quake II engine carries its original GPL notices. Other components retain
their upstream attribution and license notices. The game's data files remain
separate from the engine source license and are not distributed with this port.
