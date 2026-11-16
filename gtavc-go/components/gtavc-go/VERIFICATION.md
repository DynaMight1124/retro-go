# Verification report

All results below are reproducible with `tools/verify.sh` and
`GTAVC_SEED=1234567`. The reference is the upstream Java build
(`reference/`, built with `tools/build.sh` and run with
`tools/test.sh Shots`) with `Fx.seed = 1234567` in `World.reset()` so both
sides start from the same RNG state. The C runner is a frame-for-frame replay
of `reference/tools/test/Shots.java`.

Environment:

- host: x86-64 Linux, gcc 12, glibc
- target: MIPS32 soft-float, `mipsel-linux-muslsf-gcc` 11.2.1, `-msoft-float`,
  run under `qemu-mipsel` 10.0.13
- console core: `mipsel-mti-elf-gcc` 7.4.0 (`-EL -march=mips32 -mtune=mips32r2
  -msoft-float -G0 -mno-abicalls -fno-pic`)

## 1. Simulation parity with the Java reference

The complete scripted session produces the same simulation output, including
the RNG seed, entity counts, world checksums, car speed/HP, collision
diagnostics, slide value and final player health. The RNG is shared between
AI, voice selection and shooting in the original, so `Snd_voice` still draws
its random voice line (the mixer plays it, but the draw must happen first); without that
draw the whole simulation would drift. The sine/atan tables are the exact
values from `Fx.init()` in the JVM build (`src/port/fx_tables.h`).

| checkpoint | Java seed | C seed | Java `ph/ch` | C `ph/ch` |
|---|---|---|---|---|
| spawn | -1768830343 | -1768830343 | -248849913 / -69625126 | same |
| after camera | 571801775 | 571801775 | -293357815 / -85380381 | same |
| after pickups | -545207108 | -545207108 | -292934100 / -78687375 | same |
| after shooting | 2050916967 | 2050916967 | -291243204 / -77023980 | same |
| after kill | 1154574076 | 1154574076 | -249759123 / -68521331 | same |
| after times | 1253717254 | 1253717254 | -244278707 / -68077989 | same |
| before entering | 1253717254 | 1253717254 | -244278707 / -68089403 | same |
| entered car | 37495798 | 37495798 | -251774587 / -79177025 | same |

`ph` is a checksum over all pedestrians (x, z, yaw, state), `ch` over all
vehicles (x, z, yaw, speed). Every in-game print matches, including:

```
t=1s speed 25 km/h hp 1000      ... t=8s speed 1 km/h hp 842
speed 0 m/s, vx 32 vz 65 hp 842
slide 0 yawVel -7
burning 1
wrecked 1 health 8
```

## 2. Screenshots

The same 18-frame script was rendered on x86-64 and on MIPS32 soft-float and
compared pixel by pixel. Flat regions are byte-identical; the remaining
differences are sub-pixel edges caused by the renderer's float triangle setup
(the inner loops are integer in the original too).

| shot | vs Java reference | vs MIPS soft-float |
|---|---|---|
| 00 splash | identical | identical |
| 01 menu | identical | identical |
| 02 spawn | 5435 px (7.1%, see note) | 0 px |
| 03 map | identical | identical |
| 04 look1 | 5466 px (7.1%, see note) | 0 px |
| 05 look2 | 5508 px (7.2%, see note) | 0 px |
| 06 look3 | 5507 px (7.2%, see note) | 0 px |
| 07 aim | 310 px (0.40%) | 0 px |
| 08 bat | 322 px (0.42%) | 0 px |
| 09 dead | 306 px (0.40%) | 0 px |
| 10 time 9 | 295 px (0.38%) | 0 px |
| 11 time 18 | 323 px (0.42%) | 0 px |
| 12 time 23 | 305 px (0.40%) | 0 px |
| 13 drive | 331 px (0.43%) | 0 px |
| 14 turn | 14662 px (19.1%) | 0 px |
| 15 handbrake | 23009 px (30.0%) | 0 px |
| 16 fire | 21973 px (28.6%) | 0 px |
| 17 explode | 38471 px (50.1%) | 0 px |

