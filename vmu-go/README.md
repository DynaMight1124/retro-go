# VMU-Go

VMU-Go brings Sega Dreamcast Visual Memory Unit (VMU) mini-games to Retro-Go handhelds. It emulates the VMU itself, including its little LCD and buzzer. The default **VMU Green** palette and **Medium** pixel grid give the screen a classic VMU look.

## Get started

1. Use a Retro-Go firmware build that includes VMU-Go.
2. Copy your VMU mini-games to `roms/vmu/` on your SD card. A plain `.vms` game is the easiest way to start. You can also use `.dci` files and full 128 KiB VMU flash dumps (`.bin`). Zip is also supported with one rom per zip.
3. Open **Dreamcast VMU** in the Retro-Go launcher and select a game.


## Play and save

| Control | What it does |
| --- | --- |
| D-pad, A, B | VMU directions and game buttons |
| MENU | Open the Retro-Go game menu to save or load a state, reset, or exit |
| OPTION | Open Retro-Go options |
| START, SELECT | VMU MODE and SLEEP, respectively, when a BIOS is active |

Under **Emulator options**, choose an **LCD palette** and set **Pixel grid** to **Off**, **Low**, **Medium**, or **High**. The choices take effect when you return to the game and are remembered for the next launch.

The picture fills the screen by default. You can choose a different **Scaling** mode in the Retro-Go options menu, and VMU-Go will remember your choice.

VMU-Go saves changes that games make to VMU flash separately from the original game file. Use **Exit** from the Retro-Go game menu to return to the launcher and give VMU-Go a chance to finish saving. Save states capture the current VMU session, including its flash contents, so loading an older state can also roll back in-game progress.

## Optional BIOS

Enable the BIOS if you want the original VMU menu, clock, and file manager. Copy your VMU BIOS dump to `retro-go/bios/vmu_bios.bin` on the SD card, turn on **BIOS (restart)** in **Emulator options**, then exit and relaunch the game.

At the VMU menu, press **START** (MODE) to cycle to **Game**, then press **A** to open it. **Icon row (BIOS only)** adds the VMU status symbols when a BIOS loads successfully. Changes to the icon row also take effect after relaunch. If the BIOS is missing or invalid, VMU-Go starts in its usual BIOS-free mode.

## Credits

Thanks to **Mahmoud Jaoune**, the original VeMUlator developer, and to the contributors who brought VeMUlator to libretro and improved the fork used here. Thanks also to the Retro-Go developers and everyone who tested the port on handheld hardware.

See the [VeMUlator notes](components/vemulator/README.md) for upstream details and the [VeMUlator license](components/vemulator/COPYING) for license terms.
