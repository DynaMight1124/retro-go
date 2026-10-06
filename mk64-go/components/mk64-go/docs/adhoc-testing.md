# Ad hoc test notes (for testers)

## Build 14: leaving a race, and a safer save (please try)

EBOOT.PBP MD5 `ee4751fdb44c9224a022e73f41745559`.  **Every PSP needs this
build**: from now on two PSPs on different builds refuse each other, and the
lobby says "A PSP HAS ANOTHER GAME VERSION" instead of joining and drifting
out of sync.  (A PSP still on build 13 just sits on JOINING / SEARCHING.)
The first start rebuilds the game data from the ROM once, as after an install.

None of this has run on real PSPs yet, only in the emulator.  Please try:

1. **The pause menu in a race** now reads CONTINUE / MAIN MENU / LEAVE
   MULTIPLAYER (VS and Battle keep COURSE CHANGE and DRIVER CHANGE), full
   screen.  MAIN MENU takes everyone to the game select together, still
   connected.
2. **A joiner picks LEAVE MULTIPLAYER.**  The joiner should land on the main
   menu alone; the host should get "PLAYER 2 LEFT THE RACE" -- CONTINUE / EXIT
   at once (no five-second wait), and CONTINUE should carry on the race.
3. **The host picks LEAVE MULTIPLAYER.**  The host lands on the main menu;
   the joiner gets "HOST EXITED THE GAME" -- MAIN MENU at once.
4. **Another number of players.**  In a 2P session: pause, MAIN MENU, then the
   host moves to 3P GAME (only the host's pad moves that cursor now), picks a
   mode and confirms.  The host should go straight to "WAITING FOR 2 PLAYERS";
   the joiner should see "HOST DISCONNECTED" -- OK, and after OK its menu
   still shows the host's picks, so OK then JOIN RACE finds the new race.
   Picking 2P again instead is a rematch with no lobby, as before.
5. **After any of these, start a new ad hoc race** on the same PSPs without
   restarting the game: does the WLAN come back up and the lobby find the
   other PSP?
6. **The save**: there is now an `eeprom.bak` next to `data/eeprom.bin`.  If
   the game is interrupted mid-save (HOME, battery), the next start restores
   from the backup -- your cups and records should never reset again.

If anything goes wrong, send `data/log.txt` and `data/log_prev.txt` from every
PSP, and say which step it was.

---

## Build 6: the desync is fixed (please confirm)

I found the cause.  In versus races the circling bomb karts were only
simulated while they were on your screen; each console shows a different
player, so each froze different bombs, they drifted apart, and a bomb would
hit one player on one console only -- that is how you both "won".  Now every
console simulates every bomb.  As a backstop, if the two consoles ever
disagree again the race stops with "CONNECTION LOST -- RACE OUT OF SYNC"
instead of running on to two winners.

Please confirm:

1. Both PSPs on this build (the MD5 at the top), reboot both for clean logs.
2. A 2P VS race on a course with bombs (Luigi Raceway is a good one), ideally
   with the two of you driving in different parts of the track.
3. Does it stay in sync now?  If anything still goes wrong, send log.txt and
   log_prev.txt from both PSPs.

---

This build adds PSP-to-PSP ad hoc play to the 2P/3P/4P modes.  Two or more
PSPs, each with this build in `ms0:/PSP/GAME/MK64Portable/` (EBOOT.PBP plus
the ROM, the same as the normal install).

## Playing

1. Slide the WLAN switch on on every PSP.
2. On every PSP pick the same thing in the game select: 2P (or 3P/4P) GAME,
   the same mode (MARIO GP, VS or BATTLE) and the same class (50cc etc.).
3. Press OK.  A panel appears: HOST RACE / JOIN RACE / CANCEL.
   - One PSP picks HOST RACE.  It shows "WAITING FOR N PLAYERS" and counts
     down as the others join.
   - The others pick JOIN RACE.  They show "SEARCHING...", then "JOINING...",
     then "JOINED RACE" while the host waits for the rest.
   - CANCEL (or Circle) backs out at any point.
4. When the last player joins, every PSP moves to the character select
   together.  Everyone picks their own character.  The host picks the cup and
   course; the other pads do nothing on that screen.
5. Race.  Each PSP shows its own player full screen, with the usual HUD.

Things worth trying, in this order:
- 2P MARIO GP 50cc, one full cup.
- 2P VS, then 2P BATTLE.
- 3P or 4P if there are enough PSPs.
- Mid-race, a joiner presses HOME and quits: after about five seconds the
  host's game pauses with "PLAYER N LEFT THE RACE" and CONTINUE / EXIT;
  the other joiners see "WAITING FOR HOST...".  CONTINUE resumes with that
  kart parked; EXIT sends everyone to the main menu.
- Mid-race, the HOST presses HOME and quits: after about six seconds each
  joiner pauses with "HOST EXITED THE GAME" and a MAIN MENU button.

## If the two games stop agreeing (the other kart drives into walls, items differ)

That is a desync, not a dropped link: the log on each PSP prints "DESYNC at
frame N -- differs in: ..." lines from the first mismatch on.  Nothing to
do in the moment; just send both logs.  The "net: state check" lines near
the start of the session and every 20 s are there to be compared side by
side.

## If the players get disconnected during a race

Please note roughly how long into the race it happened and what each screen
showed, and send the logs (below).  Then one more try: put an empty file
named `cpu222` (no extension) into `ms0:/PSP/GAME/MK64Portable/data/` on
BOTH PSPs and race again.  That runs the game at 222 MHz instead of 333;
the original PSP-1000's WLAN is known to misbehave at 333 MHz.  Say whether
it made a difference, and which PSP models (1000 / 2000 / 3000 / Go) were
used.  Also check Settings > Power Save Settings > WLAN Power Save on each
PSP and say whether it is on.

## What to send back

From EVERY PSP that took part, the two files

    ms0:/PSP/GAME/MK64Portable/data/log.txt
    ms0:/PSP/GAME/MK64Portable/data/log_prev.txt

(log.txt is the last run, log_prev.txt the one before it), and a few words
per attempt: which PSP hosted, what was chosen, what happened, and roughly
when (the log lines carry seconds since boot).  If a PSP froze or reset, say
which one and what was on its screen.  If the race ran but the players saw
different things (one thought they won, the other did not), say so: the logs
record a checksum every 30 frames and will show where they parted.

The log is written continuously; at most half a second is lost on a freeze.
