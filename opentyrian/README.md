# OpenTyrian for Retro-Go

An optimised, feature-complete port of **OpenTyrian** for the **Retro-Go** handheld firmware platform on ESP32, ESP32-S3, and ESP32-P4 devices.

---

## Origins & History

- **Original Game:** *Tyrian* (1995) was developed by **Eclipse Productions** (lead programmer Jason Emery, artist Alexander Liggett) and published by **Epic MegaGames**. In 2004, the game was generously released as freeware by the authors.
- **OpenTyrian:** An open-source C/SDL reimplementation of the Tyrian engine created by the [OpenTyrian Development Team](https://github.com/opentyrian/opentyrian).
- **ESP32 Port:** Initial embedded ESP32 proof-of-concept by **Gadget Workbench (jkirsons)** ([https://github.com/jkirsons/OpenTyrian](https://github.com/jkirsons/OpenTyrian)).
- **Retro-Go Port:** Re-architected as a native Retro-Go application.

---

## Features

- **Full Game Support:** Complete Tyrian 2.1 experience including all 5 episodes, full story mode, arcade mode, ship customization, and secret levels.
- **Authentic Performance & Speed Options:**
  - **Normal (35 FPS):** Faithful to the original 1995 DOS 486 baseline ($70\text{ Hz} / 2 = 35\text{ FPS}$). Runs at 35 FPS on standard ESP32 and sips power on ESP32-S3 / ESP32-P4 (~60% CPU load).
  - **Turbo (1.5x / 50 FPS):** Emulates DOS Tyrian's "Pentium Hyper" mode for fast-paced action on more powerful chips.
  - Seamlessly toggleable on-the-fly via Retro-Go's Game Options menu.
- **Hardware Display:**
  - Native 8-bit paletted Retro-Go surfaces with ownership-safe double buffering.
  - Asynchronous frame delivery (`SDL_Flip`) overlapping gameplay with Retro-Go display processing.
  - Automatic 320×200 to 320×240 vertical aspect-ratio scaling preset on 320×240 screens, fully honoring user display scaling selections.
- **Robust Audio Subsystem:**
  - Native 11,025 Hz game mixing expanded to 22,050 Hz stereo output through Retro-Go's audio driver.
  - Fail-safe muting that zeros DMA buffers and disables the hardware amplifier immediately during in-game or system shutdown, preventing speaker popping or bus-noise buzzing across internal DAC, speaker amp, and external I2S DACs.
- **Integrated Retro-Go Menus & Options:**
  - Retro-Go Game Menu (save state alerts, game resets, quit to launcher).
  - Retro-Go Options Menu: Game Speed (Normal / Turbo), Music toggle, Sidekick firing mode, Wildcard cheats, and Invulnerability.
- **Broad Hardware Compatibility:**
  - Tested and validated on ESP32, ESP32-S3, and ESP32-P4.

---

## Controls

The control layout is tailored for handhelds with classic Retro-Go 2-button, 4-button, and 6-button configurations:

| Button | In Menus | In Gameplay |
| :--- | :--- | :--- |
| **D-Pad** | Navigate Menus / Move Cursor | Ship Movement (Up, Down, Left, Right) |
| **A** | Confirm / Select | Fire Primary Weapon |
| **B** | Back / Cancel | Toggle / fire rear weapon mode |
| **Start** | — | Fire Left Sidekick |
| **Select** | — | Fire Right Sidekick |
| **X / Y** *(if present)* | — | Fire Left / Right Sidekick |
| **L / R** *(if present)* | — | Fire Left / Right Sidekick |
| **Menu** *(Short Tap)* | — | Open In-Game Setup / Pause Menu |
| **Menu** *(Long Hold)* | Retro-Go Game Menu | Retro-Go Game Menu (Save, Load, Quit) |
| **Option** *(Short Tap)* | — | Pause / Unpause Gameplay |
| **Option** *(Long Hold)* | Retro-Go Options Menu | Retro-Go Options Menu (Speed, Audio, Cheats) |

> **Tip:** If your handheld only has face buttons without dedicated Sidekick buttons, you can enable **Sidekicks: Link to A** in Retro-Go's Options Menu so your sidekicks fire simultaneously with your main weapon.

---

## Installation & Required Files

OpenTyrian requires the original data files from **Tyrian 2.1** (freeware - tyrian21.zip).

### 1. File Placement

Copy all Tyrian 2.1 game data files directly into the following directory on your SD card:

```text
roms/opentyrian/
```

### 2. Required Files

Ensure the following data files are present in the folder:

- **Level Files:** `tyrian1.lvl`, `tyrian2.lvl`, `tyrian3.lvl`, `tyrian4.lvl`, `tyrian5.lvl`
- **Data & Tables:** `tyrian.hdt`, `tyrian.cdt`, `tyrian.fdt`, `tyrian.pal`, `palette.dat`
- **Audio Files:** `tyrian.snd`, `voices.snd`, `music.dat` (or individual `.mus` songs)
- **Graphics & Shapes:** `*.shp`, `*.pic`, `*.pcx`

*(Both lowercase and uppercase DOS 8.3 filenames are supported).*

### 3. Saves & Configuration

- **Configuration:** Stored in `retro-go/config/` (`tyrian.cfg` and `opentyrian.cfg`).
- **Save Games:** Stored in `retro-go/saves/opentyrian/` (`tyrian.sav`).

---

## Building from Source

Using Retro-Go's build tool:

```bash
# Build for the original Odroid-go target
python rg_tool.py --target odroid-go build opentyrian

```

---

## Acknowledgements & Credits

Special thanks to:

- **Jason Emery & Alexander Liggett (Eclipse Productions)** for creating *Tyrian*, an unforgettable masterpiece of the DOS shoot-'em-up golden age.
- **Epic MegaGames (Epic Games)** for publishing Tyrian and releasing it as freeware.
- **The OpenTyrian Development Team** for their dedication in reverse-engineering and open-sourcing the C/SDL engine.
- **jkirsons (Gadget Workbench)** for creating the original ESP32 port.
- **ducalex** for developing the Retro-Go platform, drivers, and reference implementations.
