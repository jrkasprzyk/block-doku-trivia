#!/usr/bin/env python3
"""
scripts/convert_audio.py — batch-convert audio assets using ffmpeg.

Usage examples:
  # Convert all WAV files under assets/ to OGG (overwrite existing):
  python scripts/convert_audio.py --from-ext wav --to-ext ogg --overwrite

  # Convert a single file:
  python scripts/convert_audio.py --file "assets/bg.wav" --to-ext ogg

Requirements: `ffmpeg` must be on PATH.
"""

from pathlib import Path
import argparse
import shutil
import subprocess
import sys
import os
import re


def find_ffmpeg(explicit_path: str = None):
    """Locate an ffmpeg executable.

    Search order:
    1. `explicit_path` (if provided)
    2. env vars `FFMPEG`, `FFMPEG_PATH`, `FFMPEG_BIN`
    3. `shutil.which('ffmpeg')` (PATH)
    4. common virtualenv locations (`sys.prefix` / .venv / venv)
    5. common Windows / Chocolatey locations
    6. chocolatey lib tree scan (limited)

    Returns the executable path or None.
    """
    # 1) explicit path provided by user
    if explicit_path:
        p = Path(explicit_path)
        if p.exists() and p.is_file():
            return str(p)
        if p.exists() and p.is_dir():
            for name in ("ffmpeg.exe", "ffmpeg"):
                cand = p / name
                if cand.exists() and cand.is_file():
                    return str(cand)

    # 2) environment variables
    for ev in ("FFMPEG", "FFMPEG_PATH", "FFMPEG_BIN"):
        val = os.environ.get(ev)
        if val:
            p = Path(val)
            if p.exists() and p.is_file():
                return str(p)
            if p.exists() and p.is_dir():
                for name in ("ffmpeg.exe", "ffmpeg"):
                    cand = p / name
                    if cand.exists() and cand.is_file():
                        return str(cand)

    # 3) PATH lookup
    w = shutil.which("ffmpeg")
    if w:
        return w

    # 4) virtualenv / venv locations
    prefix = Path(sys.prefix)
    cand_dirs = [prefix / "Scripts", prefix / "bin", prefix]
    cand_dirs += [Path('.venv') / 'Scripts', Path('venv') / 'Scripts', Path('venv') / 'bin']

    # 5) common locations (Windows + Unix)
    cand_dirs += [Path('C:/ProgramData/chocolatey/bin'),
                  Path('C:/ProgramData/chocolatey/lib/ffmpeg/tools'),
                  Path('C:/Program Files/ffmpeg/bin'),
                  Path('C:/ffmpeg/bin'),
                  Path('/usr/local/bin'),
                  Path('/usr/bin'),
                  Path('/snap/bin')]

    for d in cand_dirs:
        try:
            if not d.exists():
                continue
        except Exception:
            continue
        for name in ("ffmpeg.exe", "ffmpeg"):
            p = d / name
            if p.exists() and p.is_file():
                return str(p)

    # 6) chocolatey lib scan (targeted, not a full-drive search)
    choco_base = Path('C:/ProgramData/chocolatey/lib')
    if choco_base.exists():
        try:
            for p in choco_base.rglob('ffmpeg.exe'):
                if p.exists() and p.is_file():
                    return str(p)
        except Exception:
            pass

    return None


def convert_file(ffmpeg, src: Path, dst: Path, to_ext: str, quality: int, bitrate: str, dry_run: bool):
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists():
        return False, "exists"
    cmd = [ffmpeg, "-hide_banner", "-loglevel", "error", "-y", "-i", str(src)]
    if to_ext.lower() == "ogg":
        cmd += ["-c:a", "libvorbis", "-q:a", str(quality)]
    elif to_ext.lower() in ("mp3", "mpeg"):
        cmd += ["-c:a", "libmp3lame", "-b:a", bitrate]
    else:
        # default: let ffmpeg choose codecs (copy may fail between containers)
        cmd += ["-c:a", "copy"]
    cmd += [str(dst)]

    if dry_run:
        print("DRY-RUN:", " ".join(cmd))
        return True, None

    try:
        subprocess.run(cmd, check=True)
        return True, None
    except subprocess.CalledProcessError as e:
        return False, str(e)


def to_snake_case(name: str) -> str:
    """Convert a filename (without extension) to snake_case."""
    # Insert underscores for camelCase/PascalCase boundaries, replace non-alphanum
    # with underscores, collapse repeats, and lower-case the result.
    s = re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', name)
    s = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1_\2', s)
    s = re.sub(r'[^0-9a-zA-Z]+', '_', s)
    s = s.lower()
    s = re.sub(r'_+', '_', s).strip('_')
    return s


def scan_files(src_dir: Path, from_ext: str):
    ext = from_ext.lower().lstrip('.')
    return [p for p in src_dir.rglob("*") if p.is_file() and p.suffix.lower().lstrip('.') == ext]


def main():
    parser = argparse.ArgumentParser(description="Batch-convert audio files using ffmpeg")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--file", help="single file to convert (path)")
    group.add_argument("--src", default="assets", help="source directory to scan (default: assets)")

    parser.add_argument("--from-ext", default="wav", help="source extension (default: wav)")
    parser.add_argument("--to-ext", default="ogg", help="destination extension: ogg|mp3|wav (default: ogg)")
    parser.add_argument("--quality", type=int, default=5, help="Vorbis quality for OGG (0-10, default 5)")
    parser.add_argument("--bitrate", default="192k", help="bitrate for MP3 (default 192k)")
    parser.add_argument("--overwrite", default=False, action="store_true", help="overwrite existing destination files")
    parser.add_argument("--dry-run", action="store_true", help="print ffmpeg commands without running them")
    parser.add_argument("--ffmpeg", help="explicit path to ffmpeg executable (optional)")

    parser.add_argument("--convert-filenames", action="store_true", help="convert output filenames to snake_case")

    # Show help and exit if no arguments were provided (safer than running with
    # implicit defaults that may perform large batches).
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(0)

    args = parser.parse_args()

    ffmpeg = find_ffmpeg(args.ffmpeg)
    if not ffmpeg:
        print("ffmpeg not found. Please install ffmpeg and ensure it's available on PATH, in your venv, or set FFMPEG/FFMPEG_PATH. You can also pass --ffmpeg.")
        sys.exit(2)

    targets = []
    if args.file:
        p = Path(args.file)
        if not p.exists():
            print("File not found:", p)
            sys.exit(2)
        targets = [p]
    else:
        src_dir = Path(args.src)
        if not src_dir.exists():
            print("Source directory not found:", src_dir)
            sys.exit(2)
        targets = scan_files(src_dir, args.from_ext)

    if not targets:
        print("No files found to convert.")
        return

    converted = 0
    skipped = 0
    failed = 0

    for src in targets:
        if args.convert_filenames:
            new_stem = to_snake_case(src.stem)
            dst = src.parent / (new_stem + '.' + args.to_ext)
        else:
            dst = src.with_suffix('.' + args.to_ext)
        if dst.exists() and not args.overwrite:
            print("Skipping (exists):", src, "->", dst)
            skipped += 1
            continue

        ok, err = convert_file(ffmpeg, src, dst, args.to_ext, args.quality, args.bitrate, args.dry_run)
        if ok:
            print("Converted:", src, "->", dst)
            converted += 1
        else:
            print("Failed:", src, err)
            failed += 1

    print(f"Done. converted={converted}, skipped={skipped}, failed={failed}")


if __name__ == '__main__':
    main()
