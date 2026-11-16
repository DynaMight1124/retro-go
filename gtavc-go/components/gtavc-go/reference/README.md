# GTA Vice City – J2ME port (Nokia E72)

Open-world Vice City for MIDP 2.0 / CLDC 1.1, using its own software 3D renderer (no M3G).

## Getting the JAR (built on GitHub)
Push to `main` or `master`. The **Build JAR** workflow compiles, preverifies and packages the game, then publishes a Release with two builds:

| File | Content | Size |
|---|---|---|
| `GTA_VC_J2ME.jar` | the whole map | about 36 MB |
| `GTA_VC_J2ME_lite.jar` | Ocean Beach + Washington Beach only; the rest of the map is open sea | about 15–17 MB |

Try the lite JAR first on the E72. The full one is big for an S60 phone.
To make a different region, change `MAP_REGION=i0,j0,i1,j1` in `.github/workflows/build.yml`. Those are tile indices: tile = 100 m, x index = (GTA x + 1900) / 100, y index = (GTA y + 2100) / 100.

## Controls
| Key | On foot | Vehicle |
|---|---|---|
| D-pad | walk / turn | throttle, brake/reverse, steer |
| Fire / 5 | punch combo / bat swing / **hold to shoot** (pistol locks on to the nearest person) | throttle |
| 2 / 8, 4 / 6 | camera tilt / orbit | camera |
| 0 | toggle **run / walk** | handbrake (powerslide) |
| 1 (hold) | sprint | – |
| 3 | switch weapon (fists / bat / pistol) | – |
| * | enter / exit / carjack | exit |
| # / soft keys | pause | pause |

On the E72 QWERTY keyboard, use **Fn** (or numeric lock) for the number keys.

## Builds
| JAR | for | content |
|---|---|---|
| `GTA_VC_J2ME.jar` | E72 / big S60 phones | the whole map (~36 MB) |
| `GTA_VC_J2ME_lite.jar` | | Ocean Beach + Washington Beach |
| `GTA_VC_J2ME_legacy.jar` | Asha 311 class (S40) | Ocean Drive around the Ocean View Hotel, 4 cars, 5 peds, no intro movie, half resolution. On touch screens it shows transparent buttons. |

## Controls (new)
* **1**: tap toggles walk/run, hold to sprint.
* **0**: jump (handbrake in a car).
* **5 held with the pistol**: lock-on with a PS2-style reticle (green → red with the target's health).
* **0 + \***: debug/trainer menu. Spawn any vehicle from the list, set the time of day, Tommy speed, game speed, god mode, clear wanted level, weapons + money, teleport to spawn.
* **Options → ps2 filter**: warm colour grade, bloom and motion trails.

## Save / continue
The game saves to the phone (RMS record store) every 20 s, and whenever you pause, quit or the phone interrupts it. It keeps your money, your last position (where your car was, if you were driving), the clock, and the bat/pistol and ammo. **Continue** appears at the top of the main menu once a save exists.

## New in this build
* Dead peds drop cash about half the time (gang members drop more), and dead cops always drop their pistol. Dropped pickups vanish after 30 s.
* Ped and Tommy textures fixed (the UVs were flipped twice).
* Starts at night (22:00) with the white Admiral parked next to Tommy.
* Cars use the real `handling.cfg` values (mass, traction, top speed, acceleration, brakes, steering lock).
* Cars no longer get stuck on bridge ramps and kerbs: only steps over 1.1 m count as walls.
* Damage takes much longer. Panels dent and then fall off, the engine smokes when damaged, and at 0 HP it catches fire. About 5 s later it explodes: it kills you if you're still inside and hurts anyone close.
* The radar and pause map use your VC map picture (auto-aligned to the world). No ped or car blips, only flashing cops while you're wanted.

## What's in it
* **Full map**:
  * The 1.5 M-triangle Blender export is cut into 1,216 tiles of 100 m. They stream in on a background thread with a near and a far version of each tile, and textures load on demand into a 512-slot cache.
  * Ground uses a 2-layer height grid, so you can walk under bridges and stay on top of overpasses.
  * Walls/steps, lamps, palms and poles are solid.
  * There is generated sea using `waterclear256`. Tommy still can't swim.
* **Spawn**: in the alley beside `wshbuildws09` (Ken Rosenberg's office), with a pistol and a bat lying in the alley.
* **Peds**: 102 Vice City peds converted from `gta3.img`, including Tommy (`player.dff`), cops, gangs and street peds.
  * 12 models are loaded at a time and swapped every ~40 s, with about 22 peds on screen.
  * Peds follow the pavement, sometimes cross, flee from cars and gunfire. Gang members and some others fight back.
* **Vehicles**: 80 cars and bikes from `gta3.img` (every land vehicle except planes, boats, RC and the tank).
  * 10 models are loaded at a time and swapped, with up to 8 in traffic.
  * Panel damage (ok / damaged / falls off) and an explosion when wrecked.
* **Car physics**:
  * Velocity is now separate from the heading: grip pulls sideways speed down, the handbrake breaks rear grip, and yaw follows the steering with grip-limited response.
  * Slopes add gravity, cars have mass in collisions, and the body pitches and rolls.
  * The chase camera follows the direction of travel.
  * Handling classes: car / sports / heavy / bike / slow.
* **Cops**:
  * 1 star: they try to arrest you, and only hit back if you fight.
  * 2+ stars: they shoot, and police cars chase you (with siren).
  * Stars fade after about 22 s out of sight.
* **Sounds**: 15 effects pulled from `sfx.raw` (punch, colt45, bat, crash, doors, horn, pickup, explosion, splash, siren, engine idle/rev loops).
* **Screens**: real loading screens and title splash from the TXDs. The menu/HUD use Pricedown (`assets/fonts/hud.otf`) and tips use regular Noto Sans. The menu now fades in after the intro movie instead of cutting.

## Rebuilding assets (local, not in CI)
The sources are too large for GitHub, so keep them outside the repo:
```
python3 tools/map_full_export.py "path/GTA Vice City Map Full.obj"   # needs textures/ beside the .obj, ~10 min, ~1.5 GB RAM
python3 tools/ped_export.py   path/gta3.img
CARS=dir_with_dff_txd python3 tools/car_export.py admiral ...       # tools/img_extract.py pulls them out of gta3.img
python3 tools/weapon_export.py path/gta3.img
python3 tools/sound_export.py  path/Audio                             # sfx.RAW + sfx.SDT
python3 tools/screens_export.py path/txd                              # LOADSC*.TXD, SPLASH1.TXD
python3 tools/fx_export.py                                            # sprites + fonts
```
Building locally: JDK 17 + Python 3 → `bash tools/build.sh && python3 tools/package.py dist`.
`tools/test.sh Shots` runs a headless session with screenshots.
