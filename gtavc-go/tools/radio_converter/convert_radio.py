#!/usr/bin/env python3
"""Convert Vice City ADF stations to numbered Retro-Go radio WAVs (Python 3.9+)."""
import argparse
import csv
import math
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import wave

ROOT = Path(__file__).resolve().parent
XOR_TABLE = bytes(value ^ 0x22 for value in range(256))
RATE = 8000
MAX_STATIONS = 15


def decode_adf(source, target):
    """Decode the XOR wrapper without loading the full station into memory."""
    with Path(source).open('rb') as src, Path(target).open('wb') as dst:
        for block in iter(lambda: src.read(1024 * 1024), b''):
            dst.write(block.translate(XOR_TABLE))


def find_inputs(folder):
    files = sorted((p for p in Path(folder).iterdir()
                    if p.is_file() and p.suffix.casefold() == '.adf'),
                   key=lambda p: (p.name.casefold(), p.name))
    if not files:
        raise ValueError(f'No .adf files found in {folder}')
    if len(files) > MAX_STATIONS:
        raise ValueError(f'Found {len(files)} ADFs; the engine supports at most {MAX_STATIONS} tracks. '
                         'Select up to 15 files in import.')
    return files


def find_ffmpeg(explicit=None):
    if explicit:
        candidate = Path(explicit).expanduser()
        found = str(candidate.resolve()) if candidate.is_file() else shutil.which(explicit)
    else:
        candidate = ROOT / ('ffmpeg.exe' if os.name == 'nt' else 'ffmpeg')
        found = str(candidate) if candidate.is_file() and os.access(candidate, os.X_OK) else shutil.which('ffmpeg')
    if not found:
        raise FileNotFoundError('FFmpeg not found. Place a native ffmpeg binary beside this script, '
                                'install it on PATH, or pass --ffmpeg PATH.')
    return found


def check_outputs(folder, count, overwrite):
    surplus = [f'{number:02d}.wav' for number in range(count + 1, MAX_STATIONS + 1)
               if (Path(folder) / f'{number:02d}.wav').exists()]
    if surplus:
        raise ValueError('Export contains extra old stations: ' + ', '.join(surplus) +
                         '. Use a fresh export folder or remove those files before converting.')
    if overwrite:
        return
    outputs = [Path(folder) / f'{number:02d}.wav' for number in range(1, count + 1)]
    outputs.append(Path(folder) / 'stations.csv')
    existing = [p.name for p in outputs if p.exists()]
    if existing:
        raise FileExistsError('Export already contains ' + ', '.join(existing) +
                              '. Use --overwrite to replace these outputs.')


def validate_wav(path, seconds):
    if Path(path).stat().st_size > 0x7fffffff:
        raise ValueError('WAV exceeds the 32-bit radio reader file-size limit')
    with wave.open(str(path), 'rb') as wav:
        if (wav.getnchannels(), wav.getframerate(), wav.getsampwidth(), wav.getcomptype()) != (1, RATE, 2, 'NONE'):
            raise ValueError('FFmpeg output is not 8000 Hz mono 16-bit PCM')
        frames = wav.getnframes()
        if frames <= 0 or (seconds is not None and frames > math.ceil(seconds * RATE)):
            raise ValueError('FFmpeg output is empty or exceeds the requested duration')
        # Check the data is actually present, rather than trusting the header.
        remaining = frames
        while remaining:
            block = wav.readframes(min(remaining, RATE))
            count = len(block) // 2
            if not count or len(block) % 2:
                raise ValueError('Truncated PCM output')
            remaining -= count
    return frames / RATE


def convert_station(source, target, ffmpeg, seconds, overwrite=False):
    target = Path(target)
    if target.exists() and not overwrite:
        raise FileExistsError(f'{target} already exists; use --overwrite')
    target.parent.mkdir(parents=True, exist_ok=True)
    # Keep output staging on the destination filesystem for checked publication.
    with tempfile.TemporaryDirectory(prefix='.radio-', dir=target.parent) as temp:
        mp3 = Path(temp) / 'decoded.mp3'
        wav = Path(temp) / 'station.wav'
        decode_adf(source, mp3)
        command = [str(ffmpeg), '-nostdin', '-hide_banner', '-loglevel', 'error', '-y',
                   '-i', str(mp3), '-map', '0:a:0', '-vn']
        if seconds is not None:
            command += ['-t', str(seconds)]
        command += ['-ar', str(RATE), '-ac', '1', '-c:a', 'pcm_s16le',
                    '-map_metadata', '-1', str(wav)]
        result = subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
                                text=True, errors='replace', check=False)
        if result.returncode:
            raise RuntimeError(f'FFmpeg failed for {Path(source).name}: {result.stderr.strip()[-2000:]}')
        duration = validate_wav(wav, seconds)
        if target.exists() and not overwrite:
            raise FileExistsError(f'{target} appeared during conversion; not replacing it')
        os.replace(wav, target)
    return duration


def write_manifest(folder, rows):
    with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', newline='',
                                     dir=folder, prefix='.stations-', suffix='.csv', delete=False) as tmp:
        path = Path(tmp.name)
        try:
            writer = csv.writer(tmp)
            writer.writerow(['output', 'source', 'seconds', 'sample_rate', 'channels', 'bits'])
            writer.writerows(rows)
        except BaseException:
            tmp.close()
            path.unlink(missing_ok=True)
            raise
    try:
        os.replace(path, Path(folder) / 'stations.csv')
    finally:
        path.unlink(missing_ok=True)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--import-dir', type=Path, default=ROOT / 'import')
    parser.add_argument('--export-dir', type=Path, default=ROOT / 'export')
    parser.add_argument('--ffmpeg', help='Native FFmpeg executable path or command')
    duration = parser.add_mutually_exclusive_group()
    duration.add_argument('--seconds', type=float, default=1800, help='Maximum duration in seconds (default: 1800 / 30 minutes)')
    duration.add_argument('--minutes', type=float, help='Maximum duration in minutes, for example --minutes 10')
    duration.add_argument('--full-length', action='store_true', help='Convert complete stations without trimming')
    parser.add_argument('--overwrite', action='store_true', help='Replace matching WAVs and stations.csv')
    args = parser.parse_args(argv)
    if args.full_length:
        args.seconds = None
    elif args.minutes is not None:
        args.seconds = args.minutes * 60
    if args.seconds is not None and (not math.isfinite(args.seconds) or args.seconds < 1 / RATE):
        parser.error('--seconds/--minutes must specify a finite, positive audio duration')
    try:
        files = find_inputs(args.import_dir)
        ffmpeg = find_ffmpeg(args.ffmpeg)
        args.export_dir.mkdir(parents=True, exist_ok=True)
        check_outputs(args.export_dir, len(files), args.overwrite)
        length = 'Full length' if args.seconds is None else f'Up to {args.seconds / 60:g} minutes per station'
        print(f'{len(files)} station(s); {length}; 8000 Hz mono, 16-bit PCM.', flush=True)
        rows = []
        for number, source in enumerate(files, 1):
            output = args.export_dir / f'{number:02d}.wav'
            print(f'[{number}/{len(files)}] {source.name} -> {output.name}', flush=True)
            duration = convert_station(source, output, ffmpeg, args.seconds, args.overwrite)
            rows.append([output.name, source.name, f'{duration:.3f}', RATE, 1, 16])
        write_manifest(args.export_dir, rows)
        print(f'Exported {len(rows)} station(s) to {args.export_dir}')
        return 0
    except (OSError, ValueError, RuntimeError, wave.Error, EOFError) as error:
        print(f'Error: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
