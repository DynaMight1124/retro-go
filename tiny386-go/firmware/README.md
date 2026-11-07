# Bundled SeaBIOS and SeaVGABIOS

The app embeds `bios.bin` (131,072 bytes) and `vgabios.bin` (39,936 bytes).
They are firmware for the emulated PC, not for the ESP32 itself. Normal users
do not download BIOS files or place them on SD.

## Provenance and corresponding source

- Upstream: https://github.com/coreboot/seabios
- Pinned revision: `81ec9ec0bcf45df11fb7f98339ec9036b546fca0`.
- Complete unmodified upstream source archive:
  `source/seabios-81ec9ec0bcf45df11fb7f98339ec9036b546fca0.tar.gz`.
- Tiny386's supplied `seabios/config` and `seabios/patch` are retained as
  `source/tiny386.config` and `source/tiny386.patch`. The patch modifies floppy,
  reboot and APM handling for Tiny386; no additional SeaBIOS source changes were
  made. `source/resolved.config` records the configuration after olddefconfig.
- The selected VGA target is **Bochs stdvga/VBE**, matching Tiny386's emulated
  VGA hardware; no physical GPU firmware is included.
- `manifest.json` records source/config/patch/licence and binary sizes/SHA-256.

## Licensing and redistribution

SeaBIOS and SeaVGABIOS include work by Kevin O'Connor and other contributors.
Runtime firmware is distributed under GNU LGPL version 3 terms; some build
tools/source files use GNU GPL version 3 or other terms stated in their headers.
Preserve those individual notices. `COPYING.LESSER` and `COPYING` contain the
LGPLv3 and GPLv3 texts; the full source archive preserves copyright notices and
file-specific terms.

When distributing the app binary, provide these notices, the matching source
archive, patches/configuration and rebuild instructions alongside it. Keep the
Retro-Go application source/build instructions available so modified firmware
can be recombined into the application. BIOS files can also be replaced at run
time through explicit `bios` / `vga_bios` descriptor paths; there is no firmware
signature restriction in this adapter. Do not distribute only the ROM blobs
without their corresponding source/licensing material.

## Rebuild

From Linux/WSL with an existing x86 GCC/binutils, make, patch, tar and Python 3:

```sh
sh tiny386-go/firmware/rebuild.sh
python3 tiny386-go/firmware/verify.py
```

The script verifies the pinned source archive, extracts into a new directory
under `firmware/build`, applies Tiny386's patch, configures and builds both ROMs,
and replaces the two embedded binaries plus resolved config. It performs no
downloads and installs no dependencies. ESP-IDF builds consume the committed
ROMs directly; users building the app need not have the SeaBIOS toolchain.

The shipped pair was built with GCC 9.4.0 (Ubuntu 20.04 package
`9.4.0-1ubuntu1~20.04.2`) and GNU binutils 2.34. Version strings are pinned via
`.version` and `EXTRAVERSION=-tiny386-go`, avoiding build timestamps and hostnames.
Other compilers may produce different bytes; retain the resulting source,
configuration and updated manifest if publishing a different pair. The verifier
checks the shipped pair; it deliberately rejects unrecorded replacements.
Two independent builds with the documented toolchain produced matching binary
and resolved-configuration hashes, verified against the retained manifest.

## Memory behavior

ESP-IDF embeds the ROMs into flash-mapped `.rodata`. The loader copies them
directly into the existing guest RAM at boot/reset (system BIOS ends at
0x100000, VGA BIOS starts at 0xC0000). There is no additional persistent internal
RAM buffer or heap allocation for the embedded ROMs. Existing guest RAM and
framebuffer allocations are unchanged. Small loader code/metadata costs are
reported separately by the application linker map.

## Overrides

Omit both BIOS keys to use the embedded pair. An explicit path overrides only
that firmware image; relative paths resolve beside the descriptor. Missing or
invalid explicit overrides report an error instead of silently changing BIOS.
For example:

```ini
[pc]
hda = game.img
; Only override this for testing a custom firmware build:
bios = custom-bios.bin
vga_bios = custom-vgabios.bin
```

ROM integrity/build checks do not prove guest boot compatibility. Validate DOS
and games on crokpocket before declaring this pair hardware-tested.
