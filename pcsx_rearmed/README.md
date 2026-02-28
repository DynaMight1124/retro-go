# PCSX-ReARMed Retro-Go Port

## Description
This project is a highly proof-of-concept (POC) port of the **PCSX-ReARMed** emulator to the **Retro-Go** framework, mostly to see if it will actually boot a game (spoiler: it does! but very unplayable). It is specifically optimised for ESP32-based handheld devices, including the **ESP32-S3** and the **ESP32-P4**, untested on the original ESP32. It obtains about 7-9FPS on the P4, however it feels a lot slower than that.

## Original Developer
The core of this emulator is based on **PCSX-ReARMed**, which was developed and highly optimized for embedded devices by **notaz**. His work on ARM-based optimizations provided the foundation for this portable implementation.

## Key Features
- **PS1 Core Support**: Full PSX emulation using the stable MIPS interpreter.
- **Retro-GO Integration**: Seamless integration with the Retro-GO frontend, supporting standard input, display, and storage APIs.
- **Advanced Memory Mapping**: Custom linker fragments and relocation strategies to manage large core structures (`psxRegs`, `rcnts`, `cdr`) in SPIRAM, overcoming the DRAM limitations of the ESP32-S3.
- **Guaranteed VRAM**: Pre-allocated and aligned 1MB VRAM buffer in SPIRAM to ensure rendering stability.
- **HLE BIOS**: Built-in High-Level Emulation of the PSX BIOS for improved compatibility without requiring external files.

## Drawbacks & Limitations
- **No Dynarec for RISC/Xtensa**: Currently, the emulator relies entirely on the interpreter core. The highly efficient ARM dynamic recompiler (dynarec) from the original project is not compatible with Xtensa (S3) or RISC-V (P4) architectures.
- **Performance Considerations**: Running PS1 emulation via an interpreter on current ESP32 hardware is computationally demanding. Playable speeds currently require significant frameskipping (setting `frameskip` to 7 or higher) and virtual clock overclocking (`cycle_multiplier` set to 400+).
- **Architecture Maturity**: As a proof-of-concept, some advanced hardware features of the ESP32 (like DMA-assisted rendering or SIMD instructions) are not yet fully utilized.
- **Sound**: No sound implementation as wasn't a lot of point, since the framerate was so poor.

## Future Improvements
- **Dynarec Implementation**: The single most impactful improvement would be the development of a JIT compiler for Xtensa and RISC-V architectures.
- **SIMD / PIE Optimizations**: Leveraging the ESP32-S3's PIE instructions and the ESP32-P4's SIMD capabilities to accelerate the GTE (Geometry Transformation Engine) and GPU operations.
- **DMA-Backed Rendering**: Utilizing the ESP-IDF DMA drivers to speed up the transfer of rendered spans from the GPU plugin to the display surface.
- **Plugin Refinement**: Further stripping and optimizing the SPU and GPU plugins specifically for the ESP32's unique memory and cache constraints.
- **Add Sound**: Hardly much point until it displays at a solid framerate.

## Build/Installation
Building requires the Retro-Go toolchain and the `rg_tool.py` script.

### For ESP32-S3 (CrokPocket exampled):
```bash
python rg_tool.py --target crokpocket build-img launcher pcsx_rearmed
```

### For ESP32-P4 (GB300-P4 exampled):
```bash
python rg_tool.py --target gb300-p4 build-img launcher pcsx_rearmed --no-networking
```

## Usage
1. Flash the resulting `.img` file to your target device.
2. Store PSX game files (`.chd` or `.bin/.cue`, note that only .chd was tested) in the `/sd/roms/psx/` directory.
3. Select and launch the game from the Retro-Go menu.

## License
This port is distributed under the **GPLv2 License**, in accordance with the original PCSX-ReARMed licensing.

## Acknowledgements
- **notaz**: For the legendary PCSX-ReARMed core and ARM optimizations.
