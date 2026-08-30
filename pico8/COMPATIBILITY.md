# PICO-8 Compatibility and Performance Matrix

This document tracks cartridge compatibility and performance across Retro-Go
targets. A game being marked compatible means its tested portion behaved like
the desktop PICO-8 player; it does not imply that every level or code path has
been completed.

## Measurement protocol

Use a release build and record the firmware SHA-256, target, clock settings,
and test scene. Start each game from a cold launch, play for at least 60
seconds, and include a representative heavy scene where possible.

- **FPS/cap**: observed FPS followed by the cartridge's 30 or 60 FPS cap.
- **Busy**: typical gameplay BUSY value, followed by the observed peak when
  useful. Prefer a range over a misleading single sample.
- **FS**: highest automatic frameskip level observed.
- **-**: not measured under this protocol yet.
- Do not average title screens, loading, death screens, or paused gameplay
  into the gameplay Busy figure. Note them separately if they are important.
- Keep log.txt with the test until its result has been copied here, because
  the next serial capture overwrites it.

Suggested result format:

Game | firmware hash | scene/duration | FPS/cap | Busy typical (peak) | FS

### Performance telemetry

Runtime profiling instrumentation and its Options-menu toggle were removed
after the 2026-08-30 investigation so release builds carry no profiler branches,
scope objects, counters, or timer calls in hot graphics APIs. Retro-Go's normal
FPS, Busy, frameskip and system telemetry remain available in the serial log.
The recorded detailed findings below and the offline log-analysis scripts are
retained for future comparisons without adding emulator runtime overhead.

### Performance investigation findings (2026-08-30)

- Retro-Go presentation is not a general bottleneck: ordinary framebuffer
  expansion/submission is approximately 0.28 ms per rendered frame.
- Initial attribution showed that Buns Bunny, Brutal Pico Race, and The Tower
  of Archeos spend most of their time in cart Lua rather than the measured
  native graphics APIs. Temporary opcode profiling identified table reads as
  the only strong shared VM target. Direct short-string/integer raw-table
  dispatch reduced representative heavy update time modestly without observed
  regressions; the temporary per-instruction profiler was then removed.
- Default map requests are clipped to the visible tile rectangle before the
  map loop. Ordinary unscaled map tiles also use a guarded packed-pixel path;
  relocated sprite memory, alternate draw targets, and sprite fill patterns
  retain the full renderer.
- On CROKPOCKET, packed map tiles reduced X-Zero's measured `map()` cost from
  roughly 7-8 ms to 4-5 ms per rendered frame (about 35-40%) and total draw
  time from roughly 10-11 ms to 7-9 ms. Celeste map cost improved by roughly
  15-25% in comparable gameplay. Celeste, X-Zero, Moonrace, and Whiplash Taxi
  completed focused graphical regression tests successfully.
- Remaining evidence: X-Zero is primarily limited by roughly 10-13 ms update
  callbacks plus its remaining map work; Moonrace is overwhelmingly Lua-update
  bound; Whiplash Taxi's native draw cost is overwhelmingly `tline()`. Further
  changes should target measured call sites rather than general framebuffer or
  display code.
- Temporary native-entry counters then separated the three Lua-heavy stress
  carts: Buns Bunny constructs roughly 1,300-1,800 `all()` iterators per heavy
  second; Brutal Pico Race repeatedly uses single-byte `peek()` for procedural
  sprite decoding and `color()` for polygon fills; Tower of Archeos repeatedly
  changes `camera()` while drawing outlined objects. Guarded exact-function
  fast paths for those calls passed four-game regression testing, but produced
  no reliable timing or subjective improvement beyond normal scene variance.
  They were removed again to keep every `OP_CALL` dispatch lean; the temporary
  counters were also removed.
- Build-command inspection found that the complete Pico-8 engine, including
  the Lua VM and software rasterizer, inherited the size-oriented global `-Os`
  setting. The current hardware-validation build applies `-O2` only to the
  Pico-8 component, matching the supported per-app Retro-Go mechanism while
  leaving shared components untouched. The CROKPOCKET app grew from `0x8bdb0`
  to `0x96970` bytes (+`0xabc0`, about 7.7%). Four-game hardware validation was
  regression-free; Tower of Archeos improved from the recorded 18/30 FPS at
  100% Busy to 22/30 at 98%, while Celeste retained 30/30 FPS and reduced its
  median Busy from 22% to 17%. Buns Bunny was broadly unchanged and Brutal Pico
  Race remained draw-bound. `-O2` is retained, subject to later instruction-
  cache validation on an original ESP32.
- A follow-up exact-integer `OP_SETTABLE` specialization targeted Brutal Pico
  Race's Lua scanline-node writes. Hardware A/B testing found no subjective
  improvement: Brutal's median draw callback changed from about 42.1 ms to
  46.0 ms, Celeste varied by roughly 3%, and Tower's small improvement remained
  within scene variance. The specialization was removed; the proven raw-read
  optimization and scoped `-O2` build remain.

## ESP32-S3 results

The current development target is the CROKPOCKET ESP32-S3. Compatibility
results come from physical-device testing. Performance cells that remain
blank were not present in the standardized 2026-08-27 capture.

