# Cannonball for Retro-Go

This is a port of the **Cannonball** engine (the enhanced Outrun engine) to the **Retro-Go** ecosystem, specifically optimised for ESP32-based devices. Its full speed on the ESP32-S3 and P4, the original ESP32 is much improved at 20-22fps on the opening scene and between 25-30fps in most of the gameplay.

## Recent updates:
  - RGB conversion: effectively zero
  - Double-buffered PSRAM display
  - Parallel road and sprite rendering
  - Direct native RGB565 output
  - Render-only adaptive frameskipping
  - Audio/render priority contention
  - Several renderer hot-loop costs

## Features
- **30 FPS Arcade Gameplay:** Synced via hardware audio timers for a stable, speed-locked experience.
- **Optimised Synthesis:** Integer-only SegaPCM mixing for high-speed audio on ESP32 hardware.
- **Arcade Accurate Visuals:** Correct RGB565 color expansion and hardware-accurate shadow/highlight rendering.
- **Dedicated Menus:** Support for both the standard Retro-Go Game Menu and direct hardware "Options" button access.
- **Persistent Settings:** Game modes, music settings, and high scores are saved automatically to your SD card.


## Installation
1. Ensure your SD card is formatted and Retro-Go is installed.
2. Place your Outrun ROM files in the following directory:
   `roms/cannonball/`
3. Create an empty file called Outrun.ball (or any file as long as the extension is .ball)
4. **Required ROMs (International Set):**
   - `epr-10187.88` (Z80 Audio)
   - `opr-10193.66` through `opr-10188.71` (PCM Samples)
   - `epr-10380` through `epr-10383` (Master/Slave CPU)
   - Tile, Road, and Sprite ROMs (standard arcade set).

## Controls
| Button | Action |
| :--- | :--- |
| **D-Pad Left/Right** | Steer |
| **D-Pad Up/Down** | Navigate Menus |
| **Button A** | Accelerate |
| **Button B** | Brake |
| **Button X** | Shift to High Gear |
| **Button Y** | Shift to Low Gear |
| **Select** | Insert Coin |
| **Start** | Begin Race / Music Select |
| **Menu / Home** | Retro-Go System Menu |
| **Options** | Direct access to Emulator Options |

## Emulator Options
Access these via the dedicated **Options** button or through the **Retro-Go Menu -> Emulator Options**:
- **Music (YM2151):** Toggle the FM music synthesizer (Doesnt work very well, only stops a few beats and doesnt change performance massively).
- **Game Mode:** Choose between **Original**, **Time Trial**, and **Continuous** modes.
- **Free Play:** Toggle between arcade coin requirements or direct play.
- **New Attract:** Choose between the original arcade or enhanced attraction mode.
- **Fix Engine Bugs:** Enable/disable original arcade engine bug fixes.


## Credits
- **Cannonball Engine:** Created by Chris White.
- **Retro-Go:** The ESP32 emulation ecosystem.
- **Port:** Optimised for ESP32 targets.
