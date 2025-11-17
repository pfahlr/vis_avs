#!/usr/bin/env bash
set -euo pipefail

DISTRO="${DISTRO:-}"
WORKSPACE="${GITHUB_WORKSPACE:-}"

if [[ -z "${DISTRO}" ]]; then
  echo "DISTRO environment variable must be provided" >&2
  exit 1
fi

if [[ -z "${WORKSPACE}" ]]; then
  echo "GITHUB_WORKSPACE environment variable must be provided" >&2
  exit 1
fi

COMMON_ARGS=(
  -S .
  -B build
  -G Ninja
  -DCMAKE_BUILD_TYPE=Release
  -DCMAKE_C_COMPILER_LAUNCHER=ccache
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
  -DCMAKE_INSTALL_PREFIX="${PWD}/install"
)

if [[ "${DISTRO}" == "ubuntu" ]]; then
  COMMON_ARGS+=(
    -DFETCHCONTENT_SOURCE_DIR_PORTAUDIO="${WORKSPACE}/external/portaudio"
    -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST="${WORKSPACE}/external/googletest"
    -DFETCHCONTENT_FULLY_DISCONNECTED=ON
  )
fi

cmake "${COMMON_ARGS[@]}"
