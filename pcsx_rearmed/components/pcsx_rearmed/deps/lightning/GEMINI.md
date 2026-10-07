# Project Overview: GNU lightning RV32 Port

**Goal:** Enable dynamic recompilation (`lightrec`) for `pcsx_rearmed` on the ESP32-P4 by porting the `GNU lightning` code-generation backend to support the 32-bit RISC-V (RV32) architecture.

## Architectural Changes Made
The official `GNU lightning` repository only supported RV64 (64-bit). The following architectural modifications were made directly to the `lightning` module to adapt the instruction set and calling convention (ABI) for RV32:

1. **Unblocked RV32 Compilation:** 
   - Removed the `#if __WORDSIZE != 64` `#error` barrier in `lib/jit_riscv.c`.
2. **Calling Convention (ABI) Updates:**
   - Modified `_compute_framesize`, `_prolog`, `_epilog`, and `_vastart` in `lib/jit_riscv.c` and `lib/jit_riscv-cpu.c`. Stack offsets and register sizing (like `RA` and `FP` tracking) now dynamically calculate sizes using `sizeof(jit_word_t)` instead of hardcoded 8/16-byte limits.
3. **Memory Operation Porting:**
   - Updated the generic pointer load/store macros (`ldr_l`, `str_l`) in `lib/jit_riscv-cpu.c` to emit `LW` (Load Word) and `SW` (Store Word) on 32-bit builds, replacing the `LD`/`SD` (Load/Store Double) instructions that trigger illegal instruction faults on RV32.
4. **Immediate Loading sequence:**
   - Ported the 32-bit `movi` logic from the `Lightening` fork. RV32 immediate loading is handled using sequences like `LUI` + `ADDI` (instead of the 64-bit `ADDIW` which expects 64-bit registers). Updated `_movi_p` and `_patch_at` accordingly.
5. **ALU operations:**
   - Adapted `extr_i` and `extr_ui` extensions to fall back to simple `MV` (move) operations on RV32, bypassing the 32-bit bit-shifting hacks previously required for RV64 sign extension.
6. **Constant Pool Isolation:**
   - Wrapped the entire 64-bit PC-relative constant pool subsystem (`_load_const`, `_put_const`, `hash_const`, `get_const`) in `lib/jit_riscv.c` under a `#if __WORDSIZE == 64` block, as the logic relies heavily on 64-bit jumps and `LD` instructions that are unneeded for 32-bit constants.

## Potential Issues & Troubleshooting

### 1. Hard-Float (Double Precision) ABI Mismatches
*   **The Issue:** The ESP32-P4 supports Single-Precision (RV32F) floating point. `GNU lightning` currently attempts to save/restore Double-Precision registers (`fregs`) using `ldxi_d` / `stxi_d` during `_prolog` and `_epilog`. If the ESP32-P4 toolchain targets `ilp32f` rather than `ilp32d`, this could cause assembly errors or stack corruption.
*   **The Solution:** If compilation fails on `stxi_d` macros or double-precision registers, the floating-point context-saving loop in `_prolog` and `_epilog` inside `lib/jit_riscv-cpu.c` will need to be conditionally checked against `__riscv_flen`. Alternatively, `_jitc->framesize` for floats may need to be adjusted from `sizeof(jit_float64_t)` to `sizeof(jit_float32_t)`.

### 2. Unaligned Memory Access
*   **The Issue:** `lightrec` may occasionally emit code that attempts unaligned memory access. RV32 typically faults on unaligned loads/stores unless specifically trapped and handled by the OS/RTOS.
*   **The Solution:** If the emulator crashes with `Load/Store Address Misaligned` exceptions during runtime, check the `jit_cpu.unaligned` flag configuration in `lib/jit_riscv.c` to ensure `GNU lightning` emits safe byte-by-byte shifting loads instead of direct unaligned word loads.

### 3. Jumps exceeding 1MB (JAL offset limits)
*   **The Issue:** RV32 `JAL` instructions have a +/- 1MB relative jump limit. In very large dynamically recompiled blocks, `jmpi` or branch patching (`_patch_at`) might fail if the jump distance exceeds this limit.
*   **The Solution:** If `assert(simm32_p(relative))` or `assert(simm12_p(jmp))` fails during execution, `GNU lightning`'s branching macros may need to be rewritten for RV32 to use a trampoline or an `AUIPC` + `JALR` sequence for long-distance jumps.

### 4. Code Buffer Size / Cache Flushing
*   **The Issue:** Dynamic recompilation requires flushing the instruction cache after writing instructions. The ESP-IDF uses specific functions for cache invalidation on the ESP32-P4 that might differ from standard POSIX `__clear_cache`.
*   **The Solution:** Update `jit_flush` in `lib/jit_riscv.c` to call the ESP-IDF specific `esp_cache_msync()` or `spi_flash_cache_extmem_invalidate()` if emitted code fails to execute.