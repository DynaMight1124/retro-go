VeMUlator
=========

This is a port of the Android SEGA Dreamcast VMU emulator "VeMUlator" for libretro, it was translated from Java to C++ and then implemented the libretro.h callbacks.

## BIOS

Drop a VMU BIOS dump into the frontend's system directory and the core boots it
instead of emulating the system calls, which is what makes the clock, the file
manager and the MODE/SLEEP buttons work. Recognised names:

| Revision | File |
| --- | --- |
| American | `en1005-19991026-315-6208-05.bin`, `vmu_bios_en.bin`, `vmu_bios.bin` |
| Japanese | `jp1004-19980930-315-6208-01.bin`, `vmu_bios_jp.bin`, `vmu_bios.bin` |

Plain 60KB and full 64KB dumps both work, as do encrypted images. The `BIOS`
core option picks a revision or turns the BIOS off; the default tries American
first, then Japanese. With no BIOS present the core falls back to high level
emulation exactly as before.

The BIOS boots to its mode-select screen and waits there, which is what real
hardware does -- it is a menu, not a splash. **MODE** cycles the three modes and
**A** opens the one it is showing; the matching icon lights in the strip
underneath. Nothing on the screen changes as MODE is pressed, which makes it
look inert -- watch the BIOS's own mode byte at RAM 0x30 (0 file, 1 game,
2 clock) and it is plainly cycling.

* **File** lists what is on the flash -- the count, the name, the date -- and
  RIGHT opens that file's actions, Copy and Delete, with A for yes and B for no.
* **Game** runs a mini-game straight off the flash. Tetris boots from a flash
  image built here, shows its title screen and plays, sideways, the way a VMU
  game is meant to be held.

It powers itself off after about two minutes of no input, also as it should.
The clock is seeded from the host once, shortly after the BIOS has finished
clearing its own system variables; from there the BIOS counts base timer
interrupts and keeps its own time.

A BIOS wants a real 128KB flash image (`.bin`) to browse; a bare `.vms` is
wrapped in a synthetic filesystem that the BIOS can see but that no real VMU
would have written.

## The icon row

A real VMU has four icons in a strip under the dot matrix: a file, a spade for
game mode, a clock, and the flash-write marker. They are separate LCD segments
driven from XRAM bank 2, one byte each at 0x181, and the BIOS lights whichever
belongs to the mode it is in -- blinking it, which is why it is not always on
in a single frame.

The artwork was already in this tree, commented out, with the note that the
icons were not needed while everything ran on high level emulation. With a BIOS
they are, so they are drawn now.

They are **not** drawn from that artwork, though. It is 24x32 with one-pixel
lines, and shrinking that to the twelve pixels each icon actually gets turns
the lines into mush; worse, the four sit at different places within their
grids, so one crop for all of them caught the middle of the write icon and
rendered it as a solid black block. Since a segment is etched into the glass
rather than drawn on a grid -- it is one shape, lit or not -- the four are
hand-drawn at 12x12, the size they are shown at, and blitted. They are legible
now:

```
  ..########.......#..........####......########..
  ..#......#......###.......##....##....########..
  ..#.####.#.....#####.....#...#....#...###..###..
  ..#......#....#######....#...#....#...###..###..
  ..#.####.#...#########...#...#....#...###..###..
  ..#......#...#########...#...####.#...###..###..
  ..#.####.#...#########...#........#...########..
  ..#......#.....##.##......##....##....###..###..
  ..#......#......###.........####......########..
  ..########............................########..
```

If you want the segments to look properly etched rather than merely legible,
the way to get there is more resolution: four icons across a 48 pixel frame is
twelve pixels each, and no amount of care buys detail that is not there. That
would mean scaling the whole output, since the picture and the strip share one
raster.

Because the icons are their own segments rather than part of the picture,
showing them makes the frame **48x44 instead of 48x32**. The `Icon row` core
option turns them off and puts the frame back to 48x32 for anything that wants
the old geometry. Nothing lights them under high level emulation, where XRAM
bank 2 is never written, so the strip just stays blank there.

## Serial link (VMU to VMU)

