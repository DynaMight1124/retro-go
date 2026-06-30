# ClassiCube port for Retro-Go

This project is a port of the **ClassiCube** Minecraft Classic client to the **Retro-Go** ecosystem, enabling playable, low-overhead 3D block building on ESP32-based handheld devices (ESP32, ESP32-S3 and ESP32-P4).

ClassiCube is a lightweight, custom rewrite of Minecraft Classic (v0.30) written in C. By leveraging the `retro-go` hardware abstraction layer and optimising graphics, memory allocation, and cooperative threading, this port brings ClassiCube's 3D engine onto resource-constrained microcontroller hardware.

---

## Features & Optimisations
- **Software 3D Rasteriser (`SOFTFP`)**: Employs a custom fixed-point rasterisation backend with 64-bit edge calculations and division precision checks for robust 3D polygon rendering.
- **PSRAM-Optimised Memory Allocation**: Migrates large vertex buffers, map structures, and textures into external PSRAM using `rg_alloc(..., MEM_SLOW)`.
- **Cooperative Threading (`CC_BUILD_COOPTHREADED`)**: Terrain generation and block chunk rebuilding run cooperatively to avoid FreeRTOS task stack panics.
- **Custom Sound Engine**: A custom low-latency audio queueing system integrated with retro-go's I2S audio drivers.
- **Dual-Resolution Scaling**:
  - **Menu/UI**: Renders at the display's native resolution (e.g. 320x240) for readable text.
  - **3D World**: Automatically downscales viewport rendering (e.g. to 160x120 on standard ESP32 boards) to maintain playable framerates, while scaling up on high-end targets like the ESP32-P4.


---

## SD Card Data Files Setup
Since networking is disabled (`CC_DISABLE_NETWORKING` is active) to prevent heap fragmentation and stack panics, you must copy all assets and data files manually to your SD card.

Create the directories on your SD card and copy the files as detailed below:

### 1. Game Texture Packs (Required)
Download the texture packs from the official ClassiCube resources:
- **Minecraft Classic Assets**: Download `default.zip` from [classicube.net/static/default.zip](https://www.classicube.net/static/default.zip).
- **ClassiCube UI Assets**: Obtain `classicube.zip` from the release assets.

Place these files in the following directory:
```
roms/classicube/data/texpacks/default.zip
roms/classicube/data/texpacks/classicube.zip
```
*(If textures are missing, the game will boot to a blank world or fail to display block models).*

### 2. Audio & Sounds (Optional)
If you wish to have walk, jump, and break sounds, place the audio zips in the following directory:
```
roms/classicube/data/audio/default.zip
roms/classicube/data/audio/classicube.zip
```

### 3. Save Files and Settings
The game automatically manages and outputs these files on your SD card:
- **Saved Worlds/Maps**: `retro-go/saves/classicube/maps/`
- **Config & Controls**: `retro-go/config/classicube/options.txt`
- **Error/Debug Log**: `retro-go/saves/classicube/client.log`

---

## Controls Mapping
Because gamepads lack a mouse and full keyboard, the controls use an action modifier mode via the **START** button:

### Normal Mode (In-Game FPS Navigation)
| Button / Key | Game Action |
| :--- | :--- |
| **D-Pad Up / Down** | Walk Forward / Backward |
| **D-Pad Left / Right** | Turn Left / Right |
| **B Button** | Jump |
| **OPTION Button** | Toggle Build Mode / Destroy Mode |
| **A Button** | Trigger Action (Build block if Build mode, Destroy block if Destroy mode) |
| **R Shoulder** | Destroy / Mine block (Left Click shortcut) |
| **L Shoulder** | Build / Place block (Right Click shortcut) |
| **SELECT Button** | Open / Close block inventory menu |
| **START Button (Tap)** | Cycle active hotbar selection slot |
| **MENU Button** | Open / Close Game Pause Menu |

### START Modifier Mode (Hold START Button)
| Hold START + Button | Game Action |
| :--- | :--- |
| **START + D-Pad Up / Down** | Look Up / Down |
| **START + D-Pad Left / Right** | Strafe Left / Right |
| **START + A Button** | Cycle active hotbar selection slot |
| **START + B Button** | Cycle Camera Perspective (First-Person / Third-Person) |

### Menu Navigation & UI Mode
| Button | Menu Action |
| :--- | :--- |
| **D-Pad** | Navigate selections |
| **A Button** | Confirm / Click |
| **B Button** | Back / Close current menu (Escape) |
| **SELECT Button** | Close inventory menu |

---

## Acknowledgements
- **ClassiCube Developers & Contributors**: Special thanks to the original ClassiCube development team for writing a wonderfully modular, highly portable C client of Minecraft Classic.
- **Retro-Go Team**: Thanks to the creators of the `retro-go` framework for the excellent ESP32 system libraries, screen drivers, audio pipeline, and build tooling.
