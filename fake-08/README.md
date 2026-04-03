# Fake-08 for Retro-Go

This project is a port of **Fake-08**, a popular PICO-8 emulator, to the **Retro-Go** ecosystem. It enables PICO-8 gaming on ESP32-based handhelds (ESP32, ESP32-S3, and ESP32-P4) using the unified Retro-Go system APIs.

## Project Overview

The port is designed around a "Host" bridge architecture. The `Host` class in `main/main.cpp` translates Fake-08's core requirements (Display, Input, Audio, and Storage) into Retro-Go's `rg_x` functions.

### How it Works

*   **Display:** PICO-8's 128x128 4bpp (indexed) framebuffer is expanded into an 8bpp `rg_surface_t`. The blitter uses 32-bit word reads and pointer arithmetic to minimize memory bus overhead.
*   **Memory Management:** A "Greedy Internal" allocation strategy is used. Small, high-frequency Lua objects (< 128 bytes) are prioritized for fast Internal SRAM, while larger buffers and bytecode are pushed to PSRAM.
*   **Math Logic:** Every PICO-8 instruction is handled by a custom Lua VM configured with `fix32` fixed-point numbers to ensure bit-perfection with the original console.
*   **Storage:** Cartridges are loaded directly from the SD card (`/sd/roms/pico8/`), and `cartdata()` is mapped to the Retro-Go save partition.

## Challenges

Porting a Lua-based emulator to a microcontroller with limited RAM and CPU presented several significant challenges:

1.  **The "Hidden" Truncation Bug:** A major hurdle was an `explicit` constructor in the `fix32` class. This caused all fractional numbers in PICO-8 games (like gravity or velocity) to be silently truncated to integers during parsing, resulting in "floating" characters and broken physics.
2.  **Memory Overlap:** PICO-8 has a unique memory map where sprites 128-255 overlap with map rows 32-63. Our initial implementation had these as separate arrays, making many game tiles invisible. This required a strict restructuring of the `PicoRam` union.
3.  **Optimization vs. Correctness:** High compiler optimizations (`O3`) often "optimized away" the signed overflow behaviors that PICO-8 games depend on. We resolved this using specific compiler flags (`-fwrapv`, `-fno-strict-overflow`) and isolating the VM core.
4.  **Heap Stability:** Moving Lua memory between Internal RAM and PSRAM during execution proved unstable. We moved to a "stable-split" model to prevent system-wide crashes (panics) during file operations.

## Performance Reality

While the emulator is accurate and stable, users should have realistic expectations regarding performance on ESP32 hardware. PICO-8 is more CPU-intensive than 8-bit systems like the NES due to the overhead of the Lua VM.

| Target | Typical FPS | Playability |
| :--- | :--- | :--- |
| **ESP32 (Original)** | 7 - 15 FPS | Small, simple games only. |
| **ESP32-S3** | 10 - 20 FPS | Most "slow" games are playable. Heavy games (Celeste) are choppy. |
| **ESP32-P4** | 15 - 25 FPS | Solid for many titles. Complex games run at decent speeds. |

**Note:** Larger, more complex PICO-8 games will likely run poorly. Smaller, logic-light games are generally playable.

## Potential Future Improvements

To achieve a rock-solid 30/60 FPS, the following hardware-level optimizations are recommended:

*   **DMA Blitting:** Using Direct Memory Access to push pixels to the display while the CPU calculates the next frame's logic.
*   **Dirty Rectangle Tracking:** Only re-drawing the parts of the screen that changed since the last frame.
*   **RISC-V Math Acceleration:** Leveraging the ESP32-P4's RISC-V extensions for faster 32-bit fixed-point multiplication.
*   **Computed Gotos:** Rewriting the Lua VM core to use GCC labels-as-values, eliminating the overhead of the massive `switch` statement during instruction dispatch.

## Credits

*   **Fake-08:** Original emulator by [jtothebell](https://github.com/jtothebell/fake-08).
*   **Retro-Go:** Unified launcher and system layer by [ducalex](https://github.com/ducalex/retro-go).