Two VMUs joined by their connectors talk over the LC86K's synchronous serial
channels, SIO0 and SIO1. The core emulates those channels and carries the
bytes over the frontend-hosted link bus
([libretro/RetroArch#19454](https://github.com/libretro/RetroArch/pull/19454)),
so two instances of this core running in one frontend can reach each other.
The `Serial link to another VMU` core option turns it off. A frontend that
hosts no link bus leaves the socket empty and nothing changes.

The channels are modelled a byte at a time rather than a bit at a time -- see
the header comment in `serial.h` for exactly what that does and does not buy.
They are full duplex: the side being clocked shifts whatever its own SBUF
holds back over the same eight edges, because the shift register is one
register. The BIOS's own cable protocol depends on that and does nothing
without it.

Two things about how a VMU sees the cable are worth knowing if you are reading
the code:

* **Software checks the connector before the serial port.** P7 bit 3 is the
  state of connector pin 6, and that is how a machine notices another one has
  been clipped on. Chao Adventure's MATING mode sits on "PLEASE CONNECT"
  polling P7 and never touches SCON at all until it goes high; then it says
  "KEEP CONNECT" and starts the handshake. The core raises it when the
  frontend reports a peer on the bus.
* **The cable crosses.** Two VMUs clip together face to face, so one unit's
  connector is the mirror of the other's and SIO0 lands on the peer's SIO1.
  Both ends drive SO0/SCK0 and listen on SIO1, which only works because of
  that crossing.

Verified with two instances of the core running Chao Adventure. Its menu has
two linked modes and both work:

* **BATTLE** runs the whole thing through. The two machines hand over eight
  bytes of Chao (`A4 A4` -> `B3`, then `00 C0 07 80 01 86` each way), ask to be
  unplugged, and once they are, each plays out a fight against the Chao it
  received, health bars and all.
* **MATING** works too, once the Chao are old enough. A stock Chao Adventure
  file holds a baby, and two babies just tell each other "IT IS STILL A
  CHILD". Give them grown-up Chao and they hand over the whole 512-byte Chao
  record each way -- 518 bytes with the greeting -- ask to be unplugged, and
  then show the pair with a heart between them.

  What the game tests is the **Chao's type byte**, the first byte of the SA1
  Chao record. The record's base is not fixed: the file carries an ASCII
  `CHAO ACCEPT` marker (at 0x1E0 in the stock file) with a little-endian u16
  slot pointer at marker+0xC, which points at 0x3000. Within the record:

  | Offset | Field |
  | --- | --- |
  | +0x00 | **Type** -- bit 7 marks the slot occupied, low bits are the kind: 0 baby, 1 normal, 2 swim, 3 fly, 4 run, 5 power, 6 chaos, 7 egg |
  | +0x14, +0x16 | HP and max HP, x100 (0x05E0 = the 15 on the STATUS screen) |
  | +0x28 | Magnitude, float -- the Dreamcast's evolution progress, 0 to 1.2 |
  | +0x38 | position x, y, z, floats |
  | +0x60 | Breed, the desire to mate |

  A stock file has 0x80 there: occupied, kind 0, a baby. Anything from 0x81 up
  mates. Clearing bit 7 instead makes the game forget it has a Chao at all and
  boot to its title screen. Magnitude is what the Dreamcast side evolves and
  the VMU game never looks at it -- setting it to 1.0 on its own changes
  nothing, and changing the type on its own is enough.

  The layout is from X-Hax's SA1 VMS editor
  ([VMS_Chao.cs](https://github.com/X-Hax/sa_tools/blob/master/SA1Tools/VMSEditor/VMS_Chao.cs));
  which byte Chao Adventure itself gates on, and the 0x0FF80 state page, were
  found here by experiment.

Note the protocol needs someone to speak first: two machines started in perfect
lockstep sit sending `A4` at each other forever, which real hardware never
does. A few frames of skew and it completes.

**Chao Adventure 2 has no cable feature at all**, so there is nothing to test
there. Its menu is ITEMS, SNACK and FRIEND, and its Chao spends its time
whistling, eating flowers, napping and taking photographs; the first game's
MATING and BATTLE have no counterpart.

That is worth stating carefully, because the wikis describe both games in one
breath and several say the second supports connecting two units. The game
disagrees. Both files keep their interface text as plain ASCII, and the first
one holds every string the cable needs:

    0x0E6AD  'MATING '
    0x0E6B5  'BATTLE '
    0x0E833  'CONNECT'
    0x0E83D  'DO NOT  DIS-     CONNECT'
    0x0E885  'WAITING FOR CONNECTION'
    0x0D608  'A NEW CHAO IS BORN!'

The second holds none of them. Searching for the tails of the words rather
than whole words -- `ONNECT`, `ATTLE`, `ATING` -- so that a substituted first
letter cannot hide a match, the first game hits on all three and the second
only ever hits `ATING` inside `EATING_SNACKS` and `FASCINATING`.

It will not run properly as shipped either: it is downloaded from Sonic
Adventure 2's transporter together with a Chao, 128 blocks of the pair, and a
copy without one has an empty slot -- the `CHAO2ACCEPT` marker's pointer reads
`FF FF FF FF` -- and says so, `THERE IS NO CHAO`. It idles on a looping "my
first adventure!" with no menu behind any button. It does set Port 1 up for
serial and does see the connector when another unit is clipped on
(`P7` goes `0x0A`), but it never starts a transfer, and nothing crosses in four
minutes of emulated time.

### The BIOS's own file transfer

Two VMUs can copy a file between them with no console involved, and it works.
The path through the BIOS is not obvious, so:

1. **A** from the mode menu opens file mode, which shows free and used blocks.
2. **RIGHT** goes to the file list (`N001/002`, the name, the date), and
   **DOWN** walks it.
3. **RIGHT** again shows that file's details -- its description, its size in
   blocks, whether it is a game.
4. **RIGHT** once more opens the actions: **Copy** and **Delete**, with UP and
   DOWN between them.
5. **A** on Copy asks `Copy?` with **Yes** and **No**. `No` starts selected --
   **LEFT** moves to `Yes`, then **A**.

The receiving unit just needs to be in file mode with the cable in: it says
`Waiting for data` and stays there. Note that entering file mode with a peer
already attached goes straight to that waiting screen, so the sending unit has
to be at the Copy prompt before the cable is connected, exactly as on hardware.

Verified end to end: a 7-block `TETRIS` copied from a two-file image to an
empty one, 3939 bytes over the wire, and the 3584 bytes of payload on the
destination are identical to the source, with a correct directory entry. The
sender then says `Copied`.

## Save states

The core serializes, so a frontend can snapshot, rewind and run it netplayed.
A state is 134001 bytes, 128KB of which is the flash.

It carries RAM and both its banks, the display banks, the flash, the CPU, both
timers, the base timer, the interrupt lines, the pulse generator and the
serial channels, plus what the machine itself holds: the prescaler, the clock
`OCR` settled on, the connector. It leaves out ROM, which the BIOS fills at
load and nothing writes afterwards, and the frame buffer, which is redrawn
from XRAM every frame. States are stamped, and one of the wrong size or from
another version is refused rather than read in.

Each subsystem names its fields once, in a `serialize()` that one pass drives
in whichever direction is wanted -- counting, writing or reading. That is the
point of the shape: a field cannot be saved and then not restored, and the
size cannot drift from what is written, because all three walk the same list.

Checked by running on from a state, then restoring it and running the same
distance again, comparing the whole serialized state rather than just the
picture. Byte for byte identical -- including when the save lands part way
through an instruction rather than on a frame boundary -- and a second machine
that had been running elsewhere takes the state, becomes byte for byte the
first one, and runs on identically. The test has teeth: drop the program
counter from the state and it fails at once.

### Starting two machines the same

Netplay also needs two machines switched on apart to be the same machine from
the first frame, and two things stopped that. The clock is seeded from the
host's wall clock, so the `Clock at power-on` option has a `fixed` setting:
Saturday 1 January 2000, midnight, built by hand rather than through
`localtime` so that peers in different time zones agree as well. And the CPU's
instruction counter was never initialised while being part of the state, so two
cold starts hashed differently from the first frame even when they emulated
identically; it starts at zero now. With `fixed`, two cold starts give the same
state at every frame, and `system` a minute apart does not -- which is the check
that the setting is doing the work. `system` stays the default: a VMU on its
own should know the time.

## Saves

The whole 128KB of flash is the save, and the core hands it to the frontend as
`RETRO_MEMORY_SAVE_RAM`, so it is persisted to `.srm` and restored on load. A
VMU has no separate save area: mini-games live in the same flash the saves do,
and a game writes its state back into its own file through the BIOS firmware
call. Chao Adventure does that on the way out, when MODE is pressed -- two
pages, at `0x03000` and `0x0FF80` -- so without this its Chao was discarded
every time the core closed. With it, the Chao persists and its counters move
on across sessions as the clock advances.

Because the `.srm` is the flash, it is also a VMU image: rename it to `.bin`
and it loads as one.

The older `Enable flash write` option is a separate, cruder thing -- it writes
changes straight back into the `.bin` you loaded, in place. Saves work without
it.

## Mini-games that use the cable

Of the games to hand, only Chao Adventure uses the VMU-to-VMU cable. Recorded
here so the search is not repeated:

| Game | Uses the cable |
| --- | --- |
| Chao Adventure | **yes** -- MATING and BATTLE, both working |
| Chao Adventure 2 | no such feature; see above |
| Power Stone Mini | no -- its menu is MINI GAMES, HI-SCORES, SOUND |
| Power Stone 2 | no |
| Tech Romancer Mini | no |
| Skies of Arcadia: Pinta Quest | no cable feature; its `TRADE` is with the Dreamcast |

The Capcom three and Pinta Quest all set Port 1 up for serial at startup
(`P1DDR=0xA4`, `P1FCR=0xBF`), which looks promising and is not: it is just
their standard port init, and none of them ever sets `SCON.CTRL`.

## Sound

The VMU has one square wave. Timer 1's low half free-runs, `T1LR` sets the
period and `T1LC` the point in it where the output flips, and the pin drives a
piezo. There is no volume and no second voice.

Getting that to a frontend is mostly book-keeping, and the core used to get
all of it wrong. Measured against the promise it makes in
`retro_get_system_av_info`, it delivered **18%** of the audio it owed -- 61095
samples where 327680 were due over ten seconds, individual frames carrying
anywhere between 1 and 546. A frontend resamples on that promise, so that is a
clock wrong by a factor of five.

What was wrong, and what it sounded like:

| | Was | Now |
| --- | --- | --- |
| A silent frame | one sample, from a bare early exit | a full frame of silence |
| Frame length | `32768 / 60` rounded down to 546, losing 8 samples a second | 546 or 547, the fraction carried |
| Phase | restarted at every frame boundary -- a 60Hz buzz under every note | carried across frames |
| Period | truncated to whole samples: 16% sharp at `T1LR=0xF0`, and a click instead of a note above that | a double; too short for the sample rate is silence |
| Level | 0 to 0x7FFF, so a +16074 DC offset and a thump at each end | -16384 to +16383, same peak to peak |

Delivery is now 327679 samples against 327680 due, and every pitch measures
within 0.00% of what `T1LR` asks for.

## Timers

The rates are not a matter of taste. Sega's *Visual Memory System Hardware
Manual* gives them outright -- the scanned
[PDF](https://dmitry.gr/images/VMU.pdf), or the original Word document it was
made from, which has the tables intact where the scan garbles them. All of
these were checked against it and then measured, by running a VMU on a ROM of
NOPs and counting cycles between overflows.

**Timer 1 never overflowed.** Its counters were declared `byte` while the
overflow test is `> 255`, which a byte cannot be. The counter wrapped
silently, so in the whole history of this core timer 1 never reloaded, never
set `T1LOVF` and never raised its interrupt. With them widened, the period for
`T1LR` of `00/80/C0/F0/FE` measures 256/128/64/16/2 cycles, which is
`(256 - T1LR)` exactly.

**The timer 0 prescaler was off by one.** It tested for overflow before
counting, spending an extra cycle in every period: `(257 - T0PRR)` rather than

> 8-bit prescaler: TPR = 1 x (256 - [T0PRR]) (decimal)

The manual's own sample sets `T0PRR` to 255 for a period of one cycle, where
this gave two. The error was 0.4% at `T0PRR=0` and 100% at `0xFF`.

**Timer 1's 16-bit reload read `T0LR`** -- timer 0's register -- so a 16-bit
timer 1 ran at whatever rate timer 0 happened to be set to.

Also: in 16-bit mode the low half's overflow is not an event ("When the 16-bit
counter is used, the flag is not set also when overflow occurs"), so `T0L` no
longer raises its interrupt there; the carry into the high half only happens
when the high half is running; and the buzzer is told the new period on every
reload rather than only when the timer starts, so a game that writes a note at
a time no longer plays one held note.

**The buzzer sounded whenever timer 1 ran.** The manual's mode table makes the
pulse output a property of the pin, not the timer:

| Mode | Clock | T1LONG | P17FCR | P17DDR |
| --- | --- | --- | --- | --- |
| 0 | Tcyc | 0 | 0 | X |
| 1 | Tcyc | 0 | 1 | 1 |
| 2 | Tcyc, ½Tcyc | 1 | 0 | X |
| 3 | Tcyc, ½Tcyc | 1 | 1 | 1 |

Only modes 1 and 3 reach P17, so timer 1 used as a plain timer drives nothing.
It is gated on those two bits now. The BIOS sets both at boot, so this changes
nothing for anything measured here; it stops a game that routes the pin away
from buzzing anyway.

**Both generators sound.** Mode 1 is the plain 8-bit one:

> Pulse output signal cycle (decimal) = (256 - T1LR setting value) x Tcyc
> "L" level pulse width (decimal) = (T1LC setting value - T1LR setting value) x Tcyc

Mode 3, the 9-to-16 bit generator, repeats that interval `(256 - T1HR)` times
and spends `T1HC` extra ticks low across the repetition:

> total "L" width = ((256 - T1HR) x (T1LC - T1LR) + T1HC) x Tcyc

so the first `T1HC` of the small intervals are one tick longer than the rest.
They are placed in whole intervals rather than smeared evenly across them,
because which interval an extra tick lands in is part of the waveform.
Measured, the duty comes out at 0.5040 against 0.5039 from the formula, and
every low run is one of exactly two lengths -- 85 short and 86 long over a run
of them, with the long share 0.5029 against the 0.5 `T1HC` asks for.

Seeing that at all takes the slow clock. At the usual one a timer tick is a
fifth of a sample, so a one-tick difference is below what 32768Hz can
represent; on the quartz divided by six a tick is six samples wide and the two
run lengths are plain. The half-Tcyc clock (`T1HRUN=0` with `T1LONG=1`), which
ticks twice a cycle, halves the interval as it should.

**The comparator latched at the wrong moment.** `T1LC`/`T1HC` were copied to
the pulse generator on any cycle `ELDT1C` was set. The manual holds it to the
next overflow -- `T1L`'s in 8-bit mode, `T1H`'s in 16-bit -- so a write lands
on an interval boundary instead of part way through one. While the timer is
stopped it is copied straight away, as the manual also says.

**`T0L` can be clocked from the connector.** `T0LEXT` selects an external
signal over the prescaler and `ISL` bit 0 picks the pin -- `P72` is connector
pin 13, `P73` is pin 6 -- which is how one VMU counts pulses from another. It
counts edges on the selected pin now; before, selecting it stopped the counter
dead. That gives timer 0 all four of its modes:

| Mode | T0LONG | T0LEXT | `T0L` | `T0H` |
| --- | --- | --- | --- | --- |
| 0 | 0 | 0 | prescaler | prescaler |
| 1 | 0 | 1 | pin | prescaler |
| 2 | 1 | 0 | prescaler | `T0L` overflow |
| 3 | 1 | 1 | pin | `T0L` overflow |

`T0H` is never clocked from the pin directly in any of them -- in the 16-bit
modes it takes `T0L`'s overflow, which carries the external clock through
already. Measured: 32 edges on `P73` with `T0LR=FC` leave `T0H` at 8.

One difference between the two timers is worth knowing, because the manual
calls it out: timer 0's low overflow flag stays clear in 16-bit mode, and
timer 1's does not.
