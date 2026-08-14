# ngp-go Performance Status and Next Steps

## Project overview

`ngp-go` is the Retro-Go port of RACE, a Neo Geo Pocket and Neo Geo Pocket
Color emulator. RACE emulates the TLCS-900/H main CPU, the Z80 sound CPU,
SN76489-style tone/noise generation, DAC audio, cartridge flash and the NGP
scanline video hardware.

The port now has a conventional Retro-Go adapter around the original RACE
core. It uses Retro-Go for ROM loading, display submission, audio, input,
menus, SRAM, save states, screenshots and application lifecycle. The current
implementation targets ESP32, ESP32-S3 and ESP32-P4 devices with PSRAM.

This document is concerned only with performance. Integration correctness is
covered by the implementation and hardware test history.

## Interpreting the measurements

The debug line emitted by `record_performance()` reports two different rates:

- `frame`: completed emulated video ticks per second. This determines gameplay
  and audio speed.
- `draw`: frames actually rendered and submitted to the display per second.

When the original ESP32 cannot complete a rendered tick in time, ngp-go skips
rendering for one tick but continues CPU simulation, input and audio. A result
such as `frame=42 draw=21` therefore means approximately 70% real-time
emulation with every other frame displayed; it does not mean the game is
running at 21% speed.

Measurements should always quote both values, together with `core`, `draw`,
`skip`, `audio_drop` and the test scene.

## Performance work already retained

The following optimizations are present in the current source and have either
measured positively or are necessary to avoid known overhead:

1. **Release optimization for the RACE component.** The core is compiled with
   `-O3` and Retro-Go's interpreter-friendly compile options. The CZ80 core
   retains its own `Ofast` optimization.

2. **Fast TLCS memory access.** `NGP_OPTIMIZATION_16BIT_READ` is enabled and
   the common TLCS byte memory accessors are inlined into the interpreter's
   IRAM hot path. Aligned and unaligned word/long reads avoid unnecessary
   byte-at-a-time work where it is safe to do so.

3. **IRAM placement of measured hot code.** The TLCS execution loop, decoder
   entry points, selected high-frequency instruction handlers and the main
   scanline renderer functions are marked for IRAM. This includes sprite
   sorting/drawing, scroll-plane drawing and scanline composition.

4. **DRAM placement of interpreter dispatch data.** TLCS opcode dispatch
   tables and small frequently accessed constants are kept in internal DRAM
   instead of PSRAM or flash-backed data cache.

5. **Deliberate memory placement.** The 64 KiB main RAM and 2,208-byte sound
   CPU RAM are internal. Cold CPU ROM data is in PSRAM. The two native
   160x152 RGB565 big-endian framebuffers are split between internal RAM and
   PSRAM, leaving more internal memory for CPU-active state.

6. **Native, asynchronous display submission.** RACE renders directly into
   Retro-Go-compatible RGB565 big-endian surfaces. Double buffering avoids a
   conversion/copy pass and obeys asynchronous display ownership. Measured
   submit cost on the original ESP32 is normally only about 10-20 us, so the
   LCD submission call is not a material bottleneck.

7. **Render-only one-frame recovery.** When a frame overruns, ngp-go skips the
   next render but never skips simulation, controls or audio. Retro-Go's
   generic escalation to as many as five consecutive skipped frames is
   disabled because RACE's simulation-only ticks remain expensive and long
   skip runs made presentation much worse without restoring full speed.

8. **Asynchronous, correctly paced audio.** Sound generation runs in a
   dedicated Retro-Go task. On faster targets the two-frame audio queue applies
   the real-time cap instead of dropping work, holding emulation at 60 Hz. The
   wait is excluded from reported busy time. On the original ESP32 the queue
   normally does not fill, so this cap does not reduce its performance.

9. **Aligned ROM allocation.** ROMs are loaded with 16 KiB alignment to give
   PSRAM/cache access a predictable boundary.

10. **Low-overhead profiling telemetry.** The port records emulated rate,
    rendered rate, rendered/skipped core time, display submission time,
    display lateness and audio drops. Output is debug-only.

## Experiments that should not simply be repeated

- A direct TLCS dispatch experiment caused severe guest timing errors in
  Pac-Man and was reverted. Any future dispatch rewrite must preserve cycle
  accounting and be tested for guest speed, not merely host FPS.
- Sprite and palette caches achieved very high hit rates but did not lower
  frame time; Pac-Man was slightly slower. They were removed. A future cache
  must eliminate underlying decode/render work through dirty tracking rather
  than add comparisons around work that is already cheap.
