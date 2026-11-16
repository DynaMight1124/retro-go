# Vice City Radio Converter

Turn your original Vice City `.adf` radio files into WAV stations for gtavc-go.

**The standard run needs no options:** put your files in `import`, run the
script, and collect the converted stations from `export`. By default, each
station contains up to **30 minutes** of audio.

## What you need

- **Python 3.9 or newer:** [download Python](https://www.python.org/downloads/).
  On Windows, enable the installer option to add Python to PATH if offered.
- **FFmpeg:** [download FFmpeg](https://ffmpeg.org/download.html).
  Use a build for your operating system. No extra Python packages are needed.
- Your original Vice City radio files ending in `.adf`.

On **Windows**, put `ffmpeg.exe` beside `convert_radio.py` if it is not already
there. On **Linux or macOS**, install `ffmpeg` so it is available on PATH, or put
an executable named `ffmpeg` beside the script. A Windows `.exe` does not run
natively on Linux/macOS. A local Linux/macOS binary may need `chmod +x ffmpeg`.

Your folder should look like this:

```text
radio_converter/
  convert_radio.py
  ffmpeg.exe          Windows; Linux/macOS use their native ffmpeg instead
  import/
    EMOTION.adf
    FLASH.adf
    ...
  export/
```

## Quick start

1. Copy up to **15 `.adf` files** into `import`.
2. Open a terminal in the `radio_converter` folder.
3. Run:

   **Windows:**
   ```text
   python convert_radio.py
   ```
   If Windows uses the Python launcher, `py -3 convert_radio.py` also works.

   **Linux/macOS:**
   ```sh
   python3 convert_radio.py
   ```
4. Wait for the final `Exported ... station(s)` message. There are no conversion
   prompts. Progress shows which source is being converted.
5. Copy the numbered WAV files from `export` into your game's `radio` folder.

You can also run the script using its full path from another directory. Its
standard `import` and `export` folders always stay beside the script.

Original ADF files are left untouched. The script creates a temporary MP3 while
converting and removes it afterward; you do not need to convert ADFs yourself.

## Putting the stations on your SD card

For the recommended game layout:

```text
roms/gtavc/assets/radio/
  01.wav
  02.wav
  ...
```

For an existing flat installation, use `roms/gtavc/radio/` instead.

Copy **the WAV files themselves**, not the export folder. Keep the numbered
filenames: the game recognises `01.wav` through `15.wav`. You do not need to copy
`stations.csv` or `.gitkeep` to the SD card.

Files are numbered alphabetically by source filename. `stations.csv` tells you
which original station corresponds to each number. Nine ADFs produce nine WAVs;
you do not need all fifteen slots and the script does not duplicate stations.

The format is fixed to **8 kHz, mono, 16-bit PCM WAV**, matching the tested radio
streaming setup. A source shorter than the selected duration stays shorter;
extra silence is not added. Trimming keeps the beginning of the source station.

Thirty minutes across nine stations uses approximately **259 MB / 247 MiB**.
The game starts stations at random positions, so you will not always hear the
beginning or a full thirty minutes on each car entry.

## Optional settings

The examples below use `python`; use `python3` on Linux/macOS if needed.

| What you want | Command |
| --- | --- |
| Standard 30-minute stations | `python convert_radio.py` |
| Ten-minute stations | `python convert_radio.py --minutes 10` |
| Two-minute test clips | `python convert_radio.py --seconds 120` |
| Complete stations, without trimming | `python convert_radio.py --full-length` |
| Regenerate existing numbered outputs | `python convert_radio.py --overwrite` |
| Keep a separate full-length set | `python convert_radio.py --full-length --export-dir export/full` |
| See every option | `python convert_radio.py --help` |

Choose only one duration option: `--minutes`, `--seconds` or `--full-length`.
You can combine it with folder, FFmpeg and overwrite options. For example:

```sh
python convert_radio.py --minutes 20 --export-dir export/20min
```

To choose another input/output directory or a specific FFmpeg executable:

```text
python convert_radio.py --import-dir "D:/My Radio" --export-dir "D:/Converted Radio" --ffmpeg "C:/Tools/ffmpeg.exe"
```

Linux/macOS paths work the same way, for example `--ffmpeg /usr/local/bin/ffmpeg`.
Custom relative paths are relative to your terminal's current directory; quote
paths containing spaces.

## Running it again

Existing WAVs and the station list are protected by default. To intentionally
replace them, run:

```sh
python convert_radio.py --overwrite
```

If you remove inputs and an old export contains extra numbered stations, the
script stops rather than silently mixing old stations into the new set. Remove
the unwanted WAVs or choose a fresh output directory with `--export-dir`.
Nothing in `import` is deleted. If a station fails, its previous WAV is left
intact; earlier stations may already have completed. Fix the error and finish a
successful run before copying the new set to your SD card.

## Troubleshooting

- **Python is not recognised:** install Python, reopen your terminal, then try
  `py -3` on Windows or `python3` on Linux/macOS.
- **FFmpeg not found:** place the correct native executable beside the script,
  add it to PATH, or provide its location with `--ffmpeg`.
- **No ADF files found:** check `import` contains files ending in `.adf` rather
  than a ZIP archive or another folder. Create `import` beside the script if
  you copied only the script itself.
- **More than 15 inputs:** select up to fifteen for one export set.
- **Export already exists:** use `--overwrite` or a different output folder.
- **Conversion failed:** check that the input is an original Vice City ADF and
  read the FFmpeg error shown. Partial current output is not published.
- **Music absent in game:** confirm the WAV filenames/location above and enable
  radio in the game's options. Use a firmware version with streamed radio.

Full-length files use the same streaming buffer as shorter files, but take more
SD space. This reader supports WAVs below roughly **2 GiB per file**. Longer
tracks do not prevent SD-related music gaps while map loading takes priority.
