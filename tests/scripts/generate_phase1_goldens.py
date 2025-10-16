#!/usr/bin/env python3
"""Helper script to generate deterministic render goldens for phase-one presets.

The deterministic render integration tests expect golden frame hashes and PNG dumps
for each preset under ``tests/golden/phase1``.  The repository intentionally ships
placeholders only so that the heavy binary payload does not need to live in source
control.  This script streamlines generating those captures locally by driving the
headless ``avs-player`` binary that is built as part of the project.
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Iterable, List, Optional


def _default_source_dir() -> Path:
    return Path(__file__).resolve().parents[2]


def _parse_args(argv: Optional[Iterable[str]] = None) -> argparse.Namespace:
    source_dir = _default_source_dir()
    parser = argparse.ArgumentParser(
        description=(
            "Generate golden frame captures for the phase-one deterministic render "
            "tests by invoking the headless avs-player."
        )
    )
    parser.add_argument(
        "--source-dir",
        type=Path,
        default=source_dir,
        help="Path to the repository root (defaults to this script's grandparent)",
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=source_dir / "build",
        help="CMake build directory that contains apps/avs-player/avs-player",
    )
    parser.add_argument(
        "--player",
        type=Path,
        default=None,
        help="Path to the avs-player executable (defaults to <build-dir>/apps/avs-player/avs-player)",
    )
    parser.add_argument(
        "--preset-root",
        type=Path,
        default=None,
        help="Directory containing .avs presets (defaults to tests/data/phase1)",
    )
    parser.add_argument(
        "--output-root",
        type=Path,
        default=None,
        help=(
            "Directory that will receive generated captures. Defaults to "
            "<build-dir>/generated_goldens/phase1"
        ),
    )
    parser.add_argument(
        "--frames",
        type=int,
        default=10,
        help="Number of frames to capture per preset (defaults to 10 to match the tests)",
    )
    parser.add_argument(
        "--overwrite",
        action="store_true",
        help="Remove any existing output directories before capturing",
    )
    parser.add_argument(
        "--install-to-golden",
        action="store_true",
        help=(
            "After generation, copy the capture into tests/golden/phase1/<preset>. "
            "This updates the working tree in-place so use with care."
        ),
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be executed without invoking avs-player",
    )
    return parser.parse_args(list(argv) if argv is not None else None)


def _find_presets(preset_root: Path) -> List[Path]:
    presets = sorted(preset_root.glob("*.avs"))
    if not presets:
        raise FileNotFoundError(f"No presets found under {preset_root}")
    return presets


def _ensure_executable(path: Path) -> None:
    if not path.exists():
        raise FileNotFoundError(f"avs-player executable not found at {path}")
    if not path.is_file():
        raise FileNotFoundError(f"avs-player path is not a file: {path}")


def _run_player(command: List[str], dry_run: bool) -> None:
    if dry_run:
        print("DRY-RUN:", " ".join(command))
        return
    subprocess.run(command, check=True)


def _copy_tree(src: Path, dest: Path, overwrite: bool) -> None:
    if dest.exists():
        if overwrite:
            shutil.rmtree(dest)
        else:
            raise FileExistsError(f"Destination {dest} exists. Use --overwrite to replace it.")
    shutil.copytree(src, dest)


def main(argv: Optional[Iterable[str]] = None) -> int:
    args = _parse_args(argv)
    source_dir: Path = args.source_dir.resolve()
    build_dir: Path = args.build_dir.resolve()
    player: Path = (args.player or build_dir / "apps/avs-player/avs-player").resolve()
    preset_root: Path = (args.preset_root or source_dir / "tests/data/phase1").resolve()
    output_root: Path = (
        args.output_root or build_dir / "generated_goldens" / "phase1"
    ).resolve()
    tests_golden_root: Path = source_dir / "tests/golden/phase1"

    if args.dry_run:
        if not player.exists():
            print(
                f"DRY-RUN: avs-player not found at {player}; commands will be printed only.",
                file=sys.stderr,
            )
    else:
        _ensure_executable(player)
    presets = _find_presets(preset_root)
    wav = source_dir / "tests/data/test.wav"
    if not wav.exists():
        raise FileNotFoundError(f"Test WAV not found at {wav}")

    output_root.mkdir(parents=True, exist_ok=True)

    for preset in presets:
        preset_name = preset.stem
        capture_dir = output_root / preset_name
        if capture_dir.exists() and args.overwrite:
            shutil.rmtree(capture_dir)
        capture_dir.mkdir(parents=True, exist_ok=True)
        command = [
            str(player),
            "--headless",
            "--wav",
            str(wav),
            "--preset",
            str(preset),
            "--frames",
            str(args.frames),
            "--out",
            str(capture_dir),
        ]
        print(f"Capturing {preset_name} → {capture_dir}")
        try:
            _run_player(command, args.dry_run)
        except subprocess.CalledProcessError as exc:
            print(f"Failed to render {preset_name}: {exc}", file=sys.stderr)
            return exc.returncode or 1

        preset_copy = capture_dir / "preset.avs"
        shutil.copy2(preset, preset_copy)

        if args.install_to_golden:
            dest_dir = tests_golden_root / preset_name
            try:
                _copy_tree(capture_dir, dest_dir, overwrite=args.overwrite)
            except FileExistsError as err:
                print(err, file=sys.stderr)
                return 1

    print("All captures completed.")
    if args.install_to_golden:
        print("Golden directories updated under", tests_golden_root)
    else:
        print("Generated captures available under", output_root)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