Busy values are median with the 10th-90th percentile range in parentheses.
FPS is the median against the cart cap; significant observed dips are called
out in the notes. This batch used firmware 7EF4C7D3F7C9... and contained
17-74 non-zero one-second samples per game.

| Game | FPS/cap | Busy | FS | Compatibility / outstanding issues | Priority |
|---|---:|---:|---:|---|---|
| 8 Legs to Love | 30/30 | 24% (19-30) | 0 | No known issues; one 47% peak | Pass |
| A Hat on Time | 30/30 | 12% (10-15) | 1 | Stable after startup; initial 10 FPS/92% transition | Pass |
| Across the River | 30/30 | 40% (37-43) | 0 | Static-frame and disappearing-object faults fixed | Baseline |
| Ad Astra | 30/30 | 57% (55-70) | 0 | Matches desktop demo output | Pass |
| Air Delivery | 60/60 | 68% (65-73) | 0 | No known issues | Pass |
| Alfonzo's Bowling Challenge | 60/60 | 70% (67-79) | 0 | 30-second test completed without logged errors | Pass |
| Alone in the Pico | 30/30 | 89% (64-98) | 4 | Triangle rendering correct, but observed 6 FPS minimum | Performance |
| Alpine Alpaca | 30/30 | 68% (48-77) | 0 | 30-second test completed without logged errors | Pass |
| Alpine Ascent | 30/30 | 15% (14-15) | 0 | 30-second test completed without logged errors | Pass |
| Ascent | 30/30 | 92% (68-100) | 4 | Correct but overloaded in parts; observed 25 FPS minimum | Performance |
| BAS | 60/60 | 55% (37-78) | 0 | 30-second test completed without logged errors | Pass |
| Beckon | 30/30 | 21% (19-23) | 0 | Title, rocks and gameplay fixed | Pass |
| Birds with Guns | 60/60 | 86% (81-93) | 0 | Correct, but limited S3 CPU headroom | Watch |
| Bondstones | 30/30 | 56% (49-64) | 0 | Some music notes remain harsher than desktop; 24 FPS minimum | Audio |
| Breakout Hero | 60/60 | 52% (34-74) | 0 | No known issues; one 87% peak | Pass |
| Brutal Pico Race | 21/30 | 98% (95-98) | 5 | Heavy overload remains under scoped `-O2`; observed 16 FPS minimum and roughly 42 ms average draw callback | Performance |
| Bubble Bobble | 60/60 | 22% (20-31) | 1 | Stable after a brief 49 FPS/98% spike | Pass |
| Build a Jetpack | 60/60 | 55% (40-72) | 1 | Plays correctly. Cart explicitly calls `printh()` every frame, producing numeric developer-console output and likely inflating the 93% Busy peak; 46 FPS minimum | Pass |
| Buns Bunny | 59/60 | 92% (48-98) | 5 | Broadly unchanged under scoped `-O2`; enemy-heavy slowdown remains and 33 FPS minimum was observed | Performance |
| Cab Ride | 30/30 | 80% (75-92) | 0 | Correct, but limited S3 CPU headroom | Watch |
| Cattle Crisis | 60/60 | 87% (61-97) | 5 | Heavy scenes remain unstable; observed 43 FPS minimum | Performance |
| Celeste Classic | 30/30 | 17% (15-24) | 0 | No known issues; scoped `-O2` retest remained correct and reduced median Busy versus the prior 22% capture | Baseline |
| Celeste 2 | 30/30 | 39% (31-50) | 0 | Gameplay, graphics and background music working; 73% peak | Baseline |
| Cherry Bomb | 30/30 | 19% (9-27) | 0 | No known issues | Pass |
| Combo Pool | 30/30 | 64% (52-69) | 0 | No known issues | Pass |
| Crowded Dungeon | 30/30 | 87% (49-97) | 2 | Correct but reached 100% Busy and 26 FPS | Performance |
| Curse of the Lich King | 30/30 | 44% (33-56) | 0 | Text fixed; brief 19 FPS/80% transition | Pass |
| Dank Tomb | 58/60 | 92% (70-98) | 3 | Graphics confirmed correct with legacy draw-palette bit-7 transparency support. Performance dips to 24 FPS | Performance |
| Delunky | 30/30 | 46% (45-51) | 0 | No known issues | Pass |
| Demon Castle | 60/60 | 68% (47-84) | 0 | No logged errors; brief 9 FPS startup/transition sample | Pass |
| Dinky Kong | 60/60 | 90% (49-98) | 5 | Graphics/gameplay fixed; observed 54 FPS minimum and 100% peak | Performance |
| Dodge | 60/60 | 25% (12-57) | 0 | Plays correctly; heavier scene peaked at 61% | Pass |
| Dominion Ex | 30/30 | 20% (14-28) | 0 | No compatibility issues reported | Pass |
| Downward | 30/30 | 35% (31-41) | 0 | No compatibility issues reported | Pass |
| Driftmania | 60/60 | 92% (68-99) | 3 | Correct graphics/collision/audio; observed 32 FPS minimum | Performance |
| Dungeon | 30/30 | 7% (7-7) | 0 | Very light CPU load; minor background-note difference remains | Audio |
| Dusk Child | 30/30 | 25% (16-90) | 3 | Repeated heavy transitions fall to 6-17 FPS before recovering | Performance |
| Ennuigi | 30/30 | 5% (4-8) | 0 | No compatibility issues reported; one startup/transition peak | Pass |
| Feathered Escape | 60/60 | 88% (72-100) | 5 | Plays correctly but reaches 100% Busy and 46 FPS | Performance |
| Feed the Ducks | 30/30 | 16% (15-18) | 0 | No compatibility issues reported | Pass |
| Flip Knight | 30/30 | 13% (11-14) | 0 | No compatibility issues reported | Pass |
| Flooded Caves | 30/30 | 14% (10-18) | 1 | Procedural cave generation fixed; plays correctly. One startup stall sample | Pass |
| Froggy | - | - | - | No known issues; not present in this capture | Pass |
| Freezing Knights | 59/60 | 99% (63-100) | 4 | Multicart progression fixed and plays correctly, but is CPU-bound | Performance |
| Fuz | 30/30 | 56% (49-63) | 0 | No compatibility issues reported | Pass |
| Gar's Den | 30/30 | 75% (69-81) | 0 | Explicit-flip input fixed; plays correctly. 94% peak | Pass |
| Get Out of This Dungeon | 30/30 | 73% (21-74) | 0 | No compatibility issues reported | Pass |
| Golf Sunday | 48/60 | 100% (99-100) | 5 | Correct graphics but consistently CPU-bound; 38 FPS minimum | Performance |
| Grippy | 24/60 | 100% (100-100) | 5 | Severe CPU overload; observed 1 FPS minimum and stall samples | Performance |
| Guncho | 30/30 | 44% (34-64) | 0 | No compatibility issues reported | Pass |
| Hakai | 19/60 | 100% (100-100) | 5 | Consistently CPU-bound; observed 13 FPS minimum | Performance |
| Harold's Bad Day | 30/30 | 25% (24-26) | 0 | No compatibility issues reported | Pass |
| High Stakes | 60/60 | 40% (12-64) | 0 | No compatibility issues reported; transition samples exceeded cap | Pass |
| Hit8Ox | 30/30 | 91% (79-100) | 3 | Compatibility and reflection rendering reconfirmed after glyph globals replaced lexer substitution; rendered output often falls to about 7-9 fps and is currently unplayably slow | Performance |
| Hot Wax | 30/30 | 62% (54-78) | 0 | No compatibility issues reported | Pass |
| Hug Arena | 30/30 | 18% (17-22) | 0 | No compatibility issues reported | Pass |
| Hungry Harry 3D | 30/30 | 91% (76-98) | 2 | Runs correctly but has little S3 CPU headroom; 100% peak | Performance |
| Hydra | 30/60 | 82% (81-95) | 3 | Runs after preserving the full 16.16 srand seed; performance is poor. Procedural generation briefly peaks above one frame budget | Performance |
| Hyperspace | 30/30 | 79% (70-93) | 0 | No compatibility issues reported; 95% peak | Watch |
| iii Demake | 53/60 | 100% (73-100) | 5 | Runs but is consistently CPU-bound | Performance |
| Into Ruins | 30/30 | 88% (64-99) | 4 | Official offline multicart now runs correctly: deterministic title map, extensionless companion lookup, and 10,019-byte peek()/chr() entity hand-off are fixed. Gameplay reached 100% Busy and dipped to 24 FPS; the separate BBS bootstrap still requires its specifically named intoruins_main-7 companion | Performance |
| Invader Overload | 30/30 | 27% (14-45) | 0 | No compatibility issues reported; 50% peak | Pass |
| Islander | 60/60 | 97% (84-100) | 2 | Gameplay and outlined menu/inventory text render correctly | Performance |
| Jack of Spades | 30/30 | 91% (85-95) | 0 | Requires devkit mouse input through stat(32-34); Retro-Go target exposes no pointer and the cart has no active button fallback | Input dependency |
| Just One Boss | 30/30 | 16% (12-28) | 0 | No compatibility issues reported; 34% peak | Pass |
| King | 30/30 | 55% (54-60) | 0 | No compatibility issues reported; brief 25 FPS startup sample | Pass |
| Lab Cat | 30/30 | 33% (3-36) | 0 | No compatibility issues reported; low-Busy screen-mode transition samples included | Pass |
| Lava Joe | 60/60 | 74% (65-77) | 0 | No compatibility issues reported; 86% peak | Pass |
| Linecraft | - | - | - | Blocked by missing worlddata.p8 | Dependency |
| Little Architect | 30/30 | 42% (35-74) | 0 | No compatibility issues reported; 85% peak | Pass |
| Little Dragon Adventure | 60/60 | 59% (56-67) | 0 | No compatibility issues reported; brief 53 FPS startup sample | Pass |
| Little Eidolon | 24/30 | 88% (76-95) | 5 | Official two-cart package now resolves its renamed sole offline companion and plays correctly, but is severely CPU-bound; observed 10 FPS minimum and 97% Busy peak | Performance |
| Low Knight | 60/60 | 79% (27-97) | 2 | No compatibility issues reported, but reached 100% Busy and briefly fell to 10 FPS | Performance |
| Low Mem Sky | 44/60 | 81% (73-87) | 5 | Runs, but a long transition reached 314% Busy/9 FPS and gameplay remained uneven | Performance |
| Mai-Chan's Sweet Buns | 60/60 | 78% (57-89) | 0 | No compatibility issues reported; 91% peak | Watch |
| Marballs 2 | 30/30 | 63% (39-71) | 0 | No compatibility issues reported; one transition stalled at 242% Busy/11 FPS before stable gameplay | Pass |
| Marble Merger | 60/60 | 97% (86-100) | 1 | Runs correctly but is consistently close to the S3 CPU limit; 46 FPS minimum | Performance |
| Masters of the Universe | 30/30 | 91% (53-95) | 2 | No compatibility issues reported; reached 100% Busy | Performance |
| Metrocubevania | 30/30 | 9% (6-13) | 0 | No compatibility issues reported; 15% peak | Pass |
| Micro Murder | 30/30 | 39% (37-52) | 0 | No compatibility issues reported; a transition briefly fell to 14 FPS | Pass |
| Mistigri | 30/30 | 45% (27-56) | 0 | No compatibility issues reported; 61% peak | Pass |
| Moonrace | 16/30 | 100% (100-100) | 5 | Negative map-edge clipping fixed the disappearing track; graphics and gameplay now correct. Still severely CPU-bound, typically 13-20 FPS in gameplay | Performance |
| Mots 8-Ball Pool | 26/30 | 92% (75-95) | 5 | No compatibility issues reported, but CPU-bound; 16 FPS minimum | Performance |
| Mots Grand Prix | 30/30 | 82% (65-96) | 2 | Official two-cart edition now has correct menu input and enters gameplay after text-cart Unicode button glyph normalization. Gameplay reached 100% Busy and frameskip oscillated between 1 and 2 | Performance |
| Nanoman | 60/60 | 80% (48-86) | 0 | No compatibility issues reported; 91% peak | Watch |
| Nemo | 60/60 | 70% (63-77) | 0 | No compatibility issues reported; 81% peak | Pass |
| Nemonic Crypt | 30/30 | 31% (24-34) | 0 | No compatibility issues reported | Pass |
| Neon | 30/30 | 84% (69-98) | 2 | Tech demo renders and plays music correctly after immediate requested-pattern reporting fixed its first-frame cache initialization; reached 100% Busy and 25 FPS minimum | Performance |
| Night Ride | 30/30 | 92% (77-100) | - | Plays, but reached 100% Busy and briefly fell to 22 FPS | Performance |
| Ninja Cat | 60/60 | 73% (10-84) | - | No compatibility issues reported; 95% Busy peak | Pass |
| Nuklear Klone | 60/60 | 92% (77-100) | - | Plays, but CPU-bound with a 43 FPS minimum | Performance |
| Oblivion Eve | 58/60 | 88% (69-98) | 4 | Official multicart package loads and runs after `ord(nil,1,n)` zero-fill support; clean retest completed with both companion-cart transitions and no errors. CPU-bound, with 33 FPS minimum and 99% Busy peak | Performance |
| One Room Dungeon | 30/30 | 77% (73-79) | - | No compatibility issues reported; one 22 FPS sample and 83% Busy peak | Pass |
| Outvain | 30/30 | 90% (83-96) | - | Plays correctly but has limited S3 headroom; 99% Busy peak | Watch |
| P.Craft | 30/30 | 53% (3-57) | - | No compatibility issues reported; brief 17 FPS/95% transition | Pass |
| Pakpok | 30/30 | 19% (17-19) | - | No compatibility issues reported; 23% Busy peak | Pass |
| Pat Shooter | 30/30 | 14% (9-19) | - | No compatibility issues reported; 22% Busy peak | Pass |
| Pico Arcade | - | - | - | Excluded: launcher for an unofficial collection with no complete official pack available | Excluded |
| Pico Checkmate | 60/60 | 19% (16-92) | 2 | No compatibility issues reported; a heavy transition reached 93% Busy and 28 FPS | Pass |
| Pico De Pon | 60/60 | 64% (38-67) | 0 | No compatibility issues reported; 70% Busy peak and 53 FPS minimum | Pass |
| Pico Dino | 60/60 | 35% (29-49) | 0 | No compatibility issues reported; 63% Busy peak | Pass |
| Pico Driller | 30/30 | 85% (75-95) | 0 | No compatibility issues reported, but limited CPU headroom; 99% Busy peak | Watch |
| Pico Fox | 30/30 | 85% (62-100) | 2 | No compatibility issues reported; repeatedly CPU-bound with 100% Busy and 27 FPS minimum | Performance |
| Pico Froggo | 30/30 | 35% (33-45) | 0 | No known issues; 54% peak | Pass |
| Pico Monsters | 30/30 | 18% (18-19) | 0 | No compatibility issues reported; 21% Busy peak | Pass |
| Pico Night Punkin | 60/60 | 41% (6-89) | 0 | Official multicart package works through menu, selector and tutorial/song cart. Fixed with START-to-pause/Enter button 6 mapping and display-palette reset across `load()`; 97% Busy peak and 43 FPS transition minimum | Pass |
| Pico Off Road | 30/30 | 61% (41-95) | 2 | No compatibility issues reported; one 263% stall and 19 FPS minimum | Performance |
| Pico Racer | 30/30 | 56% (47-74) | 0 | No compatibility issues reported; 78% Busy peak | Pass |
| Pico Snail | 60/60 | 80% (73-87) | 0 | No compatibility issues reported; 93% Busy peak and 59 FPS minimum | Watch |
| Pico Tennis | 23/60 | 100% (100-100) | 5 | Runs correctly but is severely CPU-bound and very slow; 21 FPS minimum | Performance |
| Pico Tetris | 60/60 | 80% (74-87) | 0 | No compatibility issues reported; 92% Busy peak | Watch |
| Pico World Race | 59/60 | 87% (79-97) | 2 | No compatibility issues reported, but limited CPU headroom; reached 100% Busy and 44 FPS minimum | Performance |
| Pico Zombie Garden | 30/30 | 45% (44-47) | 0 | Runs, but gameplay is pointer-driven through `stat(32..34)` and the current handheld adapter has no mouse emulation | Input |
| PICO-BALL | 60/60 | 38% (37-39) | 1 | Official two-cart package now has correct splash/menu masking, off-screen text clipping, negative-width `sspr()` character turns, and responsive menu input under frameskip. Match gameplay is light; the animated launcher is much heavier at 82-94% Busy and renders about every other frame | Pass |
| Picobreed | 30/30 | 59% (44-69) | 0 | No compatibility issues reported; 76% Busy peak | Pass |
| Picohot | 60/60 | 16% (4-74) | 1 | No compatibility issues reported; heavy transition reached 94% Busy and 31 FPS | Pass |
| Picokaiju | 30/30 | 57% (51-98) | 1 | No compatibility issues reported, but a heavy transition fell to 16 FPS | Performance |
| Picopicotron | 30/30 | - | 0 | Boots after permissive `ord(false)` support, but the desktop simulation requires both mouse (`stat(32..34)`) and keyboard (`stat(31)`), which the handheld adapter does not emulate | Input |
| Picoracer-2048 | 30/30 | 74% (69-86) | 1 | Plays correctly; brief heavy samples reached 100% Busy and 28 FPS | Pass |
| Picoware | - | - | - | Requests an additional P8 file from its menu/game selection | Dependency |
| Picowars | 30/30 | 70% (54-80) | 0 | No compatibility issues reported; 86% Busy peak | Pass |
| Pieces of Cake | 59/60 | 94% (81-100) | 2 | Plays correctly but frequently reaches the S3 CPU limit; 52 FPS minimum | Performance |
| Pigments | 30/30 | 52% (38-59) | 0 | Persistent scores verified; one 81% peak | Pass |
| Pinballvania | 60/60 | 90% (76-99) | 2 | Correct but repeatedly alternates frameskip 1/2; 46 FPS minimum | Performance |
| Pizza Panda | 60/60 | 88% (77-96) | 1 | Correct title/gameplay/palette/music; 99% peak | Watch |
| Poom | 30/30 | 84% (66-100) | 3 | Correct rendering/music; 14 FPS minimum and one >100% stall sample | Performance |
| Porklike | 60/60 | 25% (21-31) | 1 | P8SCII glyph identifiers fixed; gameplay confirmed correct. A level/start transition briefly reached 288% Busy | Pass |
| Porter | 30/30 | 25% (22-27) | 0 | No compatibility issues reported | Pass |
| Praxis Fighter X | 34/60 | 100% (100-100) | 5 | Runs but becomes severely CPU-bound; observed 20 FPS minimum | Performance |
| Puzzle Cave | 30/30 | 18% (18-18) | 0 | No compatibility issues reported | Pass |
| Puzzles of the Paladin | 30/30 | 28% (11-67) | 3 | P8SCII glyph table fields fixed and gameplay confirmed. Initial screen is unusually heavy at 90-98% Busy and 13-16 FPS before recovering | Performance |
| Quest for the Book of Truth | 60/60 | 40% (35-57) | 1 | No compatibility issues reported; transition samples fell to 47 FPS | Pass |
| R-Type | 42/60 | 98% (96-100) | 5 | Runs correctly but is consistently CPU-bound; observed 24 FPS minimum | Performance |
| Rainmaker | 30/30 | 84% (75-91) | 0 | Gameplay and deferred `run()` restarts work; seven-restart retest confirmed stable heap with no cumulative VM loss. Restart frames briefly dip to 6-16 FPS | Pass |
| Ramps | 30/30 | 68% (51-87) | 2 | No compatibility issues reported; procedural/loading transitions reached 215% Busy and 6 FPS, with later brief 26 FPS dips | Performance |
| Return of the Slimepires | 59/60 | 82% (62-100) | 3 | No compatibility issues reported, but heavier scenes reach 100% Busy and 48 FPS | Performance |
| Rolly | 30/30 | 93% (89-100) | 1 | No compatibility issues reported. Menu is very light, but gameplay runs close to the S3 limit | Performance |
| Scrap Boy | 59/60 | 95% (68-100) | 3 | No compatibility issues reported, but gameplay repeatedly approaches the S3 CPU limit; 53 FPS minimum | Performance |
| Shadows of Dunwich | 42/60 | 100% (100-100) | 5 | No compatibility issues reported, but gameplay is consistently CPU-bound; 34 FPS minimum after an 8 FPS transition | Performance |
| Shape of Mind | 60/60 | 94% (89-100) | 2 | No compatibility issues reported, but consistently has little S3 CPU headroom; 55 FPS minimum | Performance |
| Shelled Shinobi | 30/30 | 53% (35-67) | 0 | No compatibility issues reported | Pass |
| Shodo | 30/30 | 15% (15-15) | 0 | No compatibility issues reported; very light CPU load | Pass |
| Slimey Jump | 60/60 | 15% (12-40) | 0 | No compatibility issues reported; later gameplay peaked at 64% Busy | Pass |
| Slipways | 53/60 | 85% (79-96) | 4 | Cart input is mouse-driven through `stat(32..34)` and has no active handheld button-navigation path; also has limited S3 headroom | Input dependency |
| Snakator | 58/60 | 82% (71-97) | 3 | No compatibility issues reported, but heavier scenes dip to 36 FPS and repeatedly raise frameskip | Performance |
| Sneaky Stealy | 30/30 | 78% (71-100) | 0 | Working after `tostr()` was corrected to preserve numeric-looking marker strings used for zero-indexed adjacency tables | Pass |
| Snow | 29/30 | 83% (71-95) | 5 | Graphics now correct after draw-palette RAM bit-4 transparency fix. CPU-heavy, with 15-34 FPS observed | Performance |
| Solais | 60/60 | 87% (72-98) | 2 | No compatibility issues reported, but reached 100% Busy and 57 FPS in heavier scenes | Performance |
| Solitomb | 46/60 | 94% (90-97) | 5 | Official four-cart package now loads and runs without logged errors after numeric-string memory reads and counted `peek2`/`peek4` support. Gameplay is entirely pointer-driven through `stat(32..34)`, including click, release and dragging, with no handheld-button fallback. It is also CPU-heavy and renders only about 8-12 FPS after frameskip reaches 5 | Input dependency / Performance |
| Snekburd | 60/60 | 80% (68-88) | 0 | Multi-cart loading, selector, gameplay and music work. The recent music regression was caused by globally converting populated speed-zero SFX to speed 1 for Wolfenstein; preserving shared custom-instrument data and applying the fallback only to direct `sfx()` playback restored the music | Compatible |
| Sonic | 60/60 | 84% (70-99) | 2 | Plays correctly, but heavy gameplay reaches 100% Busy and repeatedly alternates frameskip 1/2; 47 FPS minimum | Performance |
| Spaceman 8 | 30/30 | 23% (11-29) | 0 | No compatibility issues reported; level transitions briefly reached 60-72% Busy and 10-15 FPS | Pass |
| SPHONGOS | 30/30 | 32% (29-38) | 0 | No compatibility issues reported; 41% peak | Pass |
| Starjump | 30/30 | 77% (56-83) | 2 | No compatibility issues reported, but limited S3 headroom; 97% peak and 22 FPS minimum | Performance |
| Steps | 30/30 | 24% (13-25) | 0 | No compatibility issues reported | Pass |
| Storming The Grandmothership | 60/60 | 92% (76-99) | 4 | No compatibility issues reported, but gameplay reaches 100% Busy and 44 FPS | Performance |
| Stray Shot | 30/30 | 38% (37-40) | 0 | No compatibility issues reported; 44% peak | Pass |
| Subsurface | 60/60 | 78% (68-97) | 3 | No compatibility issues reported, but heavy scenes reach 100% Busy and 54 FPS | Performance |
| Suika Game Demake | 30/30 | 54% (40-63) | 0 | No compatibility issues reported; 72% peak | Pass |
| Super Disc Box | 60/60 | 70% (18-94) | 2 | No compatibility issues reported; demanding transitions reached 96% Busy and 28 FPS | Watch |
| Super Mario Bros. | 60/60 | 44% (32-73) | 0 | No compatibility issues reported; later heavy scenes briefly reached 96% Busy | Pass |
| SUPER World of Goo | - | - | - | Fully playable with no logged runtime errors. Uses a self-managed top-level `flip()` loop, so it never enters the normal Retro-Go tick/frameskip accounting path and the serial monitor reports zero FPS/Busy | Watch |
| Tempest | 30/30 | 93% (83-99) | 0 | Compatible, but demanding: world generation takes about 51 seconds on ESP32-S3 versus about 5 seconds on desktop PICO-8. The map now scrolls in progressively during the valid single-pass load. Gameplay holds 30 FPS but is close to the CPU limit | Performance |
| Terra | 60/60 | 94% (85-99) | 2 | No compatibility issues reported, but gameplay repeatedly approaches the S3 CPU limit and dipped to 50 FPS | Performance |
| Terra Nova Pinball | 59/60 | 76% (55-100) | 2 | No compatibility issues reported. An initial heavy transition ran at 10 FPS; normal play was mostly 59-62 FPS | Watch |
| Tetyis | 27/60 | 100% (88-100) | 5 | No compatibility issues reported, but gameplay is severely CPU-bound with sustained 16-49 FPS after the opening | Performance |
| The Heavens | 30/30 | 76% (42-92) | 0 | No compatibility issues reported; later gameplay peaked at 99% Busy | Pass |
| The Lair | 60/60 | 79% (40-94) | 0 | No compatibility issues reported; 96% peak | Pass |
| The Lost Night | 35/60 | 97% (85-99) | 5 | First level now plays correctly after restoring `split(nil)` to nil. No Tiny Hawk regression, but gameplay is strongly CPU-bound with 29-56 FPS after frameskip reaches 5 | Performance |
| The Merciless Deep | 30/30 | 90% (77-95) | 2 | No compatibility issues reported, but limited S3 headroom and a 100% peak | Performance |
| The Tower of Archeos | 22/30 | 98% (10-98) | 5 | First level and gameplay work after `foreach` was corrected to visit live in-place replacements. Scoped `-O2` improved the prior 18/30 FPS, 100% Busy capture, though gameplay remains CPU-bound with a 14 FPS minimum | Performance |
| They Started It | 60/60 | 77% (58-89) | 0 | No compatibility issues reported; 94% peak | Pass |
| Tiny Fisher | 30/30 | 84% (66-97) | 2 | No compatibility issues reported. One level transition stalled at 7 FPS/378% accumulated Busy; normal play recovered to 25-34 FPS | Watch |
| Tiny Golf Puzzles | 60/60 | 85% (79-91) | 0 | No compatibility issues reported; 95% peak | Pass |
| Tiny Hawk | 26/30 | 93% (49-100) | 5 | CPU-bound and uneven; 10 FPS minimum plus one long-stall BUSY anomaly | Performance |
| To A Starling | 60/60 | 90% (76-100) | 1 | No compatibility issues reported, but consistently close to the S3 CPU limit; 57 FPS minimum | Performance |
| Tomb of Gnir | 30/30 | 40% (33-48) | 0 | No compatibility issues reported; 54% peak | Pass |
| Trial of the Sorcerer | 30/30 | 68% (19-96) | 1 | Working; floor and ceiling confirmed restored after the incorrect `tline()` register-scale experiment was reverted. One earlier transition reached 1 FPS/100% Busy; normal gameplay recovered to 30 FPS | Compatible |
| Trichromat | 30/30 | 80% (70-94) | 0 | No compatibility issues reported; 96% peak | Pass |
| U-Turn | 30/30 | 82% (59-88) | 0 | No compatibility issues reported; 91% peak | Pass |
| UFO Swamp Odyssey | 60/60 | 90% | 3 | Working; foreground floor/ledge terrain restored by correct any-bit `map()` layer-mask semantics | Compatible |
| Underworld Siege | 30/30 | 64% (59-75) | 0 | No compatibility issues reported | Pass |
| Undune II | 60/60 | 78% | 1 | Working official multicart; intro deliberately switches to 30 FPS, then gameplay holds 60 FPS. Splash progression, house crests and map transfer fixed by `_update_buttons()`, dynamic `_set_fps()` and transient cross-cart `cstore()` hand-off | Compatible |
| Villager | 60/60 | 87% (78-100) | 2 | No compatibility issues reported, but demanding scenes reached 100% Busy; one accumulated 158% transition anomaly and 31 FPS minimum | Performance |
| Violet | 47/60 | 100% (100-100) | 5 | No compatibility issues reported, but consistently CPU-bound at 40-54 FPS | Performance |
| Whiplash Taxi Co | 30/30 | 91% (67-100) | 3 | Working; distant road/grass projection fixed by enforcing `clip()` in diagonal `tline()`. Visually correct but CPU-heavy, with 24 FPS minimum during measured gameplay after loading | Compatible / Performance |
| Winterwood | - | - | - | PNG decode succeeds and inflates 43,143 bytes of Lua, but this cart deterministically panics deep in Lua compilation. Increasing the dedicated task stack from 32 KB to 48 KB moved the stack addresses by the added allocation but failed at the same compiler point, ruling out stack exhaustion. Left unsupported to avoid risky parser/allocator changes for a single outlier | Panic / Unsupported |
| Wobblepaint | 30/30 | 12% | 0 | Runs but appears mouse-dependent; no controller interaction identified | Unsupported input |
| Wolfenstein 3D | 60/60 | 81% (60-99) | 2 | Fully playable. Scaled `sspr()` clipping keeps the HUD intact near walls; populated legacy speed-zero SFX normalization restores the gunshot | None observed |
| X-Wing Vs. Tie Fighter | 60/60 | 74-99% | 3 | Fully playable. Projected-line clipping prevents the native stall, and synchronizing runtime writes to packed SFX RAM restores its dynamically constructed positional weapon sounds | Compatible / Performance |
| X-Zero | 60/60 | 94% (82-100) | 2 | No compatibility issues or runtime errors reported. Simulation generally holds 60 Hz, but heavy racing scenes saturate the S3 and reduce rendered-frame smoothness | Compatible / Performance |
| Zepton | 60/60 | 91% (71-100) | 3 | No compatibility issues or runtime errors reported. Gameplay generally holds its 60 Hz simulation while automatic frameskip rises to 3 in demanding scenes | Compatible / Performance |

