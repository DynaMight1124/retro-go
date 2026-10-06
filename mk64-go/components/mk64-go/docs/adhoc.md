# Ad hoc multiplayer (branch `adhoc`)

Goal: two to four PSPs race or battle together over ad hoc WLAN, each showing
its own player full screen instead of a split-screen quarter.

## Design: input lockstep

MK64 is a deterministic simulation driven only by controller inputs (the RNG
is a 16-bit shift register seeded at race start, the one wall-clock value the
game reads advances a fixed two ticks per frame on the port, and audio never
feeds gameplay).  So every machine runs the whole game, all players included,
and only the pads travel:

- Slot 0 is the host; it assigns slots to the peers in join order and sends a
  START packet with the slot -> MAC table.  From then on both machines count
  frames from 0 and stay frame-locked.
- Frame F consumes, for every slot, the input that slot sampled at its frame
  F - INPUT_DELAY (2 frames = 67 ms).  A machine that lacks a slot's input for
  F stalls (holds the last picture) until it arrives.
- Every packet repeats the last REDUNDANCY (8) frames of local input, so a
  lost packet costs nothing; ad hoc PDP is unreliable by design.
- Every CHECK_EVERY (30) frames a checksum of the players' state rides along.
  A mismatch is a desync: logged as `net: DESYNC at frame N` and flagged in
  `gPortNetDesync`.  This turns a determinism bug into a bisectable frame
  number instead of a mystery.
- The session starts before the game does (`port_net_boot()` runs before
  `port_game_init()`), so the menus are part of the lockstep too: the host's
  pad is pad 1, the client's pad is pad 2, and the game's own 2P menus,
  character select, pause and results just work.

Files: `src/port/net/port_net.h` (API), `lockstep.c` (session),
`net_adhoc.c` (PSP transport: one PDP socket, broadcast), `net_file.c`
(mailbox files on the shared memory stick, for PPSSPP tests).  Hooks:
`osContGetReadData` / `osContInit` in `ultra_shim.c`, the boot and the frame
loop in `psp_main.c`.  Everything is behind `-DPORT_NET` plus one transport
define.

## Starting a session: the lobby

Every player sets up the race in the game's own menus: 2P/3P/4P GAME, the
mode (Grand Prix, VS, Battle) and the class, then OK.  For two or more players
the OK press opens a modal over the game select: HOST A RACE / JOIN A RACE /
CANCEL (Up/Down, Cross, Square cancels at any point).

- HOST loads the network modules, joins the ad hoc group and advertises the
  race it set up (players, mode, class) twice a second, showing "WAITING FOR
  N MORE PLAYERS".  Joiners whose race matches are given slots; when the
  slots are full the host sends START and the game goes on.
- JOIN loads the modules and listens for an advert that matches the race the
  joiner set up ("SEARCHING..."), asks that host for a slot ("JOINING...")
  and, once the host's advert lists its id among the filled slots, shows
  "JOINED RACE" until START.  If the advert shows the race full without it,
  it drops that host and searches again.
- START carries the host's selections, RNG seed and menu timers; both
  machines then make the OK transition into the character select in lockstep
  frame 0 (host = pad 1, joiners = pads 2-4).  Everyone picks their own
  character; the course select answers the host's pad only.  Character select, course
  select, the race, pause and results all run in lockstep from there.

Every packet carries the sender's build id (`gPortBuildId`, a hash of the
code objects generated at link time by `tools/psp/gen_build_id.py`): lockstep
needs the same simulation everywhere, so a packet from another build is
refused and the two never join.  The lobby then shows "A PSP HAS ANOTHER GAME
VERSION" under SEARCHING / WAITING (a build older than 14 sends id 0 and only
ever sees SEARCHING).  START is also checked against the race the joiner asked
for (players 2-4, the same input delay) before it sizes anything.

Scripted tests pick the modal choice from `data/netrole.bin`: 0x12/0x13/0x14
= HOST for a 2/3/4-player race, 2..4 = JOIN.  Nothing there: the modal waits
for the pad.  A machine without a session plays single player as before.

## Milestones

1. **Simulation lockstep** (this commit): two machines run the 2P game in
   lockstep, both still rendering the game's split screen.  Proves the
   transport, the stall logic and determinism.  Test: two PPSSPP instances
   with the file transport, checksums match for a whole race.
