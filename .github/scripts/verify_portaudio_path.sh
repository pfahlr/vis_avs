#!/usr/bin/env bash
set -euo pipefail

DISTRO="${DISTRO:-}"
WORKSPACE="${GITHUB_WORKSPACE:-}" 

if [[ -z "${WORKSPACE}" ]]; then
  echo "GITHUB_WORKSPACE environment variable must be provided" >&2
  exit 1
fi

if [[ "${DISTRO}" != "ubuntu" ]]; then
  exit 0
fi

PORTAUDIO_DIR="${WORKSPACE}/external/portaudio"

test -d "${PORTAUDIO_DIR}/.git"
ls -la "${PORTAUDIO_DIR}" | head -n 20
