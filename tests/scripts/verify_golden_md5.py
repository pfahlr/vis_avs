#!/usr/bin/env python3

import argparse
import json
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
  parser = argparse.ArgumentParser(description="Verify gen_golden_md5 output against expected snapshot")
  parser.add_argument("--binary", required=True, help="Path to gen_golden_md5 executable")
  parser.add_argument("--expected", required=True, help="Path to expected JSON file")
  parser.add_argument("--frames", type=int, default=10)
  parser.add_argument("--width", type=int, default=320)
  parser.add_argument("--height", type=int, default=240)
  parser.add_argument("--seed", type=int, default=1234)
  parser.add_argument("--preset", help="Optional preset path")
  return parser.parse_args()


def run_tool(binary: Path, args: argparse.Namespace) -> dict:
  cmd = [str(binary), "--frames", str(args.frames), "--width", str(args.width), "--height", str(args.height),
         "--seed", str(args.seed)]
  if args.preset:
    cmd.extend(["--preset", args.preset])
  result = subprocess.run(cmd, check=True, capture_output=True, text=True)
  return json.loads(result.stdout)


def load_expected(path: Path) -> dict:
  with path.open("r", encoding="utf-8") as handle:
    return json.load(handle)


def main() -> int:
  args = parse_args()
  binary = Path(args.binary)
  expected_path = Path(args.expected)
  if not binary.exists():
    print(f"Executable not found: {binary}", file=sys.stderr)
    return 2
  if not expected_path.exists():
    print(f"Expected file not found: {expected_path}", file=sys.stderr)
    return 2

  generated = run_tool(binary, args)
  expected = load_expected(expected_path)

  if generated != expected:
    print("Generated hashes differ from expected snapshot", file=sys.stderr)
    return 1
  return 0


if __name__ == "__main__":
  sys.exit(main())

