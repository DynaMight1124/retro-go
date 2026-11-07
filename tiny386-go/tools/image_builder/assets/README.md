# Boot asset provenance and rebuilding

These are unmodified FreeDOS kernel 2043 (8086, FAT12/16) and FreeCOM 0.85a
(English, Borland plain editor build). Both use GPL version 2 or later; see
COPYING.KERNEL.txt, COPYING.FREECOM.txt and the complete release source archives.
Upstream copyright notices remain inside those archives. No user game is included.

`manifest.json` pins every binary/archive with SHA256 and upstream release URLs.
Source archives are distributed alongside the binaries, including inside the
Windows executable. Extracting the PyInstaller archive is not required to obtain
source: the same `assets/` directory ships in the repository/source distribution.
Windows releases provide this directory in `image-builder-source.zip`; keep that
archive with the executable when distributing a package.

Exact inputs and extraction:

- `KERNEL.SYS`: `bin/KERNEL.SYS` from `ke2043_86f16.zip`.
- `COMMAND.COM`: `plainedt/command.com` from `bc-English.zip`; no XMS manager required.
- `boot.asm`: `SOURCE/ke2043/boot/boot.asm` from `ke2043s.zip`, unchanged.
- Kernel corresponding source: `ke2043s.zip`; instructions in `SOURCE/ke2043/docs/build.txt` and `build.bat`.
- FreeCOM corresponding source: `bc-freecom-source.zip`; `freecom/build.bat`, `buildall.bat` and `docs/` describe Borland/OpenWatcom builds and configuration. The binary selection is the Borland plain editor English variant.
- `mbr.asm`: local MIT-licensed source for a single-partition LBA boot loader; requires BIOS INT 13h extensions, supplied by SeaBIOS.

Rebuild the boot sectors with NASM 2.16.03 (a development-only tool):

```
nasm -dISFAT16 boot.asm -o fat16.bin
nasm mbr.asm -o mbr.bin
```

The installer preserves bytes 3–61 of the formatted FAT16 volume sector and
copies only its jump and code/signature from `fat16.bin`. It preserves the MBR
partition table and copies only bytes 0–445 from `mbr.bin`. Both sectors are 512
bytes, ending in 55 AA. Update manifest hashes deliberately after changing assets;
normal creation verifies hashes and performs no downloads.

FreeDOS upstream releases:
https://github.com/FDOS/kernel/releases/tag/ke2043
https://github.com/FDOS/freecom/releases/tag/com085a

## HIMEMX 3.38

Pinned upstream release: https://github.com/Baron-von-Riedesel/HimemX/releases/tag/v3.38

`HimemX338.zip` is the unmodified release archive containing the executable,
assembly source and makefiles. `HIMEMX.EXE` is its `HimemX.exe`;
`README.HIMEMX.txt` is its original README. SHA-256 hashes and download URL
are recorded in manifest.json and checked offline before image creation.
The upstream README states that HIMEMX changes are public domain, while the
inherited FreeDOS HIMEM copyrights and GPL and/or Artistic licensing apply.
We distribute under the GPL option; GPL version 2 text is in COPYING.HIMEMX.txt.
Keep this archive and notices with binary distributions. This is a separately
verified upstream driver, not the legacy HIMEM extracted from a user test image.

## JEMM386 5.86 and the XMS limit helper

`JEMM386.EXE` and `README.JEMM.txt` are unmodified files from upstream
[`JemmB_v586.zip`](https://github.com/Baron-von-Riedesel/Jemm/releases/download/v5.86/JemmB_v586.zip).
The binary archive SHA256 is94838a1836f94d95dcf84d250f7968dfca2eb6b5cfa7ac664d6e01f22f0d296e.
`JemmS_v586.zip` is the corresponding tagged
[source archive](https://github.com/Baron-von-Riedesel/Jemm/archive/refs/tags/v5.86.zip),
including upstream build instructions and notices. Jemm is partly under the
Artistic licence; original text is retained as `COPYING.JEMM.txt` and inside
both archives. These sources/notices are included in the release source archive.
The builder installs only JEMM386, not JEMMEX or unrelated tools from the archive.

`XMSCAP.COM` is built from local MIT-licensed `xmscap.asm` using NASM2.16.03
(full licence in `COPYING.XMSCAP.txt`):

```
nasm -f bin xmscap.asm -o XMSCAP.COM
```

It reserves free XMS above the selected0-1024KiB cap via the real XMS allocator,
keeping those handles in a small DOS resident process until reboot. No interrupt
hooks, API-report spoofing, paging or disk access. Failure frees handles already
allocated by this invocation and returns a nonzero DOS status. An optional EMS
argument also checks manager status, a nonzero page frame and available pages.
Generated AUTOEXEC stops game launch on failure. Asset/source hashes are pinned
in manifest.json and verified offline before creation or memory editing.
