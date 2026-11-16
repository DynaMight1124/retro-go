# Port conventions (gtavcj2me -> C)

Source of truth: `reference/src/*.java`, a verbatim copy of
[aidencantcode/gtavcj2me](https://github.com/aidencantcode/gtavcj2me).
`src/port/` is a direct translation. Control flow, field names, formulas,
constants and comments are kept; only the platform API is adapted.

## Names

- `Fx.foo()` -> `Fx_foo()`; `Fx.SIN` -> `Fx_SIN`; `Fx.seed` -> `Fx_seed`.
- `R3D.foo()` -> `R3D_foo(R3D* r, ...)`.
- `World.foo()` -> `World_foo(World* w, ...)`.
- `Mesh.foo()` -> `Mesh_foo(Mesh* m, ...)`.
- `Ped.foo()` -> `Ped_foo(Ped* p, ...)`; `Ped.WALK` -> `PED_WALK`.
- `Car.foo()` -> `Car_foo(Car* c, ...)`; `Car.PARKED` -> `CAR_PARKED`.
- `CarModel.foo()` -> `CarModel_foo()`; `CarModel.P_BONNET` -> `CAR_MODEL_P_BONNET`.
- `PedModel.foo()` -> `PedModel_foo()`.
- `Assets.foo()` -> `Assets_foo()`; `Assets.SLOTS` -> `Assets_SLOTS`.
- `AnimData.foo()` -> `AnimData_foo()`.
- `Ui.foo()` -> `Ui_foo()`; `Ui.PINK` -> `UI_PINK`.
- `Sky.foo()` -> `Sky_foo()`.
- `Snd.foo()` -> `Snd_foo()`; `Snd.PUNCH` -> `SND_PUNCH`.
- `Save.foo()` -> `Save_foo()`.
- `Tile.foo()` -> `Tile_foo()`; `Tile.NH` -> `TILE_NH`.
- `Game.foo()` -> `Game_foo(Game* g, ...)`; `Game.S_PLAY` -> `GAME_S_PLAY`,
  `Game.A_FIRE` -> `GAME_A_FIRE`.
- Java `Mesh.F_DOUBLE` -> `MESH_F_DOUBLE`, `Mesh.BODY` -> `MESH_BODY`.
- Java `World.K_UP` -> `WORLD_K_UP`; `World.S_ROAD` -> `WORLD_S_ROAD`.

## Types

- `byte` -> `int8_t`/`uint8_t` where the Java code masks with `& 255`.
- `short` -> `int16_t`, `int` -> `int`, `long` -> `int64_t` when a product
  needs 64 bits, `boolean` -> `bool`, `float` -> `float`, `String` -> `char*`.
- Java arrays are C arrays/pointers with explicit lengths; `a.length` becomes
  the corresponding count field or constant.
- `new Ped()` / `new Car()` -> `Ped_init` / `Car_init` (field defaults only).
  When the original reuses a dead ped it does **not** reset every field, and
  the port follows that: `Ped_init` is called only for a brand-new slot.
- `null` -> `NULL`.

## MIDP API mapping

- `Image.createImage(path)` -> `Image_createImageRes(path)`
- `Image.getRGB` -> `Image_getRGB`
- `Image.getWidth/getHeight` -> `Image_getWidth/Image_getHeight`
- `DataInputStream` -> `dis_init`, `dis_readByte`, `dis_readUnsignedByte`,
  `dis_readShort`, `dis_readUnsignedShort`, `dis_readInt`, `dis_readFully`
- `DataOutputStream` -> `dos_init`, `dos_writeInt`, `dos_writeBoolean`, ...
- `RecordStore` -> `RecordStore_*` (persisted under the data directory)
- `System.currentTimeMillis` -> `System_currentTimeMillis`
- `System.arraycopy` -> `memcpy` (typed C arrays)
- `System.gc` -> no-op
- `Thread` (tile loader, sound worker) -> synchronous loading on the target:
  the multicore frontend is single-threaded. `World_streamTiles` loads at most
  the requested ring in place of the Java background thread.
- `Math.min/max/abs` -> `Fx`/plain C comparisons.
- `Math.sqrt(double)` -> `Fx_isqrt` (exact floor of the integer square root,
  identical to the Java cast).
- `Math.sin/cos` (only used to build `Fx.SIN`) -> the tables in
  `src/port/fx_tables.h`, dumped from the JVM implementation.

## Rendering

`R3D` is kept as in Java: integer inner loops, float triangle setup, fixed-point
projection. The target compiles it with `-msoft-float`; no FPU instruction is
emitted. No renderer feature was added or removed.

## Performance changes (platform adaptation, gameplay unchanged)

The original Java renderer is integer in its inner loops but uses `float` for
triangle setup. On the FPU-less target those `float` operations are libgcc
soft-float calls, so `rasterFlat/rasterTex/rasterTexTint` now compute the same
slopes in fixed point:

- one Q40 reciprocal of the triangle area per triangle;
- Q16 slopes for depth/UV, integer scanline stepping;
- exact 64-bit division fallback for triangles smaller than 64 units of area;
- integer backface test for clipped triangles.

Other target-side changes:

- `tile.c` splits every tile mesh into 8x8x4 spatial chunks after loading.
  `R3D_drawMesh` then rejects whole chunks outside the frustum, instead of
  transforming all ~13k vertices of a dense tile every frame.
- `World_render` uses the low-detail tile mesh beyond 70 m (legacy) instead of
  the original 200 m high-detail threshold.
- Projection uses one 32-bit division per vertex (the exact
  `(X*f)/Z`, `(Y*f)/Z`, `(1<<24)/Z` form); the 64-bit alternatives were slower
  on this CPU.
- `Fx_isqrtFast` is exact below 2^32 and within a few millimetres above it.
  `Fx_dist` therefore keeps the same thresholds in practice.
- HUD clock/money/health/ammo are formatted without `snprintf`.
- `Assets` has 4096 texture slots and the renderer has 4096 texture ids, so
  map textures stay resident (about 12 MB worst case). The user approved the
  extra RAM use.
- The libretro core passes real elapsed milliseconds (capped at 100 ms) to
  `Game_frame`, so the game speed stays correct when the frame rate drops.
- The console build uses `-O3` and `-fwrapv` (the Java source relies on 32-bit
  wraparound).

## Stability changes

- `World_pools_rotatePools` is disabled.  The Java version swaps a ped and a
  car model every 40 s and releases the old ped texture while existing peds
  still point at that slot; on the target they vanished and the model reload
  caused long hitches.
- `World_streamTiles` loads at most one tile per frame, and `World_preload`
  loads only the centre tile plus four cardinal neighbours.  The original
  used a background thread for the whole ring.
- `tile_split` allocates one vertex-map and one used-list per tile and reuses
  them for all chunks; `Mesh_new` puts all mesh arrays in one allocation, and
  `Mesh_new`, `PedModel_load`, `AnimData_load` and `Tile_load` check every
  allocation and fail cleanly.
- On the host, `tools/predecode_textures.py` writes raw 32x32 ARGB texture
  caches and `GTAVC_TILECACHE=1` writes pre-split tile chunks (format `GTC1`).
  `Tile_load` prefers `/tc/<i>_<j>.bin`; `Assets_decode` prefers
  `/x/<id>.raw`.  This removes PNG decode and mesh splitting from the device
  streaming path; both fall back to the original assets when absent.
- The libretro core resolves the asset directory robustly (stub in `ROMS/`,
  core path in `system/Deimos/cores`, relative `ROMS/gtavc`), logs the load
  steps, and uses a 16 ms minimum frame delta so the loading screen cannot
  stall on a zero firmware tick.
- `AnimData_sample` returns an identity pose if the clip/frame data is
  missing, so pedestrians stay visible.
- Ped meshes have a flat fallback colour and `World_drawPed` falls back to the
  other LOD model if one is missing.
- `Options` has a traffic-density entry (low/medium/high/very high) and a 240x180
  resolution mode between 320x240 and 160x120; the upscaler now uses source/
  destination ratios, not an integer scale.
- Display options are persisted in `te_gtavc_opts.rms` in the platform data
  directory and loaded in `Game_new`, so resolution/density/HUD settings
  survive a reboot.  The options menu saves on every change, on exit, and
  `Game_stop` saves once more.
- The shipped `FIGHTppunch` clip is a static side-lying pose (nine identical
  frames), which made Tommy horizontal during the bat attack.  `World_initClips`
  maps `cPunch` to the animated upright `FIGHTpunch` clip instead.  This also
  fixes fist combo 2, which used the same clip.
- `World_drawHeld` uses the bat mesh in its original grip orientation.  `w_bat.bin` is exported with its long axis along +x, the same axis as
  the forearm; this makes the barrel stand vertical instead of lying sideways.
  The pistol is untouched.
- `World_spawnPed` picks a slot from `w->npool` (not `WORLD_NPOOL`) and
  `World_assignKind` falls back to a loaded model, so every pedestrian in the
  legacy pool is visible and the density caps are real: 6/10/16/16 peds and
  2/3/5/8 traffic cars on legacy; 12/22/30/30 peds and 3/8/10/12 cars with
  the full assets.
  Increasing density tops the population up immediately via `fillPopulation`
  instead of waiting for the next ambient-spawn tick.

## First mission

`src/port/world_mission.c` adds the first story mission, **An Old Friend**:

- `World_missionReset` places the office marker at the spawn and starts the
  mission after 1.5 s when the player is still there.
- `pickDestination` scans preloaded tiles for a street-level point 55-110 m
  away (no RNG draw; host/MIPS parity is preserved).
- `World_missionUpdate` handles the three-minute timer, arrival, fail on
  wasted/busted, and the $100 reward.
- `World_drawMissionMarker` draws the rotating additive diamond; `Ui_radarDraw`
  adds the yellow blip; `overlay` in `game.c` draws the objective and timer.
- `missionDone` is appended to the save (version 2).  Older version-1 saves
  still load with the mission available.

`GTAVC_MISSIONTEST=1` exercises start/pass/save/reload.

## Target-specific, non-gameplay adaptations

- `src/platform/` implements the CLDC/MIDP subset used by the game
  (`Image`, `Graphics`, `DataInputStream`, `RecordStore`, `System`, `Thread`).
- `Intro` uses a video stub because the multicore core has no MMAPI video
  player; the original already treats a missing `intro.3gp` as "skip".
- `Snd` keeps the original state machine and file table and adds a small
  integer WAV mixer plus an in-car radio player (15 optional two-minute
  clips in `assets/radio`).  Pressing enter near a car loads a random clip
  during the walk-to-door animation; entering starts it at a random offset,
  the next clip starts when it ends, and exiting stops it.  The radio option
  is saved with the other settings.  Only one clip is decoded at a time.
  Effects/voices are resampled to 44100 Hz and mixed into the libretro audio
  batch; engine/siren loops use the original hysteresis.  No FPU is used.
- `World_hCell` resolves the two-layer heightfield: entities on elevated roads,
  bridges and roofs stay on the upper surface `a` without falling through, while
  entities under tree canopies, palms, footbridges, archways or inside underground
  parking stay on the lower floor `b` without jumping to the canopy, roof or sky.
  Above water, seabed heights are ignored.
- `World_isBridgeAt` recognizes elevated decks and ramps over both land and
  water (`top > low + 80`).  `World_moveCar` treats ramps and bridge decks as
  traversable, distinguishing real walls (diff > 280) from slopes (diff <= 280
  with forward continuation).  `World_solidCell` ignores 3D bridge and ramp
  side-rail artifacts so cars can climb ramps and cross bridges smoothly.
- `legacy.txt` (created by the SD installer) enables the upstream
  `Assets.legacy` configuration: half-resolution 3D, smaller model pools and
  a 1-tile streaming ring. This is original game code, not a new mode.
- RNG: the original seeds `Fx.seed` from `System.currentTimeMillis()`. The port
  additionally honours `GTAVC_SEED` for reproducible verification runs.

## Verification

```bash
make host && ./build/gtavc_runner assets screenshots
make mips-test
```

`GTAVC_SEED=1234567` makes the simulation reproducible. The runner mirrors
`reference/tools/test/Shots.java` frame for frame. See `VERIFICATION.md`.
