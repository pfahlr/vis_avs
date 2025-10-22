#!/usr/bin/env bash
set -euo pipefail

SHARD_INDEX="${SHARD_INDEX:-0}"
SHARD_TOTAL="${SHARD_TOTAL:-1}"
CMAKE_BUILD_PARALLEL_LEVEL="${CMAKE_BUILD_PARALLEL_LEVEL:-1}"

if [[ "${SHARD_TOTAL}" -le 1 ]]; then
  ctest --test-dir build --output-on-failure --parallel "${CMAKE_BUILD_PARALLEL_LEVEL}"
  exit 0
fi

python3 .github/scripts/run_sharded_ctest.py
