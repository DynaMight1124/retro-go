# Image builder development

The [user guide](README.md) covers normal use. This file covers implementation,
packaging and compatibility details for maintainers.

## Packaging and tests

Use a dedicated Python environment with Tk; do not modify ESP-IDF's environment.
Install `requirements-package.txt`, then run `build_windows.ps1 -Python <python>`
on Windows (or invoke PyInstaller with `image_builder.spec`). The resulting single
executable includes Tk, boot assets and their source/licence archives. The script smoke-tests the packaged executable and copies it to
`tools/Tiny386-Image-Builder.exe`. Build intermediates stay in ignored `build/`.
Pass `-ReleasePackage` only when you want the larger distributable ZIP in
`dist/Tiny386-Image-Builder-Windows.zip`: the executable, user README, COPYING,
a complete `image-builder-source.zip`, runtime `licenses/` and SHA256SUMS.txt. Runtime assets are
embedded; the source archive retains all DOS source/licence archives without
duplicating an external assets directory. The release ZIP uses an explicit file
list, so old files in dist do not leak into the package.

From the repository root:

```
python -m unittest discover -s tiny386-go/tests/image_builder -v
```

Tests skip real Tk workflows if Tk/display is unavailable. Packaging verification:
`Tiny386-Image-Builder.exe --check-runtime <new-report.json>` checks Tk construction,
asset hashes and a generated-image round trip, then exits. This is a developer
diagnostic; ordinary use is entirely graphical.

The old optional QEMU boot/memory probes and local toolchain copies were removed
with disposable investigation artifacts. The maintained unit/GUI tests above
remain; real-device boot and memory checks are documented in
[validation notes](../../docs/TESTING.md).

See [boot-asset provenance](assets/README.md) for licences, sources and rebuild
instructions. No user floppy images or commercial game files are distributed.

## DOS memory compatibility

New images default to **XMS enabled, 1024 KiB limit; EMS disabled; Load DOS high
enabled**. Each memory type has independent enable and size controls: 128, 256,
512, 768 or 1024 KiB. These are requested limits, not guaranteed available RAM.
DOS, its shell and memory managers use some memory; the total guest allocation
still defaults to 2 MiB in the `.ini`, independently of disk capacity.

At 2 MiB guest RAM, plan roughly 1 MiB combined for XMS/EMS and split that
budget when both are enabled. DOS and drivers can leave less free. A 4 MiB
guest supports the full requested 1 MiB of each in our tested setup.

Rough guest-RAM planning guide: original ESP32 **2 MiB** with 4 MiB usable PSRAM;
S3 **up to 4 MiB** with 8 MiB PSRAM; P4 **up to 16 MiB** with 32 MiB PSRAM.
These are guides for those RAM capacities, not chip-family guarantees. P4 has
been device-tested at 4 MiB; 16 MiB is an estimate leaving room for emulator
buffers, not a tested maximum. Actual limits depend on device/free PSRAM.
The original ESP32 normally maps only 4 MiB of PSRAM for ordinary allocations,
so a full 4 MiB guest would leave no room there for emulator buffers; see
[Espressif's guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/external-ram.html).

EMS uses bundled JEMM386 5.86 with a fixed reservation. The manager can reduce
the reservation if there is insufficient RAM. A small bundled `XMSCAP.COM`
helper reserves excess free XMS until reboot, so the game sees no more than
the requested XMS limit. It does not create extra memory or alter Retro-Go RAM
placement. If the helper or required EMS page frame fails, automatic game
launch stops at the DOS prompt. Source and licences ship with all helper binaries.

EMS requires an underlying XMS driver. Disabling XMS while enabling EMS means
**zero free XMS available to games**, with the driver retained for EMS. Loading
DOS high also needs that driver. To boot with no memory manager, disable both
XMS/EMS and uncheck Load DOS high. Changing startup preserves memory settings.

Use **Open / update image**, open the `.img`, set its DOS memory controls, then
click **Apply memory settings**. The image and any recognised host manifest are
backed up. Game files and saves remain intact; unrelated CONFIG.SYS/AUTOEXEC.BAT
commands are preserved. Legacy tool images can be upgraded without their host
manifest. Selected limits on legacy images apply when saved; they do not describe
an existing unlimited XMS setup. Unknown or manually changed memory commands
are detected and memory editing is disabled instead of replacing them.

With the bundled BIOS, software-only tests at 2 MiB guest RAM may offer less
than the requested combined limits. For a full 1 MiB of each, a 4 MiB guest is
needed in the tested setup: choose `4M` guest RAM in **Machine settings…** on a
device with enough PSRAM, such as the current S3/P4 test devices. The builder
does not silently increase guest RAM when changing XMS/EMS limits. Original
ESP32 devices with 4 MiB PSRAM may not have enough space for a 4 MiB guest.

EMS selects a D0000-DFFFF guest page frame, verified between the bundled VGA
ROM and 128 KiB system BIOS. Custom replacement BIOS/option ROM layouts are
outside this tested configuration; do not assume that frame is free with them.
EMS compatibility and performance on real devices still require testing.

## Source layout and cleanup

All top-level Python modules are used by the GUI, image service, launchers or
packaged-runtime checks. The spec, packaging script and dependency pins reproduce
the Windows build. Assets include runtime binaries plus their corresponding
sources/licences; keep them together, even where a source archive is not needed
at runtime. No game images, test games, local logs or compiler toolchains belong
in this folder. Generated `build/`, `dist/` and `__pycache__/` are ignored.

The ordinary Python app needs only Python with Tk; PyInstaller dependencies are
for packaging. From this folder, one isolated Windows setup is:

```powershell
py -3.13 -m venv build/package-env
./build/package-env/Scripts/python.exe -m pip install -r requirements-package.txt
powershell -NoProfile -ExecutionPolicy Bypass -File ./build_windows.ps1 -Python ./build/package-env/Scripts/python.exe
```

Packaging is verified using Python 3.13, Tk 8.6 and PyInstaller 6.11.1. For a
portable check, copy only the EXE into a fresh directory and run its
`--check-runtime <absolute-new-report.json>` mode; it must work without adjacent
assets or source files. The automated runtime check covers Tk/tooltip construction,
asset hashes, image creation and settings-only/memory edits. It does not establish
DOS boot or game compatibility on hardware.

Runtime notices in `licenses/` were copied from the tested Python 3.13.17 Windows
installation (LICENSE.txt and bundled documentation license.html), Tk 8.6's
license.terms, and PyInstaller 6.11.1's distribution COPYING.txt. The Python HTML
includes its bundled third-party notices, including OpenSSL. These files are
embedded and shipped in the source/release archives. Review and refresh them
when changing the packaging interpreter or bundled runtime versions.

Normal builds do not create `dist/`. It is optional publication output and can
be deleted after copying/publishing the files you want. The maintained repository
keeps one executable directly under `tools/`, alongside the builder sources,
boot archives and notices; those maintained inputs remain available for Python
users and redistribution. Default disk capacity is 4 MiB; users can choose larger
presets/custom capacity. Disk geometry rounding still applies.