## Original ESP32 results

Results below use the same standardized protocol as the S3 matrix. Start with
the baseline and performance sets below to expose both compatibility regressions
and CPU limits quickly.

| Game | FPS/cap | Busy | FS | Compatibility / outstanding issues |
|---|---:|---:|---:|---|
| Celeste Classic | 30/30 | 28% median (21-81, 87 peak) | 0 | Compatible on CYD; 63 measured seconds, full speed through the captured heavy section, zero logged errors |
| Pizza Panda | 47/60 (40-63) | 100% (99-100) | 5 | No compatibility issue or error reported, but CPU-bound on CYD |
| Cab Ride | 14/30 (10-29) | 82% median (81-93, 97 peak) | 5 | No compatibility issue or error reported, but severely performance-limited on CYD |
| Snekburd | 60/60 (59-61) | 86% median (73-90, 92 peak) | 0 | Compatible on CYD; multicart bootstrap and gameplay completed without logged errors, but CPU headroom is limited |
| Poom | 21/30 (2-35) | 90% median (79-100; transition peaks above 100) | 5 | Compatible multicart and cartdata behavior on CYD; performance varies heavily by scene, recovering to 30 FPS in lighter play |
| Alone in the Pico | 16/30 (4-27) | 93% median (82-100; heavy stalls above 100) | 5 | Graphically compatible with no logged errors, but severely CPU-bound on CYD |
| Tiny Hawk | - | - | - | Not measured |
| Cattle Crisis | 51/60 (25-64) | 95% median (76-100) | 5 | Compatible on CYD; CPU-bound in heavy action but returns to 60 FPS in lighter scenes |
| Ascent | - | - | - | Not measured |
| Brutal Pico Race | 13/30 (10-17) | 100% | 5 | Stable with no logged errors, but too CPU-bound for practical play on CYD |
| Buns Bunny | 43/60 (24-61) | 100% median (37-100) | 5 | Compatible and initially near full speed, but enemy-heavy gameplay becomes CPU-bound on CYD |
| Dinky Kong | 53/60 (32-64) | 81% median (17-99, 100 peak) | 5 | Compatible on CYD; generally viable but demanding gameplay has scene-dependent slowdown |

