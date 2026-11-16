# GTA Vice City - J2ME C port for GB300 / SF2000

A **1:1 C translation** of [aidencantcode/gtavcj2me](https://github.com/aidencantcode/gtavcj2me),
the MIDP 2.0 / CLDC 1.1 demake of GTA Vice City. The original Java sources are
kept verbatim in `reference/` and `src/port/` is a direct translation of them;
nothing in the game was redesigned or reimplemented. `src/platform/` implements
the MIDP/CLDC APIs the game uses, and `libretro/` adapts input/video to the
froggy multicore frontend used by the SF2000 and GB300.

| Java (reference) | C port |
|---|---|
| `AnimData.java` | `src/port/anim_data.c` |
| `Assets.java` | `src/port/assets.c` |
| `Car.java` | `src/port/car.c` |
| `CarModel.java` | `src/port/car_model.c` |
| `Fx.java` | `src/port/fx.c` (+ `fx_tables.h`, `fx_math.c`) |
| `Game.java` | `src/port/game.c` |
| `GtaMidlet.java` | `libretro/gtavc_libretro.c` (entry point) |
| `Intro.java` | `src/port/intro.c` (MMAPI video stub) |
| `Mesh.java` | `src/port/mesh.c` |
| `Ped.java` | `src/port/ped.c` |
| `PedModel.java` | `src/port/ped_model.c` |
| `R3D.java` | `src/port/r3d.c` |
| `Save.java` | `src/port/save.c` |
| `Sky.java` | `src/port/sky.c` |
| `Snd.java` | `src/port/snd.c` (PCM mixer, voices, engine/siren loops) |
| `Tile.java` | `src/port/tile.c` |
| `Ui.java` | `src/port/ui.c` |
| `World.java` | `src/port/world.c`, `world_ai.c`, `world_combat.c`, `world_entities.c`, `world_gameplay.c`, `world_pools.c`, `world_render.c` |

## No FPU

The SF2000 / GB300 MIPS32 core has **no floating-point unit**. Everything is
built with `-msoft-float`; any `float`/`double` operation in the original
renderer setup goes through libgcc integer helpers. The final core was checked:

```
Tag_GNU_MIPS_ABI_FP: Soft float
CPR1 size: 0
FPU instructions in core.elf: 0
```

`Fx` is a fixed-point library in the original (2.14 matrices, 0..1023 angles).
Its sine/atan tables were dumped from the JVM implementation into
`src/port/fx_tables.h`, so the port uses exactly the same values as the Java
build without calling `Math.sin` at runtime. Square roots that the original
computes as `(int)Math.sqrt(double)` are done with an exact 64-bit integer
square root (`Fx_isqrt`), which returns the identical integer. This removes
libm differences between host and target and is faster on a CPU without FPU.

## Build

Host (for the runner and screenshots):

```bash
make host
./build/gtavc_runner assets screenshots
```

MIPS32 soft-float under qemu (same runner, no FPU):

```bash
make mips-test        # tools/test_mips.sh
```

Console core:

```bash
make device FROGGY_TYPE=GB300V2   # -> dist/gtavc_GB300V2.gb3k
make device FROGGY_TYPE=SF2000    # -> dist/gtavc_SF2000.sf2k
```

Install on the SD card:

```bash
./install_to_sd.sh /media/<user>/GB300 GB300V2
```

The installer copies the core into every location the loader looks at, copies
the game data (about 88 MB) next to the ROM stub in `ROMS/gtavc/`, and creates
the upstream `legacy.txt` marker. That marker enables the original game's own
low-end S40 configuration: 160x120 internal rendering upscaled to 320x240,
5 pedestrian models, 4 car models, 10 peds, 3 traffic cars and a 1-tile
streaming ring. It is not a new mode - it is the `Assets.legacy` path that
already exists in the Java source.

## Controls (Switch layout)

Tommy always runs (continuous autorun); holding A sprints.

| Button | On foot | In a vehicle |
|---|---|---|
| D-pad | move / turn | steer, accelerate / brake |
| A | sprint (hold) | accelerate |
| B | attack / shoot (hold to aim) | brake + reverse |
| X | jump | handbrake |
| Y | enter / exit vehicle | exit vehicle |
| L | camera left | handbrake |
| R | camera right | look behind |
| Select | switch weapon | switch weapon |
| Start | pause | pause |

## Performance

The port targets the 87 MHz MIPS core without an FPU.  The main changes for
speed (all in `src/port/`, no gameplay rewrites):

- the triangle setup of `R3D` is integer fixed-point instead of float, so no
  soft-float calls remain in the rasteriser;
- tile geometry is split into small spatial/height chunks at load time, so
  off-screen parts of a 100 m tile are not transformed every frame;
- the low-detail tile mesh is used away from the player;
- `Fx_dist` uses a fast integer square root, HUD numbers avoid `snprintf`,
  and the texture cache is 4096 slots (about 12 MB if the whole map is seen),
  so revisiting an area does not decode PNGs again;
- the console core uses `-O3`, real elapsed time for `dt`, and the original
  `legacy.txt` configuration (160x120 internal, 10 peds / 3 cars).

Measured with the MIPS32 soft-float runner under qemu (not the real CPU):

| configuration | before | after |
|---|---|---|
| legacy 160x120 | ~21.5 ms/frame | ~5.7 ms/frame |
| full 320x240 | ~90.8 ms/frame | ~24 ms/frame |

`Options -> screen resolution` cycles **320x240 / 240x180 / 160x120 / 106x80**
(default on the console: 160x120, the upstream legacy mode).  The new 240x180
mode sits between the two original resolutions.  `Options -> traffic density`
cycles **low / medium / high / very high** populations independently of the model pools.
On the legacy configuration that is **6 / 10 / 16 / 16 pedestrians and 2 / 3 / 5 / 8
cars**; with the full model pool it is **12 / 22 / 30 / 30 and 3 / 8 / 10 / 12**.  The
spawner now only picks ped slots that actually have a loaded model, so a
density change is reflected immediately and fewer pedestrians vanish.

All display options (brightness, draw distance, limiter, filter, subtitles,
wide screen, legend, radar, HUD, density, resolution, radio) are stored in
`te_gtavc_opts.rms` next to the game and restored on the next boot.  They are
written when changed, when leaving the options screen, and on exit.

The shipped `FIGHTppunch` animation in `anim.bin` is a **static side-lying
pose** (all nine exported frames are identical), so the old bat attack made
Tommy rotate onto his side.  `cPunch` now uses the animated, upright
`FIGHTpunch` clip; the bat mesh is also kept in its original grip orientation
(its +x axis is the forearm axis) so the barrel points forward from the hand
instead of backwards.  The pistol attachment is unchanged.

## Multi-layer elevation & bridge traversal

- **Two-layer ground stability & overhead canopy handling**: `World_hCell`
  resolves two-layer terrain: if an entity is on the upper deck (driving on an
  elevated highway, overpass, bridge, or parking roof), it stays on the upper
  surface `a` even across upward slopes, never falling through into lower floors.
  If an obstacle is overhead (`a > y + 70`), the entity is underneath (under a
  tree canopy, palm tree, archway, footbridge, or inside underground parking)
  and reliably stays on the lower floor `b`. Entities under palm trees and
  leaf canopies never teleport or spawn onto the treetop. Above water (`y > -70`),
  seabed heights (`b < -70`) are never selected as walking floors, preventing
  water drownings on bridges and piers.
- **Ramp traversal & stunt jumps**: `World_moveCar` distinguishes real walls
  (buildings with roof heights > 4.7 m) from driveable ramps (height rise
  <= 4.3 m where the slope continues upward, or bridge/road decks). Cars can
  freely climb stunt ramps, beach ramps, parking ramps, and bridge approaches
  without crashing into invisible 1.1-meter height barriers.
- **Ramp side artifact clearing**: `World_solidCell` ignores steep 3D geometry
  artifacts (bridge trusses, ramp side flanks, railings) on ramps and decks,
  so cars do not catch side-rail collision boxes when driving onto ramps.
- `GTAVC_RAMPTEST=1` verifies climbing stunt ramps and blocking at real building walls.
- `GTAVC_BRIDGETEST=1` verifies driving up the high suspension bridge ramp
  onto the deck and traversing the elevated road above the parking garage.

## Audio

The core now decodes the original WAV effects and voice lines and mixes them
into the libretro stereo batch at 44100 Hz.  The hardware is mono, so the
frontend averages the two channels.  Effects are 11025 Hz 16-bit; voices are
8000 Hz 8-bit.  Engine idle/rev and the police siren are looping voices with
the original 0.7 s engine hysteresis.  `Snd_mix` is integer-only (no FPU) and
mixes up to 12 simultaneous voices.

`GTAVC_AUDIOTEST=1` loads the clips, mixes gun/bat/siren/engine/voice/radio,
and writes the mixed stream to `/tmp/gtavc_audio.raw` for inspection.

### Radio

Fifteen two-minute Flash FM clips can be placed in `assets/radio/01.wav` …
`15.wav` (8 kHz mono).  Pressing enter next to a car prepares a random clip
while Tommy walks to it, so the SD read is hidden by the get-in animation.
Sitting in the car starts that clip at a random offset; when it ends another
random clip starts, and exiting stops the radio.  `Options -> radio` turns the
in-car radio off and the setting is saved with the other options.  Only one
clip is decoded at a time to keep the device heap free.  The Flash FM clips
are copyrighted audio and are **not** part of the public repository or
release; the code loads them from `assets/radio` when present.

## First mission: An Old Friend

A small mission layer now runs on top of the original world.  New games start
at Ken's office; after a short delay **An Old Friend** starts automatically.
Ken asks Tommy to drive to the Ocean View Hotel.  A rotating yellow diamond
marks the destination, the radar shows a yellow blip, and the objective/timer
are drawn at the top-left.  Reaching the marker within three minutes pays
**$100** and marks the mission complete; the state is stored in the save
(record version 2), so it stays completed after a reboot.  If the timer runs
out or Tommy is wasted/busted, the mission fails and the office marker becomes
available again.

`GTAVC_MISSIONTEST=1` starts the mission, teleports Tommy to the destination,
checks the pass/reward and verifies the save round trip.

## Fast tile streaming

A dense 100 m tile contains up to 39k near vertices and 91 texture PNGs.
Decoding those PNGs and splitting the mesh synchronously was the source of the
multi-second hitches while driving.  Two host-generated caches remove that
work from the device:

- `/x/<id>.raw` - every 32x32 map texture pre-decoded to raw ARGB
  (12.4 MB total).  `Assets_decode` reads these directly instead of invoking
  stb PNG decode; if a `.raw` is missing it falls back to the PNG.
- `/tc/<i>_<j>.bin` - the near/far meshes already split into spatial chunks
  (71 MB for 1216 tiles, versioned format).  `Tile_load` reads the cache and
  only falls back to the original `t/*.bin` + `tile_split` if it is absent.

Both caches are generated on the host before installation:

```bash
python3 tools/predecode_textures.py
GTAVC_TILECACHE=1 ./build/gtavc_runner assets /tmp/tile_shots
```

`install_to_sd.sh` copies `assets/x/*.raw` and `assets/tc/` with the rest of
the assets.  If they are not present, the game still works, just with the old
streaming cost.

## Stability on the console

Several things that are harmless on a phone are not on a 32 MB device, so the
console build also does the following:

- the startup preload is only the centre tile plus its four neighbours, and
  **one more tile is loaded per frame** once gameplay starts (the Java build
  used a loader thread for the whole ring);
- tile meshes are split at load time with a **single reusable vertex map**
  (not one 54 KB map per chunk) and **one allocation per mesh** instead of
  twelve, and every allocation is checked;
- the model-pool **rotation every 40 s is disabled**: it released textures
  still used by existing pedestrians (they disappeared) and reloaded large
  models during play (hitches).  The initial pools stay resident;
- if `anim.bin` is missing or corrupt, pedestrians get a **neutral pose**
  instead of collapsing to invisible degenerate triangles;
- if the player/ped texture is unavailable, they are drawn with a flat
  fallback colour instead of disappearing;
- a red **`PED DATA MISSING`** banner and a `gtavc.log` line report missing
  `anim.bin` / `p_*.bin` / `t_p_*.png`;
- `install_to_sd.sh` now verifies the critical files and the tile/texture
  counts after copying, so a partial card copy cannot silently ship;
- the loader can hand us the stub in `ROMS/` instead of `ROMS/gtavc/`; the
  core resolves the asset directory by looking for `mapidx.bin` next to the
  file, one level up, in `ROMS/gtavc`, and at the SD root;
- the frame delta has a 16 ms floor, so the loading screen advances even if
  the firmware tick is still zero at boot;
- `gtavc.log` (next to the game) now records the resolved asset directory,
  `mapidx`/`p_player`/`anim` presence and the loading step, which identifies
  a stuck load immediately.

The texture cache is 4096 slots (about 12 MB when the whole map has been seen),
as requested; the 32-bit heap still measured about 15 MB RSS in a 3000-frame
stress test.

## Verification

`make run` replays the exact scripted session of the upstream Java harness
(`reference/tools/test/Shots.java`): menu, spawn, map, camera, pickups, pistol,
bat, a dead pedestrian, three times of day, driving, handbrake, fire and
explosion. With `GTAVC_SEED=1234567` the port reproduces the Java run's RNG
seed, world checksums, speeds, HP, slide value and final health exactly.
Splash, menu and map are pixel-identical; on-foot 3D scenes differ on about
0.4% of pixels (float rasteriser edges). Host and MIPS soft-float differ on
0.02% of pixels with identical simulation logs. See `VERIFICATION.md` and
`screenshots/compare_java.png` / `screenshots/compare_mips.png`.

## Assets / licence

The game code is a translation of the upstream MIT-licensed project. GTA Vice
City assets are property of Rockstar Games / Take-Two and are not licensed by
this repository; the `assets/` directory is the resource set published in the
upstream repository for running the demake.
