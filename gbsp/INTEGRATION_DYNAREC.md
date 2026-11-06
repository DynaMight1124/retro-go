# gpSP shared-core integration

The `gbsp` application compiles all three CPU backends from one portable core
and public header set in `components/gbsp-libretro/`.

| Chip | Selected backend | Architecture sources | Retro-Go adapter |
| --- | --- | --- | --- |
| ESP32 | Original interpreter | Shared `cpu.cpp` | Shared `main/main.c`, interpreter path |
| ESP32-S3 | Xtensa dynarec | `xtensa/` plus application-local `xjit` | Shared `main/main.c`, S3 path |
| ESP32-P4 | RISC-V dynarec | `riscv/` plus P4 allocation/bridge | Shared `main/main.c`, P4 path |

`components/gbsp-libretro/core_selection.cmake` controls selection for the adapter,
core and xjit components. No selection file is needed at the application root. The
existing S31 RISC-V selection is retained but has no validated target/toolchain
in this checkout. Core sources use canonical `cpu.cpp`, `video.cpp` and `main.c`
names. There are no active `orig/` or `s3/` copies inside the component.

## Stage 2 boundary

Identical portable code is shared; differences from the working implementations
are preserved under `RISCV_ARCH` and `XTENSA_ARCH`. Consolidation guards carry
`GBSP_BACKEND_VARIANT` comments so the migration audit can reconstruct each
baseline. They preserve complete macro definitions and original preprocessor scopes.
Backend differences include CPU/cache layout, memory access, timing, save-state
fields and S3 renderer hooks. This structural consolidation intentionally preserves
behavior instead of adopting one variant's semantics for every chip.

Assembly, code emitters and executable-cache management remain architecture
specific. The core's P4 compatibility headers route to the full libretro headers
for ESP32/S3, avoiding header shadowing when sharing the component directory.
S3 layout definitions are exported to its adapter so allocations match the core.

The Retro-Go frontend is also shared in `main/main.c`. Its 15 conditional
sections preserve P4's bridge API, the interpreter path, and S3's asynchronous
renderer, battery writer and fallback policy. Common sections occur once; selected
backend code reconstructs the corresponding Stage 1 adapter exactly, apart from
shared include paths. Backend selection defines RISCV_ARCH for the P4 frontend;
S3 inherits its public architecture/layout definitions from the core. The original
ESP32's compatibility is the requirement; interpreter speed tuning is outside this
change. This pass changes source organization while preserving runtime behavior.

The three top-level reference applications `gbsp-orig`, `gbsp-s3` and `gbsp-p4`
remain unchanged. Old active component trees and separate frontend files were moved reversibly to the local,
Git-ignored `tests/artifacts/stage1-active-core-backup/`; the Stage 1 commit also
preserves them. The shared core is edited directly; regeneration is not required.

## Build and recovery

From the repository root:

```powershell
python rg_tool.py --target crokpocket build gbsp
python rg_tool.py --target odroid-go build gbsp
python rg_tool.py --target gb300-p4 build gbsp
```

Clean gbsp before changing device targets. P4 needs the user's established
16384-byte main-task stack setting in the target configuration. This consolidation
does not edit shared platform or target files.

An S3 recovery build retains the original interpreter behavior using the same
shared sources and the original adapter:

```powershell
$env:GBAJIT = "0"
python rg_tool.py --target crokpocket clean gbsp
python rg_tool.py --target crokpocket build gbsp
Remove-Item Env:GBAJIT
```

Clean again before restoring the default dynarec. The variable is read at CMake
configuration time and affects S3 only. S3's default build also retains the
modified core's runtime interpreter fallback, Fast CPU option, and per-game crash
tracking. The experimental upstream GBAJIT_IRAM mode remains unexposed.

## Persistence and runtime behavior

Save states, screenshots and boot resume retain their existing Retro-Go paths.
All cartridge saves use `rg_emu_get_path(RG_PATH_SAVE_SRAM, app->romPath)`; S3 dynarec fallback
metadata uses the same generated name plus `.jit`. The optional official S3 BIOS
is `RG_BASE_PATH_BIOS "/gba_bios.bin"`. Old separate-package `roms/pokegbsp` files
are not migrated. State formats are preserved per backend; cross-backend state
compatibility is not established merely by matching version constants.

S3 keeps its second-core renderer, triple buffering, RTC, background save snapshot
writer, memory placement and custom frameskip policy. It disables the platform
frameskip controller while playing. Reconciling that policy with the reference
Retro-Go loop is deferred, so this source consolidation can be tested independently.