## ESP32-P4 results

Results below were captured on the GB300-P4 using the same standardized
protocol as the S3 and original ESP32 runs. Scene differences can still affect
direct Busy comparisons, especially in procedurally varying or action-heavy
games.

| Game | FPS/cap | Busy | FS | Compatibility / outstanding issues |
|---|---:|---:|---:|---|
| Celeste Classic | 30/30 | 8% median (7-22, 24 peak) | 0 | Compatible on GB300-P4; full speed with substantial CPU headroom and zero logged errors |
| Pizza Panda | - | - | - | Not measured |
| Cab Ride | 30/30 (24-34) | 80% median (50-99, 100 peak) | 4 | Compatible and reaches its target rate on GB300-P4, but demanding scenes still consume most available CPU time |
| Snekburd | 60/60 | 37% median (31-59, 67 peak) | 0 | Compatible on GB300-P4; multicart gameplay held full speed with healthy CPU headroom and zero logged errors |
| Poom | - | - | - | Not measured |
| Alone in the Pico | - | - | - | Not measured |
| Tiny Hawk | - | - | - | Not measured |
| Cattle Crisis | - | - | - | Not measured |
| Ascent | - | - | - | Not measured |
| Brutal Pico Race | 26/30 (20-33) | 99% median (97-100) | 5 | Stable with zero logged errors and roughly twice the CYD frame rate, but remains CPU-bound and does not consistently reach its 30 FPS target |
| Buns Bunny | - | - | - | Not measured |
| Dinky Kong | - | - | - | Not measured |

