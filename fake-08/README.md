# Fake-08 for Retro-Go

This project is a port of **Fake-08**, a popular PICO-8 emulator, to the **Retro-Go** ecosystem. It enables PICO-8 gaming on ESP32-based handhelds (ESP32, ESP32-S3, and ESP32-P4) using the unified Retro-Go system APIs.

## Project Overview

The port is designed around a "Host" bridge architecture. The `Host` class in `main/main.cpp` translates Fake-08's core requirements (Display, Input, Audio, and Storage) into Retro-Go's `rg_x` functions.

### How it Works

*   **Display:** PICO-8's 128x128 4bpp framebuffer is expanded through a palette lookup into double-buffered indexed Retro-Go surfaces. Surface ownership is synchronized with the asynchronous display task.
*   **Memory Management:** Core objects and latency-sensitive state use `MEM_FAST`. The Lua allocator maintains a bounded internal-RAM budget and places overflow allocations in PSRAM.
*   **Math Logic:** Every PICO-8 instruction is handled by a custom Lua VM configured with `fix32` fixed-point numbers to ensure bit-perfection with the original console.
*   **Storage:** Launches use `rg_system_get_app()->romPath`; plain and ZIP-wrapped carts use Retro-Go storage APIs, while browsing and `cartdata()` paths are derived from `RG_BASE_PATH_ROMS` and `RG_BASE_PATH_SAVES`.
*   **Lifecycle:** Retro-Go handlers provide versioned save states, load, reset, screenshots, redraw, shutdown flushing, and launcher cold-resume support. In-session loads unwind and recreate the Lua VM before restoring state because Eris cannot safely replace closures while the live VM remains on the call stack.
*   **Pacing:** Audio submission provides the real-time throttle and blocking menu time is excluded from busy accounting. The port deliberately renders every emulated tick: measured adaptive frameskipping made motion substantially less fluid while saving too little work to improve the Lua-heavy scenes.
*   **Diagnostics:** Initial launch emits a `CART` serial line with the selected filename and path, source format, and resume flag. The first emulated tick adds the actual 30/60 Hz update rate, so performance traces can be attributed to a specific game without misreporting `_update60` carts during initialization.

## Challenges

Porting a Lua-based emulator to a microcontroller with limited RAM and CPU presented several significant challenges:

1.  **The "Hidden" Truncation Bug:** A major hurdle was an `explicit` constructor in the `fix32` class. This caused all fractional numbers in PICO-8 games (like gravity or velocity) to be silently truncated to integers during parsing, resulting in "floating" characters and broken physics.
2.  **Memory Overlap:** PICO-8 has a unique memory map where sprites 128-255 overlap with map rows 32-63. Our initial implementation had these as separate arrays, making many game tiles invisible. This required a strict restructuring of the `PicoRam` union.
3.  **Optimization vs. Correctness:** High compiler optimizations (`O3`) often "optimized away" the signed overflow behaviors that PICO-8 games depend on. We resolved this using specific compiler flags (`-fwrapv`, `-fno-strict-overflow`) and isolating the VM core.
4.  **Heap Stability:** Moving Lua memory between Internal RAM and PSRAM during execution proved unstable. We moved to a "stable-split" model to prevent system-wide crashes (panics) during file operations.

## Measured Performance

The reference measurement is Celeste using the same play route for each build.

| Target and scene | Unedited port | Optimized port | Final observation |
| :--- | :--- | :--- | :--- |
| ESP32-S3, typical rooms | 11-13 FPS | 28-30 FPS | Lua 8-9 ms, audio mix 19-20 ms |
| ESP32-S3, script-heavy room | Not reached in the baseline run | 20-22 FPS | Lua about 20 ms, audio mix about 22 ms |
| Original ESP32, typical rooms | Not measured | 22-24 FPS | 22.8 FPS average before and 22.9 FPS after the heavy room |
| Original ESP32, script-heavy room | Not measured | 12-17 FPS | 14.4 FPS average, followed by immediate recovery |
| ESP32-P4, typical rooms | Not measured | 29-31 FPS | 30.2 FPS average both before and after the heavy room |
| ESP32-P4, script-heavy room | Not measured | 27-30 FPS | 28.7 FPS average, followed by immediate recovery |

The audio work reduced the representative mixer cost from about 37.4 ms to 19.7-21.8 ms without lowering the 22,050 Hz sample rate or removing synth channels, filters, reverb, or effects. The slow-room/fast-room transition is repeatable: Lua time roughly doubles in the slow room and returns immediately afterward, while mixer time remains stable. This indicates game-script complexity rather than cumulative slowdown or a resource leak.

The original ESP32 run completed without panics, watchdogs, or low-stack warnings. Internal free memory remained around 25-26 KiB and external free memory cycled between roughly 3.3 and 3.7 MiB rather than declining monotonically. Its 64 KiB `PicoRam` allocation falls back from `MEM_FAST` to PSRAM because no sufficiently large internal block remains; this is a safe best-effort allocation outcome and likely contributes to the performance difference from ESP32-S3.

The ESP32-P4 run also completed without panics, watchdogs, or low-stack warnings. All core allocations, including the 64 KiB `PicoRam`, remained in internal memory. Internal free memory stayed around 204-209 KiB with a 106 KiB largest block, while external free memory stayed around 31 MiB without a monotonic decline.

## Lifecycle Validation

On CrokPocket, saving multiple Celeste slots, repeated in-session loads, loading a different slot, cold-start resume, and both soft and hard reset all completed successfully. The repeated-load traces contain no errors, panics, watchdogs, or low-stack warnings. Each live load deliberately reconstructs the Lua VM; after restoration, external free memory returns to roughly 5 MiB rather than declining across successive loads. Save-state creation also generates the Retro-Go launcher screenshot successfully.

Visual inspection of the saved screenshot should still be included in a final device acceptance pass, along with exit/shutdown persistence, ZIP loading, and testing another representative cart.

## Build and Profiling

Production build for CrokPocket:

```text
python rg_tool.py --target crokpocket release launcher fake-08
```

Frame-stage profiling is disabled by default. After configuring a target with the command above, it can be enabled independently of Retro-Go's function instrumentation:

```text
cd fake-08
idf.py app -DFAKE08_STAGE_PROFILING=ON
```

Set the option back to `OFF`, or perform a clean/release build, before producing firmware for distribution. Profiling emits a three-second aggregate containing wall, Lua, video, mixer, audio-submit, and other time.

## Performance Boundary

The remaining large-gain options deliberately have not been applied because they change quality or substantially increase correctness risk:

* Synthesizing at 11,025 Hz and duplicating output frames would reduce audio work but lower effective audio/DSP fidelity.
* Moving synthesis to another task could overlap Lua and audio, but Fake-08 shares live SFX RAM and playback state with the VM and would require careful synchronization.
* Invasive Lua dispatch or arithmetic changes could improve script-heavy scenes but risk changing PICO-8 fixed-point and overflow behavior.

The current implementation therefore favors stable audio, timing, and game compatibility over benchmark-only gains.

## Credits

*   **Fake-08:** Original emulator by [jtothebell](https://github.com/jtothebell/fake-08).
*   **Retro-Go:** Unified launcher and system layer by [ducalex](https://github.com/ducalex/retro-go).