- Moving both framebuffers into internal RAM was not justified by the measured
  sequential framebuffer cost. One PSRAM framebuffer is currently a good use
  of memory because it reserves internal RAM for less predictable CPU access.

## Current measured performance

### Original ESP32/CYD

Results vary with the scene, but the useful sustained ranges are:

| Game/scene | Emulated ticks (`frame`) | Submitted frames (`draw`) | Typical core cost |
|---|---:|---:|---:|
| Sonic, lighter/title scenes | about 45-50/s | about 22-25/s | about 20-22 ms/tick |
| Sonic, demanding gameplay | about 40-43/s | about 20-22/s | about 23-24 ms/tick |
| Pac-Man | about 43-49/s | about 21-25/s | about 20-23 ms/tick |

Rendered Sonic ticks have measured roughly 26-29 ms in demanding sections;
simulation-only ticks are still roughly 19 ms. This is the key constraint:
renderer work matters, but the TLCS/Z80/timer simulation alone can already
exceed the 16.67 ms budget.

There are no material display-submit or audio-drop costs in the ESP32 runs.
Optimization should therefore remain focused inside RACE rather than on the
Retro-Go display API.

### ESP32-S3 and ESP32-P4 context

- The S3 was already around the 60 Hz boundary before real-time capping. Its
  measured core cost was approximately 15-16 ms in the tested Sonic scenes.
- The P4 completes Sonic ticks in approximately 8.5-10 ms. With audio pacing
  it holds about 60.4-60.5 emulated ticks/s, normally renders 57-60 frames/s,
  reports zero audio drops and has roughly 40-50% CPU headroom.

The P4 result demonstrates that the emulator's guest timing is sound when the
host has sufficient headroom. The remaining optimization target is primarily
the original ESP32, with the S3 useful as a near-60 regression target.

## Optimization roadmap

Gain estimates below refer to emulated ticks per second on the original ESP32
in CPU-heavy gameplay. They are ranges, not additive guarantees. Every stage
must be measured in the same repeatable Sonic and Pac-Man scenes.

### Stage 1: Easy, low-risk work

**Realistic gain:** 0-3 FPS, approximately 0-7%. This may improve consistency
more than the headline average. It is unlikely to turn a 42 FPS Sonic scene
into 50 FPS by itself.

Work:

1. Add short-lived cycle profiling around TLCS execution, timers, Z80 sound
   execution, sprite work and scroll-plane work. Measure rendered and skipped
   ticks separately.
2. Add opcode/decode-family counters for representative 30-second captures.
   Do not leave per-instruction timers enabled; counters are much cheaper.
3. Inspect the ESP32 linker map and runtime allocation capabilities for every
   small hot table: TLCS register-pointer tables, flag tables, CZ80 flag tables,
   renderer scratch lists, palette working sets and sound-chip state.
4. Replace hot `malloc`/`calloc` allocations with explicit `rg_alloc(...,
   MEM_FAST)` only where profiling proves they are not already internal. Keep
   large or cold allocations in PSRAM.
5. Review the top few opcode handlers for missed `static inline`, redundant
   address masking, repeated `get_address()` calls and avoidable 8/16/32-bit
   conversions.
6. Compare `-O3` and carefully scoped `-Ofast` on individual arithmetic-heavy
   files. Validate save states, audio pitch and guest timing before retaining
   it. Do not apply unsafe flags globally.

Profiling required:

- Record average and p95 rendered/skipped core time, not only FPS.
- Capture internal free heap, largest internal block, IRAM usage and binary
  size before and after placement changes.
- Confirm `audio_drop=0` and unchanged guest speed on ESP32, S3 and P4.

**Stage gate:** retain a change only if it produces at least about a 2% repeatable
core-time reduction or removes a significant p95 spike. If the complete stage
finds less than 3%, stop pursuing memory-placement micro-tuning.

### Stage 2: Medium, structural interpreter and renderer work

**Realistic gain:** 4-8 FPS, approximately 10-20%. A good result could move
Sonic from 40-43 to roughly 46-50 FPS and Pac-Man into roughly 50-55 FPS.
Locked 60 remains unlikely because simulation-only Sonic ticks currently cost
about 19 ms.

Work:

1. Profile the TLCS decoder by opcode family and addressing mode, then shorten
   only the dominant paths. Candidates include specialized ROM/RAM fast paths,
   carrying an already-masked address through a handler and avoiding repeated
   generic memory-map resolution.
2. Investigate a verified computed-goto or compact threaded interpreter. Start
   with one decode family behind a compile-time experiment. Preserve the exact
   cycle result of every handler and compare deterministic state/checksum traces
   against the current interpreter.
