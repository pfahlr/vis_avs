#!/usr/bin/env bash
set -euo pipefail

SHARD_INDEX="${SHARD_INDEX:-0}"
SHARD_TOTAL="${SHARD_TOTAL:-1}"
CMAKE_BUILD_PARALLEL_LEVEL="${CMAKE_BUILD_PARALLEL_LEVEL:-1}"

ensure_test_binaries_are_executable() {
  local candidate
  for candidate in "build/tests" "build/tools"; do
    if [[ -d "${candidate}" ]]; then
      find "${candidate}" -type f -print0 | xargs -0 -r chmod +x
    fi
  done
}

ensure_test_binaries_are_executable

if [[ "${SHARD_TOTAL}" -le 1 ]]; then
  ctest --test-dir build --output-on-failure --parallel "${CMAKE_BUILD_PARALLEL_LEVEL}"
  exit 0
fi

python3 .github/scripts/run_sharded_ctest.py