## Shared controls and compatibility features

The functional-unification pass adds common helpers in `main/gbsp_options.h` and
`main/gbsp_controls.h`, using the core's public `gpsp_features.h` constants.

| Option | ESP32 | S3 dynarec | P4 |
| --- | --- | --- | --- |
| Audio enable | Shared | Shared | Shared (new control) |
| RTC detection (automatic; no menu option) | Auto | Auto | Auto |
| Double frame buffering | Shared callback | Triple-buffer renderer retained | Shared callback |
| Frameskip | Existing platform policy | Existing Auto/0/1/2 controller | Existing maximum cap |
| Renderer selection | Synchronous | Core 1 / Sync | Existing synchronous renderer |
| Fast CPU | Interpreter | Existing per-ROM dynarec control | Existing dynarec |

Audio enable retains the `sound` application setting. Disabled output is zeroed,
consumed samples are cleared, and stereo frame counts are preserved for pacing.
Output conversion now uses the same P4-derived soft clipper on all targets,
implemented by 1540 bytes of read-only lookup knots and 32-bit interpolation.
All signed-16 inputs are within two PCM units of the previous P4 output; enabled
ESP32/S3 output is no longer the original hard clamp plus 16x gain. Their volume
can be lower at the same platform setting. Backend ring sizes remain unchanged.
This also retains the earlier fix for stale output while muted.

RTC always uses the existing game-database/ROM autodetection on all targets.
There is no RTC menu option or saved override. Old `rtc` settings are ignored,
including previously stored Off/On values. Games without a detected clock leave
RTC disabled; detected RTC games use the shared persisted clock automatically.
The startup log reports effective RTC enabled/disabled status after detection.
All three now use the shared real-elapsed cartridge clock (`gpsp_clock.h` and
`gpsp_persistence.c`), independent of emulation FPS, pause, reset and savestate
rewind. Its initial epoch is the later of usable device time and a versioned `.sram.rtc`
sidecar. Zero/negative or beyond-year-9999 device dates are ignored. Without a
usable saved/device clock it starts at 2000-01-01 noon UTC, advances from the
monotonic timer and persists normally. Noon avoids a pre-2000 local date at UTC-12. Later device-time corrections are accepted without backwards jumps.
The clock advances from the monotonic Retro-Go timer; elapsed time while fully
powered off remains unknowable without a trustworthy external clock. Missing or
invalid metadata falls back to usable device time or the offline year-2000 baseline. P4's legacy savestate clock fields
remain readable but do not control the new clock.

All input paths now use the same GBA button mapping. L or X maps to GBA L, and R
or Y maps to GBA R; the direct-core callback translates this into libretro's bit
positions. MENU/OPTION remain platform menu controls.

Renderer strategies and frameskip policies retain the Stage 2 behavior. `main/gbsp_save.h` now handles cartridge saves on every backend with
unchanged raw `.sram` files and an optional clock sidecar. S3/P4 reserve one
128 KiB PSRAM snapshot before the ROM cache. A core-0 worker owns it from queue
submission until completion acknowledgement. Failed writes and writes made while
a snapshot is in flight stay pending. Automatic saves wait for two seconds without
backup writes, with five seconds between attempts. S3 retains its display/paging
quiet gate; P4 does not defer on a continuously animated screen. RTC metadata is
checkpointed every minute when the worker and quiet gate permit, and at lifecycle
flushes. Original ESP32 (including S3 interpreter fallback) retains its ROM-cache
budget and saves synchronously at menus, reset, state load and shutdown. Missing
background resources use that same lifecycle fallback. Startup loading is always
synchronous, before running the cartridge. Lifecycle flushes await worker completion
before storage can unmount; shutdown offers retry on actual write failure.
Loaded/detected capacity is retained across reset to protect 8 KiB EEPROM saves.
A successful emulator state load marks its cartridge contents pending explicitly.

## Verification

```powershell
python -m unittest discover -s gbsp/tests -v
python gbsp/tests/verify_build.py crokpocket s3 --label stage2
```

The tests retain exact comparisons for unchanged core sections, emulation loops,
and save/load/reset/screenshot/event handlers. Intentionally upgraded settings,
input and audio-output functions use native behavior tests rather than requiring
continued equivalence to the old implementation. Native tests compile the actual
production C reader/helpers and P4 ROM-loader bridge with MSVC on Windows; they
check mute/drain/wrap, unchanged enabled gain, shoulder masks, feature-mode bounds,
settings persistence, removal of the RTC menu/legacy setting and Auto-only ROM loading. Native tests
are skipped when the MSVC test toolchain is unavailable. Firmware builds remain
the integration check for actual ESP-IDF declarations, preprocessing and linkage.

