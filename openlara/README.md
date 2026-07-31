# OpenLara for Retro-Go

This project is a high-performance port of the **OpenLara** (Tomb Raider) engine to the ESP32 platform using the **Retro-Go** ecosystem. It is specifically optimized for the ESP32-S3 and ESP32-P4 with at least 8MB of PSRAM.

![OpenLara Retro-Go](assets/retro-go-preview.jpg)

## Features
- **Tomb Raider 1 PC Support**: Fully playable with correct lighting and textures.
- **Software Rasterizer**: Custom 320x240 RGB565 renderer with memory-optimized 16-bit depth buffering.
- **PSRAM Optimized**: Intelligent memory management (`OL_ALLOC`) ensures level data, textures, and sounds reside in 8MB PSRAM.
- **High-Fidelity Audio**: Optimized 11025Hz Mono pipeline with averaging downsampling for clear playback.
- **Save Anywhere**: Direct, one-touch saving and loading via the Passport menu.
- **UI Scaling**: 2x font and interface scaling optimized for small 2-3" displays.

---

## Known Limitations: FMV Videos
FMV video playback is **disabled by default**. 
- **Reason**: The ESP32's CPU cannot simultaneously decode heavy video streams (via the Escape/Cinepak codecs) and maintain the performance required to run the game engine. 
- **Result**: Enabling FMV playback on handhelds causes stuttering, audio crackling, and memory pressure that leads to system panics. 
- **Status**: The engine is configured to skip all intro and cutscene videos to prioritize smooth, stable gameplay.

---

## Installation & SD Card Structure

Assets must be placed in the following directory on your SD card:

`/sdcard/roms/openlara/data/`

### Required Folder Structure:
```text
/sdcard/roms/openlara/data/
├── AUDIO/
│   └── 1/              <-- Place converted .WAV files here
├── DATA/
│   ├── TITLE.PHD      <-- Main entry point
│   ├── TITLEH.PNG     <-- Title background (optimized)
│   ├── LEVEL1.PHD     <-- Level data
│   └── ...            <-- All other .PHD files
└── PIX/               <-- High-res loading screens (optimized)
```

---

## Media Preparation

Original Tomb Raider 1 PC files require conversion for optimal performance on the ESP32.

### Automated Conversion
Use the scripts in the `TR1_PC` folder:
1. Copy your original assets into the `AUDIO`, `DATA`, and `LEVEL` folders.
2. Run `convert_media.bat` (Windows) or `convert_media.sh` (Linux/macOS).
3. The scripts will:
    - Convert audio to **11025Hz Mono WAV**.
    - Resize background images to **320x240 PNG**.
    - Optimize the Title Screen background.
    - Remove the unsupported `FMV` folder.

### Manual Conversion Settings
- **Audio**: 11025Hz, Mono, 16-bit PCM.
- **Backgrounds**: 320x240 PNG (Lanczos scaling recommended).

---

## Saving and Loading
- **In-Game**: Open the Passport menu. Page 0 is **SAVE PROGRESS**, Page 1 is **LOAD PROGRESS**, Page 2 is **EXIT TO TITLE**.
- **Main Menu**: Page 0 is **LOAD GAME**, Page 1 is **START GAME**, Page 2 is **EXIT GAME**.
- **Persistence**: Saves are automatically managed in `/sdcard/retro-go/saves/openlara/`.

---

## Credits
- **Xproger**: Original OpenLara engine.
- **ducalex**: Retro-Go firmware.
- **Core Design / Eidos**: Original Tomb Raider developers.
