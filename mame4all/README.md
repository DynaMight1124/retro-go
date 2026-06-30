# MAME4ALL for Retro-Go (ESP32-S3 & ESP32-P4 only)

Heres some real person talk: I set out to try to get AI to port MAME4ALL to Retro-Go, I didnt realise at the time that every single driver/game had its own internal memory penalty, initially booted games like Pacman etc quite quickly but soon ran out of internal memory after about 20 or so games, so felt pointless limiting to 20 simple games when MAME4ALL supports over 1600. Went down many rabbit holes and left AI doing its thing for many hours, I am not even sure it knew what it was doing but ended up with something that does support the vast majority of the library on the ESP32-S3 and P4, the whole setup is very flakey and broke when trying to implement ESP32 support, so left it as is. Most games seem to work pretty well, some dont but to be honest, I was done with the project to start chasing fixes on a game by game basis! May aswell release something, so below is the AI talk. 

This is a port of **MAME4ALL (0.37b5)** to the **Retro-Go** ecosystem, specifically optimised for the **ESP32-S3 & ESP32-P4**, this does not work on the ESP32 without an extremely limited (20ish) amount of games. It supports a vast library of arcade classics with advanced memory management to handle the large metadata requirements of MAME.

## Work Completed (Phase 1: Stabilisation)
- **Source Integrity:** Restored and cleaned the MAME4ALL 0.37b5 baseline for the ESP-IDF environment.
- **Linkage Architecture:** Implemented a selective header wrap and surgical implementation prepend strategy to bridge legacy MAME C code with Retro-Go's C++ components.
- **Flash-Resident Metadata:** Developed an automated "Metadata Const-ification" tool that forces ~1MB of driver structures into Flash memory (`.rodata`), preventing internal RAM overflows and runtime panics.
- **PSRAM Mapping:** Strategic mapping of non-initialized state (`.bss`) to PSRAM, allowing for massive driver support (1,600+ games) without exhausting high-speed internal RAM.
- **Binary Success:** Successfully compiled and linked a stable release binary supporting 1,603 arcade games.

---

## Getting Started

### 1. Required ROM Set
This port is based on **MAME 0.37b5**. You must use ROMs from this specific set. Later MAME ROMs or those from other arcade emulators (like FBNeo) will likely not work due to different ROM naming and structure requirements.

### 2. Game Placement
Place your `.zip` game files in the following directory on your SD card:
```
/roms/mame/
```

### 3. How to Play
- Launch the **Retro-Go Launcher**.
- Select the **MAME** category.
- Browse and launch your chosen game.
- **Controls:** Standard Retro-Go mappings apply. Access the MAME internal menu for advanced dipswitch settings (if supported).

---

## Compatibility Guide

### Supported Systems
The current build supports **1,603 arcade games**, including:
- **Classics:** Pac-Man, Galaga, Frogger, Donkey Kong, Dig Dug, Galaxian.
- **80s Hits:** 1942, 1943, Burger Time, Bubble Bobble, Arkanoid, Asteroids.
- **Early 90s:** Snow Bros, Tumblepop, etc.
- **Sega System 16:** Including *OutRun*, *Altered Beast*, *Golden Axe*.

### Currently Incompatible / Disabled Systems
Some larger or more complex systems are currently disabled to maintain memory stability and build integrity:
- **Capcom Play System 1 (CPS1):** Including *Street Fighter II*, *Final Fight*, *Ghouls 'n Ghosts*.
- **Midway Y-Unit/T-Unit:** Including *Mortal Kombat II*, *NBA Jam* (Requires `TMS34010` core stabilization).
- **Technos Hardware:** Including *Double Dragon* (Currently in the process of stabilization).

---

## Customizing the Build (Memory Management)

If you are targeting devices with less memory (like the standard ESP32 or ESP32-S3), you may need to reduce the number of enabled drivers to save RAM and Flash space. This port utilises aggressive linker garbage collection; if a driver is not referenced in the main table, it will not be included in the final binary.

### How to Disable Games/Drivers
You can granularly control which games are included by editing `mame4all/components/mame4all/src/retrogo/rg_drivers.cpp`:

1.  **Locate the Drivers Table:** Scroll to the bottom of the file to find the `const struct GameDriver * const drivers[]` array.
2.  **Comment Out Games:** Simply comment out the lines for games you do not wish to support.
    ```cpp
    const struct GameDriver * const drivers[] = {
        &driver_1942,
        // &driver_1943,  // This game and its associated driver code will be excluded from the final build
        &driver_pacman,
        ...
        0
    };
    ```
3.  **Warning on Clones:** If you enable a clone game (e.g., `driver_1942a`), ensure that its parent driver (e.g., `driver_1942`) is also present in the list (or at least its `extern` declaration is active), otherwise the build will fail with an "undefined reference" error.

4.  **Rebuild:**
    ```powershell
    python rg_tool.py --target [your-target] release mame4all
    ```

By pruning the list in `rg_drivers.cpp`, you can significantly reduce the memory footprint and binary size, allowing MAME4ALL to run on hardware with limited resources.
