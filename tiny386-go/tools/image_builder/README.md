# Tiny386 DOS Image Builder

Turn a folder of DOS game files into a bootable game disk for Tiny386-Go on
Retro-Go. The builder supplies FreeDOS, starts your selected game automatically
and creates the matching emulator settings file. No disk mounting or DOS boot
floppy is needed.

You supply the game files. Use an already installed/unpacked DOS game; the
builder does not run installers or guarantee game compatibility.

## Run the builder

### Windows - standalone application

Open `tools/Tiny386-Image-Builder.exe` in the project download, or
`Tiny386-Image-Builder.exe` from a separate Windows package. Python, the graphical
interface and all DOS boot assets are included. No Python installation,
administrator access or internet connection is needed to create images.

The executable runs independently; `dist/` is not needed. Optional release
packages also include this guide, runtime notices in `licenses/` and `image-builder-source.zip`. Those files
provide documentation and source; they are not required beside the executable
for it to run. The executable is unsigned.

### Python - Windows, Linux and macOS

Use Python 3.11 or later with Tkinter/Tcl/Tk. Download/extract the source package
and keep its folder structure intact, including `assets/`. From its
`tools/image_builder/` folder, run:

```sh
python launch_gui.py
```

Use `python3` if that is your Python command. There are no additional Python
packages needed for normal use. Some Linux distributions require `python3-tk`.
Windows GUI use is tested; Linux and macOS GUI validation remains pending.

## Create your first game image

1. In **Create game image**, choose the **Game folder** containing your unpacked
   game and click **Scan folder**.
2. Choose the **Startup file**: the game's `.EXE`, `.COM` or `.BAT`. Choose the
   actual game rather than its installer or setup utility. Leave arguments blank
   unless the game needs them.
3. Choose a **Disk capacity**. Presets run from 4 to 64 MiB; 4 MiB is the default.
   Custom sizes are available up to 2048 MiB. Capacity must fit the game, DOS and
   space for saves; it does not change emulated RAM.
4. Leave the default memory settings initially. **Machine settings...** lets you
   change guest RAM, keyboard mappings and other compatibility settings. Hover
   over a setting for a short explanation.
5. Choose an **Output folder** outside the game folder and an **Image name**.
   Click **Check space**, then **Create image**.
6. Copy the resulting `.img` and `.ini` together to `roms/tiny386/` on your SD
   card. Select the `.ini` under **Tiny 386** in the Retro-Go launcher.

For an image named `MyGame`, the output is:

| File | What to do with it |
| --- | --- |
| `MyGame.img` | Copy to the SD card; contains DOS, your game and its saves. |
| `MyGame.ini` | Copy beside the image; contains machine settings and control defaults. |
| `MyGame.json` | Keep on your computer; records how the image was built. |

The game starts after DOS boots. If it exits to a DOS prompt, type `RUN` using
the emulator keyboard to start it again.

## Memory settings

Start with **XMS enabled, 1024 KiB**, **EMS disabled**, **Load DOS high enabled**
and **2M guest RAM**. Enable EMS only if a game needs it. Each XMS/EMS limit can
be set from 128 to 1024 KiB.

- With **2M guest RAM**, allow roughly 1 MiB combined for XMS and EMS. Split
  that allowance if you enable both; DOS and drivers also consume memory.
- **4M guest RAM** supports a full requested 1 MiB of each in our tested setup,
  provided the handheld has enough free PSRAM.
- As a rough starting guide: **2M** on original ESP32 devices with 4 MiB usable
  PSRAM, **up to 4M** on S3 devices with 8 MiB PSRAM, and **4M** as a tested P4
  starting point. P4 devices with 32 MiB PSRAM may have room for larger guests,
  but larger allocations are not tested limits. Device RAM varies.

Guest RAM is changed in **Machine settings...**; changing a disk or XMS/EMS size
will not increase it automatically. Limits are requests, and DOS may report less
available memory. CPU generation and VGA memory are compatibility settings;
leave their defaults unless a game requires a change.

The builder's controller settings become the game's INI defaults. You can also
remap controls in Tiny386-Go's menu. Existing saved menu overrides take precedence;
use **Reset to INI defaults** there to use new builder mappings.

## Open or update an existing image

Close the emulator before editing. Keep a separate backup of valuable saves.

1. Choose **Open / update image** and **Open image...**.
2. Browse or extract files, use **Add / replace file...**, or **Change startup...**.
3. To change DOS memory, adjust its controls and click **Apply memory settings**.
   Use **Machine settings...** to change the matching INI's guest RAM or bindings.
4. Copy the updated files back to the SD card. Copy an updated `.ini` as well as
   the `.img` when both changed; machine-only edits need only the new `.ini`.

Supported updates preserve unrelated files and in-game saves. Replaced files get
`.bak` backups, refreshed on subsequent updates. Keep the matching host `.json`
if available, although it is not required on the handheld.

Editing is intended for images created by this tool. Arbitrary floppy images,
FAT12/FAT32 images and other layouts cannot be converted. Some supported FAT16
images can be inspected but cannot safely be updated.

## Common problems

| Problem | What to try |
| --- | --- |
| Invalid DOS filename | Use DOS 8.3 names: up to eight characters plus a three-character extension, including folder names. Work on a copy of the game. |
| Not enough disk space | Choose a larger image, leaving room for DOS and saves; ensure the computer has room for temporary files and backups too. |
| Not enough game memory | Check XMS/EMS requirements and guest RAM. Increase guest RAM only if your handheld has enough PSRAM. |
| Game does not start | Check the startup file and game-specific setup; try another game to distinguish image setup from emulator compatibility. |
| Keyboard mapping seems unchanged | Reset the game's menu overrides to INI defaults after copying a changed descriptor. |
| Update controls unavailable | The image or INI may not match a supported layout. Create a new image from the original game folder instead. |

DOS shell operators and variable expansion are not accepted in startup arguments.
If a build is interrupted and a `.tiny386-recovery-*` folder remains in the output
folder, keep it: it may contain the previous matching files. Restore those as a
set before retrying.

## Credits, licences and source

Thanks to Chunhui He for [Tiny386](https://github.com/hchunhui/tiny386), the
Retro-Go contributors and the FreeDOS, FreeCOM, HIMEMX and Jemm developers.
No commercial DOS or game files are supplied.

The project includes the builder's Python source and bundled DOS sources in
`tools/image_builder/`. Optional Windows packages include the builder's Python
source and the bundled DOS components' source archives and licences in `image-builder-source.zip`.
See its `tools/image_builder/assets/README.md` for exact versions and provenance.
In a source checkout, see [boot asset notices](assets/README.md).
Keep the source archive and notices with redistributed packages. The bundled
Python, Tcl/Tk and PyInstaller notices are also provided in `licenses/`.

Maintainers: [build, packaging and test instructions](DEVELOPMENT.md).
