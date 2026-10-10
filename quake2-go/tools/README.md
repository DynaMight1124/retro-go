# Quake II music converter

Output is **22,050 Hz, stereo, signed 16-bit PCM WAV** for `quake2-go`. Requires Python 3.8+ and FFmpeg.

## Conversion

Put `ffmpeg.exe` (Windows), or an executable named `ffmpeg` (Linux), in the
`ffmpeg/` folder beside `convert_music.py`. Alternatively pass `--ffmpeg PATH`
or `--ffmpeg ffmpeg` to use your PATH installation.

Put your original MP3/OGG files in `music/` beside the script. Supported names
include `track02.mp3`, `track002.ogg`, and `02.ogg`. Track numbers 02 through
255 are preserved, not renumbered.

From the repository root:

```text
python quake2-go/tools/convert_music.py
```

The default scan starts beside the script, regardless of your working directory.
Each source folder gets a `wav/` output folder. Names are normalised to
`track02.wav`, `track03.wav`, etc. Duplicate tracks in one folder prefer OGG.
Originals and existing outputs are preserved. Use `--overwrite` to replace
existing WAVs after successful conversion and validation; failure preserves
any previous usable output.

Optional source/output overrides:

```text
python quake2-go/tools/convert_music.py /path/to/source --output /path/to/wav
```

Subfolder structure is retained beneath `--output`. Generated `wav` folders,
the explicit output directory, FFmpeg and directory links are excluded from
input scanning. Keep different soundtrack versions in separate source folders.

## SD card installation

Copy the resulting WAV files directly to:

```text
roms/quake2/music/track02.wav
roms/quake2/music/track03.wav
...
```

Do not copy the enclosing `wav/` folder. Playback also accepts `02.wav`,
`03.wav`, etc., but prefers a usable `trackNN.wav` file.

Enable **CD music** in Quake II's Options menu. Music volume defaults to 50%.
`s_musicvolume` in `/retro-go/config/quake2/config.cfg` controls it independently
of the sound-effects slider. Retro-Go's volume applies to both.

Missing, malformed or unsupported tracks leave the game running without music.
Playback requires the PCM format above; MP3, OGG, 44,100 Hz WAV and compressed
WAV cannot be played directly.

## FFmpeg alone

Create the destination folder first. Example from the repository root on Windows:

```bat
quake2-go\tools\ffmpeg\ffmpeg.exe -n -i "quake2-go\tools\music\track02.mp3" -map 0:a:0 -vn -ac 2 -ar 22050 -c:a pcm_s16le -map_metadata -1 "quake2-go\tools\music\wav\track02.wav"
```

On Linux use your native FFmpeg executable with the same arguments. `-n`
preserves an existing output. The Python tool additionally handles whole
folders, normalises names, validates output and safely replaces files.

Local FFmpeg executables, source soundtracks and generated audio are ignored
by Git and are not distributed with the port.

PAKs may also be placed directly in `roms/quake2/baseq2`. In that layout, place
WAV tracks in `roms/quake2/baseq2/music/`. The selected PAK directory determines
both the game-data and music paths; `baseq2/` is optional.
