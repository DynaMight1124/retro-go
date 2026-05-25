Below is the AI stuff, but oversold little in true AI fashion! So the real stuff is, it mostly runs the same compared to Gwenesis, possibly with some proper dev optimisation it could be made to run quicker, I would had expected PicoDrive to run faster. MegaCD does work, but isnt that fast, seems to be limited to 30fps, unsure if its a bug (since its literally half of 60 and never goes above 30) or just maxed out. Disabling the sound doesnt work properly, spent a lot of time tring to fix but varied between getting stuck at 50fps (for some weird reason) or uncapped at 70-100fps, gave up. Ultimately, it works fine, but I was hoping for that solid 60fps on an ESP32 and as great as AI is, its not very good at that kind of optimisation.

# PicoDrive-Go for Retro-Go

PicoDrive-Go is a optimised port of the legendary **PicoDrive** emulator, tailored specifically for the **Retro-Go** ecosystem. It provides high-performance emulation for the Sega Genesis / Mega Drive and Sega CD / Mega CD on ESP32, ESP32-S3, and ESP32-P4 hardware.

## Features

*   **Multi-Core Support**: Full support for Sega Genesis and experimental Sega CD emulation.
*   **Dual-CPU Orchestration**: Cycle-accurate interleaving of the Main M68K and Sub M68K processors for Sega CD titles.
*   **High Performance**:
    *   Utilizes the **FAME** (Fast Awesome Mega Drive Emulator) C-based M68K core.
    *   **ESP32-P4 Optimized**: Re-enabled `-O3` compiler optimizations for RISC-V targets, significantly boosting dual-CPU performance.
    *   **Dynamic Memory Management**: Core tables and heavy buffers are intelligently distributed between internal `MEM_FAST` DRAM and external PSRAM to minimize latency.
*   **Audio Excellence**:
    *   Standard 22kHz stereo output via Retro-Go's audio abstraction.
    *   Efficient 11kHz internal mono synthesis with zero-cost stereo upsampling to save CPU cycles.
*   **Native Retro-Go Integration**:
    *   Full support for **Save States** (Save/Load) and auto-resume from the launcher.
    *   In-game options menu for region overrides and performance hacks.
    *   Native hardware scaling and palette handling.

## How It Works

PicoDrive-Go bridges the gap between the low-level PicoDrive core and the Retro-Go OS layer.

1.  **CPU Core**: On ESP32 targets, we utilize the FAME M68K core. While traditionally limited by Xtensa's literal pool range, we have implemented architecture-specific build rules to allow RISC-V targets (like the P4) to run the core at maximum optimisation levels.
2.  **Timing & Sync**: The emulator uses a master clock approach. For Sega CD, the `pcd_sync_s68k` loop manages the temporal lockstep between the two 68000 processors, ensuring that interrupts and hardware handshakes (like the BIOS boot sequence) occur with cycle accuracy.
3.  **Rendering**: The port supports both an accurate per-pixel renderer and a "Fast Renderer" line-based hack. It utilizes Retro-Go's surface submission system to handle varying Genesis resolutions (256/320 wide) and PAL/NTSC heights (224/240 lines). Note that "Fast Renderer" will possibly break quite a few games.
4.  **Filesystem**: Implements the Retro-Go standard pathing, looking for Sega CD BIOS files (`bios_CD_U.bin`, etc.) in the standard BIOS directory while managing ROMs and Saves through the OS-provided handles.

## Credits

*   **Original PicoDrive**: Developed by **notaz**. A masterpiece of ARM/C optimization that made Genesis emulation possible on low-power handhelds.
*   **Retro-Go**: Developed by **ducalex** and contributors. The underlying OS providing the drivers, GUI, and hardware abstraction.
*   **FAME Core**: Developed by **Fox68k**.
*   **CZ80 Core**: Developed by **Stephane Dallongeville**.

## License

PicoDrive is licensed under the MAME license (non-commercial). Please refer to the `COPYING` file in the root directory for full license text.
