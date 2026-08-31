# gpSP RISC-V dynarec integration (from HowBoyAdvance)

The GBA app (`gbsp/`) was switched from the old interpreter-only gpSP core
(`gbsp-libretro`) to the newer gpSP core with a **RISC-V dynarec** taken from
HowBoyAdvance by Irak4t0n (GPL-2.0, same license as gpSP already in retro-go).

## What changed

- **New core**: `gbsp/components/gpsp/` (copied from HowBoyAdvance `components/gpsp`).
  Self-contained: platform glue in `gpsp_esp.c`, exposes a small C API in `gpsp_esp.h`.
  Built with `HAVE_DYNAREC MMAP_JIT_CACHE RISCV_ARCH` — NOT with `RETRO_GO`.
- **`gbsp/components/gpsp/CMakeLists.txt`**: retro-go build; `ROM_BUFFER_SIZE=8` (PSRAM
  ROM cache in MB; raise to 16/32 if the board has the PSRAM — big ROMs still work via
  SD page-swapping).
- **`gpsp_esp.[ch]`**: added `gpsp_set_framebuffer()` (host owns the display surface)
  and `gpsp_state_size()` (416 KB) so `main.c` stays decoupled from core internals.
- **`gbsp/main/main.c`**: rewritten to drive the core through `gpsp_esp.h` only
  (init → load_rom → loop{set_buttons, run_frame, get_audio}) with retro-go `rg_*` I/O.
- **`gbsp/CMakeLists.txt`**: component list `gbsp-libretro` → `gpsp`.
- **`gbsp/main/CMakeLists.txt`**: `COMPONENT_REQUIRES "retro-go gpsp"`.

## How the dynarec works on P4

- JIT cache is allocated in PSRAM, then a SECOND executable virtual mapping of the same
  physical pages is created via `esp_mmu_map(..., MMU_TARGET_PSRAM0, MMU_MEM_CAP_EXEC, ...)`
  (`gpsp_esp.c`). This is what makes PSRAM executable.
- I-cache coherency after emitting code: `esp_cache_msync` (D-cache writeback) + `fence.i`
  (`cpu_threaded.c` `platform_cache_sync`, `RISCV_ARCH` branch).

## Build

    python rg_tool.py --target esp32-p4 build-img --no-networking

(display stays the retro-go SPI ILI9341 path — the PPA/DSI scaler from HowBoyAdvance is
NOT used here; the core renders 240x160 and rg_display scales in software.)

## Risks / things to verify on first build

1. **`.incbin "bios/open_gba_bios.bin"`** in `bios_data.S` — must resolve at assemble time
   (works when the component dir is on the assembler path, as in HowBoyAdvance).
2. **`-nostdlib++` global link option**: fine because only the gpSP .cc files are C++
  (built `-fno-exceptions -fno-rtti`, no STL). Don't add C++ that needs libstdc++.
3. **Pixel format**: core outputs RGB565-LE; the P4 screen is 565-BE, so rg_display does a
   per-pixel swap (existing path). Optional later optimization: bake `__builtin_bswap16`
   into `convert_palette` and mark the surface `RG_PIXEL_565_BE`.

## Attribution (GPL-2.0)

gpSP © Exophase and contributors; RISC-V dynarec + ESP32-P4 glue © Irak4t0n
(HowBoyAdvance). Keep these notices if you distribute builds.