3. Predecode immutable ROM basic information such as instruction length,
   addressing class or handler index into PSRAM, while keeping the small active
   lookup working set internal. Do not predecode writable RAM without explicit
   invalidation.
4. Split TLCS execution profiling into instruction dispatch, timers/interrupts,
   scanline scheduling and Z80 synchronization. The simulation-only cost must
   fall; optimizing only pixels cannot reach real time.
5. Redesign graphics caching around VRAM write-time dirty tracking. Decode
   changed 2-bpp patterns or tile metadata once when written, rather than
   checking/caching every scanline. Measure the write-side overhead as well as
   the render saving.
6. Reduce repeated per-scanline setup by building per-frame sprite visibility
   and priority lists where hardware behavior permits. Validate mid-frame
   register and VRAM changes with games that use raster effects.
7. Examine whether tone/noise and DAC generation can use smaller lookup-driven
   or fixed-point loops without changing output timing.

Profiling required:

- Produce per-subsystem microseconds for rendered and skipped ticks.
- Use deterministic input and periodic state hashes to detect interpreter
  timing divergence.
- Test more than Sonic and Pac-Man, including games with heavy sprites, raster
  effects, DAC audio, monochrome mode and flash saves.
- Track cache hit rate only alongside net core-time reduction.

**Stage gate:** continue only if one or two subsystems account for most of the
remaining time and a prototype reduces total tick cost by at least 8-10% with
no guest-timing differences. If medium work cannot bring demanding Sonic below
about 21 ms per rendered tick, a stable 50 FPS is a more realistic endpoint.

### Stage 3: Hard, high-risk core redesign

**Realistic gain:** 8-15 FPS, approximately 20-40%, if successful. This is the
only stage with a plausible route to 55-60 emulated ticks/s on the original
ESP32, but the engineering cost and regression risk are high. A broadly locked
60 FPS should be treated as an aspirational result, not a commitment.

Work:

1. Replace the large function-per-opcode TLCS interpreter with a deliberately
   cache-efficient core. Options include a compact threaded interpreter,
   decoded basic-block interpreter or a new table layout designed for the
   ESP32 instruction/data cache behavior.
2. Consider a small basic-block translator for immutable cartridge ROM. ESP32
   cannot execute code from ordinary PSRAM, so translated executable blocks
   would require a tightly managed internal executable cache. RAM-resident code
   needs invalidation and makes this substantially harder.
3. Write architecture-specific fast paths only after profiles identify a small
   dominant primitive. Xtensa ESP32, Xtensa/LX7 S3 and RISC-V P4 must have
   separate guarded implementations with a correct portable fallback.
4. Rework the scanline renderer into dirty-decoded tiles plus span composition,
   minimizing branches and repeated palette/pattern extraction. Preserve all
   mid-scanline/window/priority behavior before enabling it generally.
5. Revisit TLCS/Z80 synchronization so the sound CPU runs in larger safe batches
   rather than through frequent small calls, while maintaining interrupts and
   sound-register timing.

Profiling and validation required:

- Build a deterministic differential harness comparing old and new cores at
  instruction/block boundaries, including registers, RAM, timers, interrupts
  and Z80-visible state.
- Maintain a compatibility suite across a meaningful NGP/NGPC ROM set.
- Run long-session tests, repeated save/load, SRAM writes, screenshots and
  cold resume on all three chip families.
- Measure p95 and worst-case tick time. An average of 60 is insufficient if
  frequent long ticks cause audio or display instability.

**Success target:** a demanding rendered tick must fall below approximately
15.5-16 ms at p95, with simulation-only ticks comfortably below that, to claim
a robust 60 Hz result. If hard work reaches 50-55 FPS with correct timing and
without fragile game-specific hacks, that is still a strong and realistic
final outcome for the original ESP32.

## Recommended order of work

1. Preserve the current version as the correctness and performance baseline.
2. Run Stage 1 profiling before making further placement or inline changes.
3. Choose either the TLCS interpreter or dirty-tracked renderer based on total
   measured time; do not optimize both simultaneously.
4. Reassess after the first medium prototype. Stop if gains are below the stage
   gate rather than accumulating risky one-FPS patches.
5. Attempt Stage 3 only if original-ESP32 55-60 FPS is valuable enough to
   justify a core-level project and broad compatibility testing.

The most realistic near-term goal is a stable 45-50 emulated ticks/s in heavy
Sonic gameplay and 50+ in lighter games. Reaching a broadly solid 60 on the
original ESP32 likely requires a materially more efficient TLCS core, not more
Retro-Go adapter tuning.
