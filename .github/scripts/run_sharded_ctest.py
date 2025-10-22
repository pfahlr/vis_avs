#!/usr/bin/env python3
"""Utility to shard ctest execution for GitHub Actions."""

import os
import re
import subprocess
import sys


def resolve_build_dir() -> str:
    """Return the path to the CTest binary directory."""
    env_dir = os.environ.get("CTEST_BINARY_DIRECTORY")
    if env_dir:
        if os.path.isdir(env_dir):
            return env_dir
        raise FileNotFoundError(f"Configured CTest directory '{env_dir}' does not exist")

    default_dir = "build"
    if os.path.isdir(default_dir):
        return default_dir

    # Fall back to scanning for a directory that contains CTest metadata.
    for entry in os.scandir("."):
        if not entry.is_dir():
            continue
        candidate = entry.path
        ctest_file = os.path.join(candidate, "CTestTestfile.cmake")
        if os.path.isfile(ctest_file):
            return candidate
        nested_candidate = os.path.join(candidate, "build")
        nested_ctest_file = os.path.join(nested_candidate, "CTestTestfile.cmake")
        if os.path.isfile(nested_ctest_file):
            return nested_candidate

    raise FileNotFoundError(
        "Unable to locate a CTest binary directory. Ensure the build artifact "
        "is downloaded and extracted into the workspace."
    )


def discover_tests(build_dir: str) -> list[str]:
    """Return the sorted list of discovered ctest names."""
    ctest_list = subprocess.check_output(
        ["ctest", "--test-dir", build_dir, "-N"], text=True
    )
    tests: list[str] = []
    for line in ctest_list.splitlines():
        line = line.strip()
        if line.startswith("Test #"):
            parts = line.split(": ", 1)
            if len(parts) == 2:
                tests.append(parts[1].strip())
    tests.sort()
    return tests


def shard_tests(tests: list[str], shard_index: int, shard_total: int) -> list[str]:
    """Select the subset of tests that belong to the shard."""
    return tests[shard_index::shard_total]


def run_sharded_ctest() -> int:
    shard_index = int(os.environ["SHARD_INDEX"])
    shard_total = int(os.environ["SHARD_TOTAL"])

    try:
        build_dir = resolve_build_dir()
    except FileNotFoundError as exc:  # pragma: no cover - informative failure path
        print(str(exc), file=sys.stderr)
        return 1

    tests = discover_tests(build_dir)
    if not tests:
        print("No tests discovered; exiting shard early.")
        return 0

    shard_selection = shard_tests(tests, shard_index, shard_total)
    if not shard_selection:
        print("Shard has no tests to run; exiting.")
        return 0

    pattern = "^(%s)$" % "|".join(re.escape(test) for test in shard_selection)
    cmd = [
        "ctest",
        "--test-dir",
        build_dir,
        "--output-on-failure",
        "--parallel",
        os.environ.get("CMAKE_BUILD_PARALLEL_LEVEL", "1"),
        "--tests-regex",
        pattern,
    ]

    print("Running:", " ".join(cmd))
    sys.stdout.flush()
    return subprocess.call(cmd)


if __name__ == "__main__":
    sys.exit(run_sharded_ctest())