The build verifier requires shared CPU/video/memory/audio/main sources, the shared
frontend, isolated architecture sources/dependencies, and matching backend defines.
Stage 2 test images use `tests/artifacts/stage2-*.bin`; the functional-control
pass uses `tests/artifacts/features-*.bin`. Both are separate from historical
Stage 1 images; matching ELF files are preserved for future crash decoding. Logs and binaries are Git-ignored. Build-only checks never access
serial devices. `docs/stage2-validation.md` records the consolidation build matrix;
`docs/functional-unification-validation.md` records the newer control/RTC pass.

## Hardware checks

On crokpocket, odroid-go and gb300-p4, repeat the same games and transitions used
for Stage 1. Check launch, controls, audio, menus, state save/load, screenshots,
resume and sustained gameplay. Record free/largest memory blocks and stack minima.
Check P4's race-start path with the existing 16 KiB stack. Compare S3 Core 1/Sync
rendering, dynarec/runtime fallback and original-interpreter recovery builds; check
battery-save persistence and RTC games. Preserve reference implementations until
these comparisons pass. Hardware testing remains user-owned.

## Attribution

gpSP is GPL-2.0 software by Exophase and contributors. The RISC-V backend and P4
integration derive from HowBoyAdvance by Irak4t0n. The Xtensa backend, xjit and S3
renderer/adapter are credited in the supplied snapshot to pjcau and contributors.
Original copyright notices and `COPYING` files remain with the sources.


## Diagnostic logging

Application/core logs use the Retro-Go logger with the common `gbsp` tag.
Low-level P4 JIT instruction/block/register dumps (`tba`, `tr`, `jit`, `eat`, BX
and ROM-read traces) are disabled by default, even in debug firmware. For focused
backend debugging only, compile the core and adapter with
`GPSP_DYNAREC_TRACE=1`; release builds disable this switch unconditionally.
Legacy TRACE_INSTRUCTIONS also requires that explicit trace gate before it emits
per-instruction callbacks. Dynarec cache/emitter/invalid-jump failures remain
Retro-Go errors; interpreter-fallback/SMC problems remain warnings.

All three backends share `main/gbsp_perf.h`: every five seconds debug firmware
reports frames/drawn count and average execute, renderer-wait, display and audio
stage time plus current frameskip. P4/ESP32 have no separate renderer-wait stage,
so that field is zero. S3 frameskip change/residency diagnostics are debug only;
its controller and measurements that drive real frameskip remain active in
release. The old S3-specific combined JIT/page/LCD report has been replaced by the
common timing summary; existing platform heap/stack/FPS reporting is unchanged.
Debug performance state and extra timer reads compile out in release. Both local
component CMake files explicitly propagate RG_BUILD_RELEASE so the logger macros
see the correct build mode; no shared platform changes are required.

The tested RTC/save checkpoint was pushed as 85bc2b9 before this logging pass.
Logging changes remain separate and await hardware regression verification.


## Shared sound output

`gpsp_audio_format.h` owns the 32768 Hz sample rate and a 549-stereo-frame maximum
transfer block. Native GBA timing produces 548.625 frames per emulated frame;
the old 547-frame limit could accumulate P4 backlog. ESP32/S3's existing 60 FPS
cadence still produces 546/547. The block grows by eight bytes; rings remain
2048 interleaved samples for ESP32/S3 and 32768 for P4. Only the returned number
of complete stereo frames is submitted, including short/empty reads and mute.
The 512-interleaved-sample producer guard is unchanged.

`gpsp_audio_output.h` is generated by `tests/generate_audio_curve.py` and provides
one conversion for all backends. No new filters, resampling, worker task, heap
allocation or per-sample wide multiply is added. Actual S3 disassembly confirms
one 32-bit multiply in the conversion loop, no calls or wide arithmetic, and
exactly two 770-byte read-only tables. This is code inspection, not a hardware
performance measurement. Ring/FIFO/mixer synthesis and CPU frame timing remain.

The user-preferred P4 response is the reference. S3/ESP32 retain less digital gain
and softer peaks than their original output. Clipping distortion and insufficient
emulation throughput are distinct; neither the curve nor a larger transfer cap
can create samples for simulation that has not run. Compare the same ROM/scene
at similar listening volume and check performance counters and long sessions.
See docs/audio-unification-validation.md for native/build checks and hardware work.