## Retro-Go integration status

- Dynamic Retro-Go speed pacing, deferred low-memory Lua collection, reset,
  screenshots, redraw, atomic cartdata persistence, menus and About metadata
  are integrated.
- Save states remain intentionally unavailable because restoring only PICO-8
  RAM would omit the live Lua graph, closures, upvalues and coroutines.
- ZIP loading accepts exactly one `.p8` or `.p8.png` cartridge entry while
  ignoring non-cart metadata. Archives containing zero or multiple carts show
  an explanatory error; multicart packages must remain extracted into folders.

## Recommended test sets

### Quick regression set

1. Celeste Classic - core Lua, map, sprites, input and 30 FPS timing.
2. Pizza Panda - palette, transparency, text and audio.
3. Cab Ride - rendering and performance-sensitive gameplay.
4. Snekburd - cart chaining, relocated graphics and save/dependency handling.
5. Poom - complex rendering, multi-file loading, input and music.
6. Alone in the Pico - triangle rasterization.
7. Dinky Kong - compressed cart, short-if parsing, custom font, fractional
   sprites and a demanding 60 FPS workload.

### Performance stress set

1. Golf Sunday
2. Brutal Pico Race
3. Tiny Hawk
4. Ascent
5. Driftmania
6. Dank Tomb
7. Dinky Kong
8. Pinballvania
9. Buns Bunny
10. Cattle Crisis
11. Alone in the Pico
12. Crowded Dungeon
13. Poom

### Outstanding compatibility queue

1. Linecraft - determine whether worlddata.p8 is genuinely missing from the
   distribution or generated by its development kit.
2. Picoware - identify and supply the requested child cart.
3. Dungeon - compare the remaining click/missing long notes with desktop.
4. Bondstones - compare note envelopes and harshness with desktop.
5. Build a Jetpack - identify whether its serial numbers are intentional.