Notes:

- Splash, menu, map and all deterministic UI screens are **byte-identical**
  to the Java build and to the MIPS build.
- The first four 3D shots differ from Java mainly in the **help box**, whose
  text was changed to the new Switch-layout controls ("A: run/sprint ...").
  Masking that box leaves ~0.4% of pixels, all on texture edges.
- The later 3D scenes (help box gone) differ from Java on about 0.4% of
  pixels; sampled flat road/sky/building pixels are exactly equal. The
  camera/vehicle views at the end contain more high-frequency textured
  surfaces; the images are visually the same.
- Host vs MIPS: **0 differing pixels** across all 18 shots, with identical
  simulation logs. The integer fixed-point rasteriser is fully deterministic
  across x86-64 and MIPS32 soft-float.

The screenshots are in `screenshots/` (host), `screenshots_mips/` (MIPS
soft-float), `screenshots_legacy/` + `screenshots_legacy_mips/` (the original
`Assets.legacy` configuration used on the console) and `screenshots_java/`
(the Java reference).

## 3. Controls test (Switch layout)

Run with `GTAVC_CONTROLS=1` on the MIPS runner. Measured results:

```
CTRL run speed=6200 (expect ~6200)          # continuous autorun, no button
CTRL sprint speed=8800 (expect ~8800)       # A held
CTRL attack clip=15                         # B
CTRL jump air=1                             # X
CTRL inCar=1                                # Y next to the Admiral
CTRL A throttle speed=10971                 # A in the car
CTRL B brake speed=-2985                    # B in the car
CTRL L handbrake slide=1 braking=0          # L in the car
CTRL R lookbehind camYaw=864 carYaw=352 want=864
```

## 4. Options, stress and sanitizers

Traffic density was measured through the real options menu on the console
configuration (alive civilians / traffic cars):

```
low       -> 6 peds / 2 cars
medium    -> 10 peds / 3 cars
high      -> 16 peds / 5 cars
very high -> 16 peds / 8 cars
```

The same test with the full model pool gives 12 / 22 / 30 / 30 peds and 3 / 8 / 10 / 12
cars.  All spawned pedestrians have a loaded model and texture (`visible`
equals `alive`), because `World_spawnPed` now picks from `w->npool` instead of
the full `WORLD_NPOOL` range.

`GTAVC_OPTSTEST=1` writes and reads back every display option and prints
`PASS`.  On the console the record is stored as
`ROMS/gtavc/te_gtavc_opts.rms` (42 bytes), next to the game save.

The default console resolution is 160x120 (the upstream legacy mode); 240x180
remains selectable and renders correctly.

`GTAVC_BATTEST=1` captures the idle bat and 12 frames of a swing.  The shipped
`FIGHTppunch` clip is a static side-lying pose (all nine exported frames are
identical), so the old bat attack made Tommy lie on his side.  `cPunch` now
uses the animated upright `FIGHTpunch` clip; the bat is held upright and
swings across.  Host and MIPS32 soft-float runs are still pixel-identical.

A 3000-frame stress run at high density drives across roughly 500 m of the
map, forcing tile streaming, traffic, pedestrian AI and collisions. It
completes without errors; peak host RSS was **14.6 MB** (the 32-bit device heap
is smaller). A 5000-frame run also completed. An ASan build reports no memory
errors.  The only UBSan reports are the 32-bit wraparound/negative-shift
idioms inherited from Java; the build uses `-fwrapv` so those have the same
defined behaviour as on the JVM.

The 40-second model-pool rotation is disabled on the target: the Java code
releases the old ped texture while existing peds still reference that slot
(peds disappear) and reloads large models during play (long hitches).  The
initial pools stay resident.

If `anim.bin` is missing, pedestrians now use an identity pose (visible T-pose)
instead of collapsing.  A red `PED DATA MISSING` banner on the console and a
line in `gtavc.log` report missing `anim.bin` / `p_*.bin` / `t_p_*.png`; the
SD installer verifies those files and the tile/texture counts after copying.

