#!/usr/bin/env python3
"""Utility runner for repository tooling.

Currently supports executing ``get_unimplemented_effects.sh`` and
presenting selected fields from its JSON output using Rich tables.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path
from typing import Any, Iterable

from rich.console import Console
from rich.table import Table

REPO_ROOT = Path(__file__).resolve().parent.parent
TOOLS_DIR = REPO_ROOT / "tools"
DEFAULT_FIELDS = ("name", "origin", "status", "implementation_files")


def run_get_unimplemented_effects() -> list[dict[str, Any]]:
    """Execute the shell tool and return the parsed JSON payload."""

    script = TOOLS_DIR / "get_unimplemented_effects.sh"
    if not script.is_file():
        raise FileNotFoundError(f"Unable to locate tool: {script}")

    result = subprocess.run(
        [str(script)],
        cwd=str(REPO_ROOT),
        check=False,
        capture_output=True,
        text=True,
    )

    if result.returncode != 0:
        raise RuntimeError(
            "get_unimplemented_effects.sh exited with "
            f"status {result.returncode}:\n{result.stderr.strip()}"
        )

    try:
        payload = json.loads(result.stdout)
    except json.JSONDecodeError as exc:  # pragma: no cover - defensive
        raise RuntimeError("Failed to parse JSON output from get_unimplemented_effects.sh") from exc

    if not isinstance(payload, list):
        raise RuntimeError("Unexpected payload format: expected a list of effect entries")

    return payload


def field_value(entry: dict[str, Any], field: str) -> str:
    """Resolve a printable value for the requested field."""

    if field == "name":
        candidate = entry.get("display_name") or entry.get("registry_id")
        candidate = candidate or entry.get("machine_readable_effect_name")
        return str(candidate) if candidate is not None else ""
    if field == "implementation_files":
        impl = entry.get("implementation_files")
        if not impl:
            return "—"
        if isinstance(impl, dict):
            parts = [f"{key}: {value}" for key, value in sorted(impl.items())]
            return "\n".join(parts)
        return str(impl)
    value = entry.get(field)
    if value is None:
        return ""
    if isinstance(value, (list, tuple)):
        return ", ".join(str(v) for v in value)
    return str(value)


def build_table(entries: Iterable[dict[str, Any]], fields: Iterable[str]) -> Table:
    table = Table(show_header=True, header_style="bold cyan")
    for field in fields:
        table.add_column(field.replace("_", " ").title())

    for entry in entries:
        table.add_row(*(field_value(entry, field) for field in fields))
    return table


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Repository tool runner")
    parser.add_argument(
        "tool",
        choices=["get_unimplemented_effects"],
        nargs="?",
        default="get_unimplemented_effects",
        help="Tool to execute (currently only get_unimplemented_effects)",
    )
    parser.add_argument(
        "--fields",
        metavar="FIELD",
        nargs="+",
        default=list(DEFAULT_FIELDS),
        help="Fields from the tool output to display",
    )
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    console = Console()

    if args.tool == "get_unimplemented_effects":
        entries = run_get_unimplemented_effects()
    else:  # pragma: no cover - defensive for future tools
        console.print(f"Unsupported tool: {args.tool}", style="bold red")
        return 2

    table = build_table(entries, args.fields)
    console.print(table)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
