#!/usr/bin/env python3
"""Audit the legacy effect registry against the manifest."""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys
from typing import Dict, List, Optional, Set

_ALLOWED_KINDS = {"effect", "sentinel", "support"}
_REGISTER_REGEX = re.compile(r"REGISTER_AVS_EFFECT\([^,]+,\s*\"([^\"]+)\"\)")


def _parse_manifest(path: pathlib.Path) -> List[Dict[str, object]]:
  text = path.read_text(encoding="utf-8")
  try:
    import yaml  # type: ignore
  except Exception:  # pragma: no cover - PyYAML optional
    yaml = None  # type: ignore

  if yaml is not None:
    data = yaml.safe_load(text)
    if not isinstance(data, dict) or "entries" not in data:
      raise ValueError("Invalid manifest structure")
    entries = data["entries"]
    if not isinstance(entries, list):
      raise ValueError("Manifest entries must be a list")
    result: List[Dict[str, object]] = []
    for entry in entries:
      if not isinstance(entry, dict):
        raise ValueError("Manifest entry must be a mapping")
      token = entry.get("token")
      kind = entry.get("kind")
      modes = entry.get("modes", [])
      if not isinstance(token, str) or not token:
        raise ValueError("Manifest entry missing token")
      if not isinstance(kind, str) or kind not in _ALLOWED_KINDS:
        raise ValueError(f"Invalid kind for token {token!r}")
      if modes and not isinstance(modes, list):
        raise ValueError(f"Invalid modes list for token {token!r}")
      result.append({"token": token, "kind": kind, "modes": list(modes)})
    return result

  entries: List[Dict[str, object]] = []
  current: Optional[Dict[str, object]] = None
  in_modes = False
  for raw_line in text.splitlines():
    line = raw_line.strip()
    if not line or line.startswith("#"):
      continue
    if line.startswith("- token:"):
      token = line.split(":", 1)[1].strip().strip('"')
      current = {"token": token, "kind": None, "modes": []}
      entries.append(current)
      in_modes = False
      continue
    if current is None:
      continue
    if line.startswith("kind:"):
      kind = line.split(":", 1)[1].strip().strip('"')
      current["kind"] = kind
      in_modes = False
      continue
    if line.startswith("modes:"):
      in_modes = True
      continue
    if in_modes and line.startswith("-"):
      mode = line[1:].strip().strip('"')
      modes = current.setdefault("modes", [])
      if isinstance(modes, list):
        modes.append(mode)
      continue
    if in_modes and not line.startswith("-"):
      in_modes = False

  for entry in entries:
    token = entry.get("token")
    kind = entry.get("kind")
    if not token:
      raise ValueError("Manifest entry missing token")
    if kind not in _ALLOWED_KINDS:
      raise ValueError(f"Invalid kind for token {token!r}")
    if not isinstance(entry.get("modes"), list):
      raise ValueError(f"Invalid modes list for token {token!r}")
  return entries


def _collect_registered_tokens(root: pathlib.Path) -> Set[str]:
  tokens: Set[str] = set()
  for path in root.rglob("*.cpp"):
    text = path.read_text(encoding="utf-8")
    for match in _REGISTER_REGEX.finditer(text):
      tokens.add(match.group(1))
  return tokens


def main(argv: Optional[List[str]] = None) -> int:
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument("--out", type=pathlib.Path)
  parser.add_argument("--ci", action="store_true", help="Reserved for CI integration")
  args = parser.parse_args(argv)

  repo_root = pathlib.Path(__file__).resolve().parents[1]
  manifest_path = repo_root / "docs" / "effects_manifest.yaml"
  if not manifest_path.exists():
    raise SystemExit("effects manifest not found")

  manifest_entries = _parse_manifest(manifest_path)
  manifest_tokens = {entry["token"] for entry in manifest_entries}
  if len(manifest_tokens) != len(manifest_entries):
    raise SystemExit("Duplicate tokens found in manifest")

  registry_tokens = _collect_registered_tokens(repo_root / "libs" / "avs-effects-legacy" / "src")

  missing = manifest_tokens - registry_tokens
  extra = registry_tokens - manifest_tokens
  if missing:
    raise SystemExit(f"Unregistered tokens: {sorted(missing)}")
  if extra:
    raise SystemExit(f"Tokens not present in manifest: {sorted(extra)}")

  if args.out:
    payload = {
      "tokens": sorted(registry_tokens),
      "entries": manifest_entries,
    }
    args.out.write_text(json.dumps(payload, indent=2), encoding="utf-8")

  print("Effect audit passed.")
  return 0


if __name__ == "__main__":
  sys.exit(main())