2. **Memory probe on the PSP** (done 2026-09-10, David's PSP, hosting):

   | step | free | largest block |
   | --- | --- | --- |
   | before the net modules | 1857 KB | 1621 KB |
   | after NET_COMMON + NET_ADHOC | 1649 KB | 1413 KB |
   | after sceNetInit (128 KB pool) + adhoc + adhocctl init | 1505 KB | 1285 KB |
   | after adhocctl connect + PDP socket | 1505 KB | 1285 KB |

   The network stack costs about 350 KB and leaves 1.25 MB, so memory is not a
   blocker.  The transport connects a group on hardware; the PDP socket was
   created (no peer was present).
3. **Full-screen local view** (done 2026-09-10): `port_render_local_player()`
   in skybox_and_splitscreen.c walks the single-player render sequence with
   every player-one resource indexed by the local slot (camera, zoom, matrices,
   skybox buffer, kart renderer) through a shadow of the player's screen struct
   whose viewport is the whole screen, at the wide FOV and the 1P geometry
   mode.  The HUD is the game's split-screen drawers for that player at
   full-screen coordinates (`port_net_hud_layout()`); only the local rank, map
   and lap/time/item draw; the split-screen divider and the position-portrait
   pair are skipped in a session.  Verified: two PPSSPP instances, each showing
   its own kart full screen, zero desyncs, pause menu works.
   Left for polish: the 1P position-portrait column (the 2P HUD animates the
   portraits into its own layout every frame), the mini-map at 1P
   coordinates for slot 0, the kart shadows for slots 2-3 (func_80021B0C /
   func_80021C78 only cover two screens), the ceremony/results camera.
4. **Lobby, 3-4 players, drop-outs** (done 2026-09-10): the in-menu lobby
   above (first version: the boot console with L/R held, replaced the same
   day); START carries the selections, seed and timers.  The
   host relays every slot's inputs it knows (star on top of the broadcast),
   so client-to-client delivery is never required.  A slot the host has
   waited 10 s for is declared dropped from that frame on and reads as an
   idle pad on every machine from that same frame (the survivors stay in
   step); a client that hears nothing from the host for 15 s drops everyone
   and plays on alone.  Verified with three PPSSPP instances: a 3-player VS
   race, each machine full screen on its own kart, zero desyncs; killing one
   client mid-session dropped it at the same frame on both survivors, which
   ran on with identical state.
   Left for polish: the 3P/4P HUD (map position per slot, the timer), a
   proper lobby screen instead of the boot console, a "connection lost"
   message on the dropped machine.
5. **Two real PSPs**: ad hoc transport on hardware, latency and loss tuning.

## Testing in PPSSPP

### Authoritative results

The host sends one immutable result containing the winner, all eight ranks
(including GP CPUs), VS and Battle totals, and the GP points before the results
animation. Clients apply that result regardless of their local finishing order.
Rank updates remain locked until `setup_race` starts the next race. A client
that has not finished locally enters `RACE_DONE` when the host result arrives.

A client that finishes first holds only the results countdown: game iterations
and controller sampling continue so the host can still finish. Once finished,
the host holds network frame F and retries its result every 50 ms until each
remaining client acknowledges receipt. Clients acknowledge duplicate packets
without reapplying scores. Results and acknowledgements carry a session ID and
an explicit race generation, so previous-race packets cannot settle a new race.

Every console starts the results countdown at F + INPUT_DELAY + 1. Clients
cannot simulate that frame until the host advances after receiving all ACKs,
because the host has not sampled that frame's input yet. This keeps result
menus aligned even when the consoles finish at different frames. Result packet
handling runs before gameplay-input availability checks to avoid deadlocks.

If delivery cannot complete within the transport timeout, the host aborts with
`CONNECTION LOST / RESULT NOT DELIVERED`; no client falls back to a local result.
Mid-race checksum diagnostics remain enabled. Finished/post-result simulations
are excluded from comparison, and each new race seeds its RNG from the common
session ID and race generation before initialization.

The packet format changed: **all consoles must use the same updated build**.
Run the host-side regression harness with:

```
python3 tests/port/test_net_results.py
```

It runs the production protocol, input rings, relay, result countdown gate and
rank writers with stubbed driving/UI/platform calls. It covers 2–4 players,
different finish times, packet loss, stale results/ACKs, immutable retries,
Battle totals, and consecutive races. Hardware testing is still needed for the
race-to-results presentation and WLAN behavior.

### Scripted races

Build: `gmake -f Makefile.psp -j8 EXTRA_CFLAGS="-DPORT_INPUT_SCRIPT -DPORT_GFX_DEBUG -DPORT_COURSE_TEST -DPORT_NET -DPORT_NET_FILE"`.
Two or three game folders, `data/netrole.bin` = 0x12 or 0x13 in the host's,
2 and 3 in the clients', no `testcourse.bin`.  Start the host instance first.  Both run the input script,
which picks 2P GAME; the client's script becomes pad 2.  The mailbox files
live in `ms0:/mk64net/` (the emulator's memory stick directory).

## Leaving a race: the pause menu and the game select

The pause menu is the game's own and runs in lockstep like everything else, so
CONTINUE, COURSE CHANGE, DRIVER CHANGE and QUIT take every machine to the same
screen in the same frame, still in the session.  QUIT lands on the game select
with the session's number of players preselected; confirming that number (any
mode, any class) is a rematch with the same group and opens no lobby.

**Another number of players ends the session.**  In a session only the host's
pad moves the 1P/2P/3P/4P cursor, and only the host's OK counts while the
number differs from the session's.  That OK is a lockstep input, so every
machine sees it in the same frame and ends the session there
(`port_net_end_for_new_race()`): no packet is involved and none can be lost.

- The host takes the WLAN down and goes on with what it picked: 1P starts the
  single-player game; 2P-4P goes straight to hosting the new race ("WAITING
  FOR N PLAYERS", no HOST / JOIN choice -- it has said what it wants), which
  brings the WLAN up again under a new session id.
- Each joiner takes the WLAN down and shows "HOST DISCONNECTED" -- OK over the
  game select.  The menu under it still holds the host's picks (players, mode,
  class: they were made in lockstep), so after OK the joiner's own OK press
  opens HOST / JOIN with the matching race already set up, and JOIN finds the
  host's new lobby.

Scripted test: add `-DPORT_NET_RECOUNT_TEST` to the scripted-race build below.
The host backs out of the 2P character select, picks 3P VS and confirms; the
joiner dismisses the prompt and joins.  Expect on both machines `net: the host
set up a 3-player race at frame N` with the same N, then `lobby -> hosting` /
`joined the race`; `shot7000` (joiner) is the prompt, `shot7100` the new lobby.

**LEAVE MULTIPLAYER.**  In a session the pause menu reads CONTINUE / (COURSE
CHANGE / DRIVER CHANGE in VS and Battle) / MAIN MENU / LEAVE MULTIPLAYER, in
the 1P layout with the whole screen dimmed (each machine shows one player, so
the split-screen half is wrong whoever paused).  MAIN MENU is the game's QUIT
under the name of where it takes everyone.  LEAVE MULTIPLAYER is one line past
each mode's QUIT state (menu_items.c, states 25 / 33 / 45) and calls
`port_net_leave(slot)` with the pad that paused -- a lockstep input again, so
every machine acts in the same frame, without a packet:

- the leaver ends its session, takes the WLAN down and quits to the main menu;
- if it was the host, every joiner takes the WLAN down and shows "HOST EXITED
  THE GAME" -- MAIN MENU over the paused race, at once instead of after
  GIVEUP_AFTER_US;
- if it was a joiner, the others drop its slot from the next frame (no
  DROP_AFTER_US wait) and the race stays paused: the host gets "PLAYER N LEFT
  THE RACE" -- CONTINUE / EXIT, the other joiners "WAITING FOR HOST...", and
  CONTINUE unpauses everyone in lockstep as after any drop-out.

Scripted test: `-DPORT_NET_LEAVE_TEST=1` (the joiner leaves) or `=2` (the
host) on the scripted-race build.  Expect `net: slot S chose LEAVE MULTIPLAYER
at frame N` with the same N on both machines; shot1650 is the pause menu,
shot1700..1880 the prompt on the machine left behind, whose A at 1900 picks
CONTINUE / MAIN MENU.  After a joiner left, the host's frame heartbeats go on
with no new stalls.

## Drop-outs

The host alone declares a drop: a slot whose input it has waited
DROP_AFTER_US for (5 s on the PSP, 10 s on the file transport) is dropped
from that frame and reads as a neutral pad on every machine.  The host then
pauses everyone and asks: "PLAYER N LEFT THE RACE" -- CONTINUE / EXIT.  The
pause and the exit travel as flags in the host's own inputs (NETIN_PAUSE,
NETIN_END), so every machine applies them at the same lockstep frame.  In
the race the pause is the game's own (what START does); in the menus the
overlay freezes them.  Joiners show "WAITING FOR HOST..." meanwhile; EXIT
ends the session everywhere and sends everyone to the main menu, joiners
first seeing "HOST EXITED THE GAME" -- MAIN MENU.

A joiner that hears nothing from the host for GIVEUP_AFTER_US (6 s / 15 s)
ends its session and shows the same "HOST EXITED THE GAME" prompt; one that
finds its own slot in the host's dropped mask (it was away too long but is
still here) shows "YOU WERE DROPPED FROM THE RACE".  Both keep the
full-screen view and the paused race under the prompt until MAIN MENU.
