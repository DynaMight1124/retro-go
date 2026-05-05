# ECWolf for Retro-Go

An optimised port of [ECWolf](https://maniacsvault.net/ecwolf/) to the [Retro-Go](https://github.com/ducalex/retro-go) ecosystem, bringing the advanced Wolfenstein 3D engine to ESP32, ESP32-S3, and ESP32-P4 handhelds. Note that performance is subpar on the ESP32, first level plays acceptable but bigger/complex levels really bog down. S3 and P4 are fine.

This port is based on the [ECWolf RS97/RG350 port by gameblabla](https://github.com/gameblabla/ecwolf), adapted and optimised for the Retro-Go unified system layer.

## Features
- **Multi-Game Support:** Automatically detects and boots Wolfenstein 3D, Spear of Destiny (and mission packs), and Super 3D Noah's Ark.
- **Optimised Performance:** Defaults to 320x200 on standard ESP32 for better frame rates, while maintaining 320x240 on S3/P4.
- **Handheld Optimised:** Custom input mapping for Retro-Go, including native weapon cycling and menu access.
- **Clean SD Organisation:** Separates selection files from messy game data and keeps saves/configs in standard Retro-Go directories.
- **Modern Renderer:** Full support for high-res textures, widescreen aspect ratios, and smooth scaling.

## Supported Game Formats
The engine automatically detects the game type based on the file extension selected in the launcher:
- **.wl6 / .wl1:** Wolfenstein 3D (Full/Shareware)
- **.sod / .sdm:** Spear of Destiny (Full/Demo)
- **.sd2 / .sd3:** SOD Mission Packs
- **.n3d:** Super 3D Noah's Ark

---

## Directory Structure

To keep your launcher clean and organised, follow this structure on your SD card:

### 1. Game Selection (Launcher)
Place your "trigger" files here. These are the files you will see in the Retro-Go launcher. They can be empty files and named anything as long as the extension is .wl6, .sod etc, or you can copy a smaller game file and rename.
```
roms/wolf3d/
├── Wolfenstein 3D Full.wl6
├── Spear of Destiny.sod
└── Noahs Ark.n3d
```

### 2. Game Data (The messy files)
Move **all** game data files and the ecwolf.pk3 file into this subfolder:
```
roms/wolf3d/data/
├── ecwolf.pk3 (This is required)
├── GAMEMAPS.WL6
├── VGAGRAPH.SOD
├── AUDIOHED.N3D
└── (All other .WL6, .SOD, .WL1, .pk3 files)
```
Note: You dont need every file above. WL6 files are full Wolf3D, SOD are Spear of Destiny, so if you only want Wolf3D, then you just need all of the WL6 files. In any case, you will need the ecwolf.pk3 file though.

### 3. Saves & Configuration
ECWolf follows the Retro-Go standard for system files:
- **Saves:** `retro-go/saves/wolf3d/`
- **Screenshots:** `retro-go/saves/wolf3d/screenshots/`
- **Configuration:** `retro-go/config/wolf3d/ecwolf.cfg`

---

## Supported Game Formats
The engine automatically detects the game type based on the file extension selected in the launcher:
- **.wl6 / .wl1:** Wolfenstein 3D (Full/Shareware)
- **.sod / .sdm:** Spear of Destiny (Full/Demo)
- **.sd2 / .sd3:** SOD Mission Packs
- **.n3d:** Super 3D Noah's Ark

---

## Controls

| Retro-Go Button | ECWolf Action |
| :--- | :--- |
| **D-Pad** | Movement / Turning |
| **A** | Fire |
| **B** | Strafe |
| **X** | Automap |
| **Y / SELECT** | Cycle Weapons |
| **START** | Open Doors |
| **OPTION** | Run |
| **MENU** | Open Game Menu |
| **L / R** | Strafe Left / Right |

---

## Credits & Thanks
- **Braden Obrzut (Blzut3):** Lead developer of the incredible ECWolf engine.
- **gameblabla:** For the RS97/SDL1.2 port which served as the foundation for this Retro-Go version.
- **ducalex:** Creator of the Retro-Go ecosystem.
- **Id Software:** For the legendary Wolfenstein 3D.
- **Wisdom Tree:** For the "educational" Noah's Ark experience.

---
*Note: This port is provided for use with legally owned game data. Wolfenstein 3D is copyright Id Software; ECWolf is licensed under the GPL.*
