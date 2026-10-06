# 60 fps and the speed work (branch `max_fps_experiments`)

Branched from `adhoc` at build 14.  Everything here was measured on a PSP; the
emulator's timings mean nothing for this, and three of the faults below never
showed in PPSSPP at all.

## 60 fps in 1P races: split frames

MK64 simulates two ticks per displayed frame in 1P.  A split frame shows a
picture after each tick: iteration A runs the frame up to tick 1 and renders,
iteration B runs tick 2 and the rest of the frame (kart animation and sprite
loader, object/HUD update) and renders again.  Pads, audio commands and the
mixer stay once per game frame.  The simulation executes the same operations in
the same order, so game speed, physics, ghosts and the ad hoc lockstep are
untouched: a scripted race logs identical kart positions either way, and the
60 fps pictures match the 30 fps renders of the same game frames.

- Only a plain, unpaused 1P race splits (`port_frame_can_split`, main.c).  2P-4P,
  the menus, the results screen and ad hoc sessions keep whole 30 fps frames.
- `end_frame` waits one vblank per picture.  A late picture makes the game run
  slow, so past 15 late pictures in 120 it falls back to whole frames -- and
  returns as soon as half a second of those frames says 60 would fit again (a
  whole frame is one picture plus one tick), 10 s at most.
- The draw-distance governor starts every race at `PORT_DRAW_DIST` (3000), never
  goes below it, and draws further (to 5000) while there is headroom; a far
  triangle that still spans a tenth of the screen is always kept (backdrop).
- Lakitu is drawn camera-relative in a frame's first picture: objects update once
  per game frame and he hangs in front of a camera that moves every tick.
- `port_log` collects lines in RAM while a race runs: a memory-stick write is tens
  of milliseconds.

## Renderer

- Off-screen triangles are rejected outright; the GE guard band is 3.0 while the
  viewport is the whole screen.  Any smaller view (split-screen quadrants, the
  results screen's replays) uses 1.0 and is clipped to its viewport on the CPU --
  the GE scissor alone let a replay draw over its neighbour.
- `gfx_sp_vertex` keeps an outcode word and clip.x/w, clip.y/w per vertex: a
  triangle is one AND (reject), one OR (needs the clipper) and a two-multiply
  winding test.  The clipper runs only the planes a triangle crosses (the two
  depth planes always; a plane the polygon is wholly inside costs one distance
  per vertex and no copies).  This was the largest single cost: 300 of a
  picture's 1000 triangles went through all seven planes.
- Batches are written straight into GE list memory through the cache, in cache
  lines of their own, and written back in one block.
- VFPU: lighting/texgen, clip interpolation, the clipper's winding test,
  fixed-point matrix loads, `mtxf_multiplication` (render-only callers), and the
  per-vertex outcodes.
- The FPS counter keeps one font texture (it used to make a new one every frame
  and exhaust the arena's 512 slots every ~7 s at 60 fps).
- The texture cache key includes the tile's size.

## Audio on the Media Engine (`-DPORT_ME_AUDIO`)

synthesis.c's mixer commands are recorded into a job, as the N64's audio command
list was, and the ME executes it while the main CPU runs the game: all of the
DSP (ADPCM, resampling, envelopes, mixing, reverb).  The sequence player and the
sound-effect logic stay on the main CPU.  `mk64k.prx` (tools/psp/mekprx), a
kernel helper next to the EBOOT, boots the ME; boot, transport and cache
discipline are the 007 Portable port's.  Without the PRX, under an emulator, or
if the ME does not answer its ping, the mixer runs on the main CPU.

## Hardware-only lessons

1. **`mfvc` straight after `vcmp` reads the previous compare's bits on a PSP**
   (PPSSPP returns the new ones).  A `vsync` before the read fixes it.  The boot
   self-test checks both on the machine it runs on and enables the VFPU outcodes
   only if the vsync variant matches the C compares.
2. **Everything the ME writes must sit in whole 64-byte cache lines** of its own
   (the audio heap allocates in 64-byte units; mixer state and the job ring are
   padded).  Otherwise one core's write-back carries stale bytes over the
   other's.  First attempt without this: a crash at the logo.
3. **The GE scissor** as the overlap of the game's scissor and the viewport gave
   garbled textures on the results screen.  Never explained; reverted.
4. **PPSSPP clamps where the GE wraps or drops**: never trust it for clipping,
   depth-range or guard-band behaviour.  Ask for a clean-picture run on hardware
   after any renderer change, before measuring anything.

## Switch files (empty files in `data/`)

| File | Effect |
| --- | --- |
| `fps30` | whole frames only (no 60 fps) |
| `showfps` | start with the FPS counter on (SELECT 3 s toggles it as always) |
| `nome` | mixer on the main CPU |
| `nodirect` | batches through the staging copy instead of direct emit |
| `novfpuoc` | per-vertex outcodes from C compares |
| `logsync` | write every log line through, also in a race |
| `cpu222` | 222 MHz |

## Measuring

- `-DPORT_PROFILE`: per-frame split (logic / display list / GE + vsync / audio),
  and per half of a split frame the time outside the interpreter (tick, kart
  sprites, objects, 3D and HUD display lists, pads + audio commands).
- `-DPORT_EXP` plus an empty `data/exp`: a race rotates eight renderer experiments
  one second at a time and logs each one's busy time every 24 s, with event
  counts per normal picture.  The picture is wrong while it runs.  Rotating by
  the second matters: five-second windows compared different stretches of track.
- Scripted runs: `-DPORT_FINISH_TEST` finishes a race at the first crossing of the
  line (the results screen follows); `data/testcourse.bin` picks the course.

Reference numbers, DK's Jungle Parkway 1P, per picture: about 3.6 ms (A) / 4.0 ms
(B) outside the interpreter, 1 ms decode, 2 ms vertices; 14.8 ms busy on
average, no fallback after the first seconds of the race.