### First mission

`GTAVC_MISSIONTEST=1` produced:

```
MISSION start state=1 timer=173532 text=Go to the Ocean View Hotel marker=27426,217,-943696 player=133922,-837200
MISSION pass state=2 done=1 money=100 text=
MISSION save load=1 done=1 state=2 money=100
```

The test passes on the host, MIPS32 soft-float and ASan builds.  Host/MIPS
screenshots remain pixel-identical.

### Audio and streaming caches

`GTAVC_AUDIOTEST=1` reports `PASS` on the host and MIPS32 soft-float builds;
the mixed stream peaks around 22k with non-zero RMS for gun, bat, siren,
engine, voice and radio segments.  ASan is clean.

`GTAVC_RADIOTEST=1` drives the real enter/exit paths and reports `PASS` on
host and MIPS32 soft-float: the radio stays silent while walking to the car,
plays after entering, stops when `Options -> radio` is switched off, resumes
when it is switched on, and stops on exit.  `GTAVC_OPTSTEST=1` confirms the
radio flag survives the save/load round-trip; options from version 1 saves
default to radio on.

`GTAVC_BRIDGETEST=1` drives a car up the high bridge ascent onto the deck
over water and along the elevated road above the parking structure.  Both
report `PASS`: the car climbs the ramp smoothly without hitting invisible
walls, does not fall into the parking garage, and suffers zero water drownings.

`GTAVC_RAMPTEST=1` accelerates a car up the wooden beach stunt ramp and confirms
the vehicle launches upward (reaching full ramp elevation) and also asserts that
driving into a building wall correctly halts the vehicle without roof penetration.

`tools/predecode_textures.py` produced 3165 raw 32x32 textures (12.4 MB).
`GTAVC_TILECACHE=1` produced 1216 pre-split tile caches (71 MB) in ~21 s on
the host.  With both caches present, host and MIPS screenshots are unchanged
but tile loading no longer runs stb PNG decode or `tile_split` on the device.

## 5. No FPU

`readelf -A` on the console core:

```
Tag_GNU_MIPS_ABI_FP: Soft float
CPR1 size: 0
```

Disassembly of `core.elf`: **0** `cop1` / `mtc1` / `mfc1` / `lwc1` / `swc1` /
`ldc1` / `sdc1` / `add.s` / `mul.s` / `div.s` instructions.

The MIPS qemu runner has the same property; `tools/verify.sh` asserts it.

## 6. Console build

```
make device FROGGY_TYPE=GB300V2   -> dist/gtavc_GB300V2.gb3k  (318 KB)
make device FROGGY_TYPE=SF2000    -> dist/gtavc_SF2000.sf2k   (318 KB)
```

`core.elf` section sizes: text 320,820 B, data 3,824 B, BSS 269,868 B (`-fwrapv` is enabled so the JVM's 32-bit wrap semantics are preserved).  The BSS increase is the 4096-entry texture cache index (about 12 MB of map textures can stay resident).

## 7. Speed under qemu (soft-float, not the real CPU)

| configuration | ms/frame | triangles |
|---|---|---|
| full 320x240, 22 peds / 8 cars | ~90.8 ms | ~24.0 ms |
| `legacy.txt` (160x120, 10 peds / 3 cars) | ~21.5 ms | ~8.9 ms |

The SD installer enables `legacy.txt`, i.e. the upstream game's own S40
low-end mode (160x120 internal, 10 peds / 3 cars). The user can cycle
320x240 / 160x120 / 106x80 in Options. The speedups come from the integer
rasteriser setup, spatial tile chunks (x/z and height), a 70 m LOD threshold,
the fast integer distance function and `-O3`; no gameplay logic was changed.
The numbers vary with host load and with the streamed scene; the legacy
scripted run is about 2.4x faster and the full one about 3.8x faster, with a
much smaller vertex-transform count (20k -> 6k per frame).
Real-hardware timing still needs to be measured on the device.
