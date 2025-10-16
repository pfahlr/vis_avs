#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR=${BUILD_DIR:-build}

if [ ! -d "${BUILD_DIR}" ]; then
  echo "[update_goldens] build directory '${BUILD_DIR}' does not exist" >&2
  echo "Run CMake configure step first (e.g., cmake -S . -B ${BUILD_DIR})" >&2
  exit 1
fi

cmake --build "${BUILD_DIR}" --target core_effects_tests
UPDATE_GOLDENS=1 "${BUILD_DIR}/tests/core_effects_tests"
